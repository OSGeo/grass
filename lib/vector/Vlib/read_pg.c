/*!
   \file lib/vector/Vlib/read_pg.c

   \brief Vector library - reading features (PostGIS format)

   Higher level functions for reading/writing/manipulating vectors.

   \todo Currently only points, linestrings and polygons are supported,
   implement also other types

   \todo Support multigeometries

   \todo PostGIS Topology - fix category handling (read categories
   from feature table)

   (C) 2011-2013 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Martin Landa <landa.martin gmail.com>
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#ifdef HAVE_POSTGRES
#include "pg_local_proto.h"

/* #define USE_CURSOR_RND */

static unsigned char *wkb_data;
static unsigned int wkb_data_length;

static int read_next_line_pg(struct Map_info *,
                             struct line_pnts *, struct line_cats *, int);
SF_FeatureType get_feature(struct Format_info_pg *, int, int);
static unsigned char *hex_to_wkb(const char *, int *);
static int point_from_wkb(const unsigned char *, int, int, int,
                          struct line_pnts *);
static int linestring_from_wkb(const unsigned char *, int, int, int,
                               struct line_pnts *, int);
static int polygon_from_wkb(const unsigned char *, int, int, int,
                            struct Format_info_cache *, int *);
static int geometry_collection_from_wkb(const unsigned char *, int, int, int,
                                        struct Format_info_cache *,
                                        struct feat_parts *);
static int error_corrupted_data(const char *);
static void reallocate_cache(struct Format_info_cache *, int);
static void add_fpart(struct feat_parts *, SF_FeatureType, int, int);
static int get_centroid(struct Map_info *, int, struct line_pnts *);
static void error_tuples(struct Format_info_pg *);
#endif

/*!
   \brief Read next feature from PostGIS layer. Skip
   empty features (level 1 without topology).
   t
   This function implements sequential access.

   The action of this routine can be modified by:
   - Vect_read_constraint_region()
   - Vect_read_constraint_type()
   - Vect_remove_constraints()

   \param Map pointer to Map_info structure
   \param[out] line_p container used to store line points within
   (pointer to line_pnts struct)
   \param[out] line_c container used to store line categories within
   (pointer line_cats struct)

   \return feature type
   \return -2 no more features (EOF)
   \return -1 out of memory
 */
int V1_read_next_line_pg(struct Map_info *Map,
                         struct line_pnts *line_p, struct line_cats *line_c)
{
#ifdef HAVE_POSTGRES
    G_debug(3, "V1_read_next_line_pg()");

    /* constraints not ignored */
    return read_next_line_pg(Map, line_p, line_c, FALSE);
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

/*!
   \brief Read next feature from PostGIS layer on topological level
   (simple feature access).

   This function implements sequential access.

   \param Map pointer to Map_info structure
   \param[out] line_p container used to store line points within
   (pointer to line_pnts struct)
   \param[out] line_c container used to store line categories within
   (pointer to line_cats struct)

   \return feature type
   \return -2 no more features (EOF)
   \return -1 on failure
 */
int V2_read_next_line_pg(struct Map_info *Map, struct line_pnts *line_p,
                         struct line_cats *line_c)
{
#ifdef HAVE_POSTGRES
    int line, ret;
    struct P_line *Line;
    struct bound_box lbox, mbox;

    struct Format_info_pg *pg_info;
    
    G_debug(3, "V2_read_next_line_pg()");

    pg_info = &(Map->fInfo.pg);
    
    if (Map->constraint.region_flag)
        Vect_get_constraint_box(Map, &mbox);

    ret = -1;
    while (TRUE) {
        line = Map->next_line;

        if (Map->next_line > Map->plus.n_lines)
            return -2;

        Line = Map->plus.Line[line];
        if (Line == NULL) {     /* skip dead features */
            Map->next_line++;
            continue;
        }

        if (Map->constraint.type_flag) {
            /* skip by type */
            if (!(Line->type & Map->constraint.type)) {
                Map->next_line++;
                continue;
            }
        }

        if (!pg_info->toposchema_name &&
            Line->type == GV_CENTROID) {
            G_debug(4, "Determine centroid for simple features");

            if (line_p != NULL) {
                int i, found;
                struct bound_box box;
                struct boxlist list;
                struct P_topo_c *topo = (struct P_topo_c *)Line->topo;

                /* get area bbox */
                Vect_get_area_box(Map, topo->area, &box);
                /* search in spatial index for centroid with area bbox */
                dig_init_boxlist(&list, TRUE);
                Vect_select_lines_by_box(Map, &box, Line->type, &list);

                found = -1;
                for (i = 0; i < list.n_values; i++) {
                    if (list.id[i] == line) {
                        found = i;
                        break;
                    }
                }

                if (found > -1) {
                    Vect_reset_line(line_p);
                    Vect_append_point(line_p, list.box[found].E,
                                      list.box[found].N, 0.0);
                }
            }
            if (line_c != NULL) {
                /* cat = FID and offset = FID for centroid */
                Vect_reset_cats(line_c);
                Vect_cat_set(line_c, 1, (int)Line->offset);
            }

            ret = GV_CENTROID;
        }
        else {
            /* ignore constraints */
            ret = read_next_line_pg(Map, line_p, line_c, TRUE);
            if (ret != Line->type) {
                G_warning(_("Unexpected feature type (%d) - should be (%d)"),
                          ret, Line->type);
                return -1;
            }
        }

        if (Map->constraint.region_flag) {
            /* skip by region */
            Vect_line_box(line_p, &lbox);
            if (!Vect_box_overlap(&lbox, &mbox)) {
                Map->next_line++;
                continue;
            }
        }

        /* skip by field ignored */
        
        Map->next_line++; /* read next */
                    
        return ret;
    }
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
#endif

    return -1;                  /* not reached */
}

/*!
   \brief Read feature from PostGIS layer at given offset (level 1 without topology)

   This function implements random access on level 1.

   \param Map pointer to Map_info structure 
   \param[out] line_p container used to store line points within
   (pointer line_pnts struct)
   \param[out] line_c container used to store line categories within
   (pointer line_cats struct)
   \param offset given offset 

   \return line type
   \return 0 dead line
   \return -2 no more features
   \return -1 out of memory
 */
int V1_read_line_pg(struct Map_info *Map,
                    struct line_pnts *line_p, struct line_cats *line_c,
                    off_t offset)
{
#ifdef HAVE_POSTGRES
    long fid;
    int ipart, type;

    struct Format_info_pg *pg_info;

    pg_info = &(Map->fInfo.pg);

    G_debug(3, "V1_read_line_pg(): offset = %lu offset_num = %lu",
            (long)offset, (long)pg_info->offset.array_num);

    if (offset >= pg_info->offset.array_num)
        return -2;              /* nothing to read */

    if (line_p != NULL)
        Vect_reset_line(line_p);
    if (line_c != NULL)
        Vect_reset_cats(line_c);

    fid = pg_info->offset.array[offset];
    G_debug(4, "  fid = %ld", fid);

    /* read feature to cache if necessary */
    if (pg_info->cache.fid != fid) {
        int type;

        G_debug(3, "read (%s) feature (fid = %ld) to cache",
                pg_info->table_name, fid);
        get_feature(pg_info, fid, -1);

        if (pg_info->cache.sf_type == SF_NONE) {
            G_warning(_("Feature %ld without geometry skipped"), fid);
            return -1;
        }

        type = (int)pg_info->cache.sf_type;
        if (type < 0)           /* -1 || - 2 */
            return type;
    }

    /* get data from cache */
    if (pg_info->cache.sf_type == SF_POINT ||
        pg_info->cache.sf_type == SF_LINESTRING)
        ipart = 0;
    else
        ipart = pg_info->offset.array[offset + 1];
    type = pg_info->cache.lines_types[ipart];
    G_debug(3, "read feature part: %d -> type = %d", ipart, type);

    if (line_p)
        Vect_append_points(line_p, pg_info->cache.lines[ipart], GV_FORWARD);

    if (line_c)
        Vect_cat_set(line_c, 1, (int)fid);

    return type;
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

/*!
   \brief Read feature from PostGIS layer on topological level

   This function implements random access on level 2.

   Note: Topology must be built at level >= GV_BUILD_BASE
    
   \param Map pointer to Map_info structure 
   \param[out] line_p container used to store line points within (pointer line_pnts struct)
   \param[out] line_c container used to store line categories within (pointer line_cats struct)
   \param line feature id to read

   \return feature type
   \return 0 dead feature
   \return -1 on error
 */
int V2_read_line_pg(struct Map_info *Map, struct line_pnts *line_p,
                    struct line_cats *line_c, int line)
{
#ifdef HAVE_POSTGRES
    int fid;
    
    struct Format_info_pg *pg_info;
    struct P_line *Line;

    pg_info = &(Map->fInfo.pg);
    
    if (line < 1 || line > Map->plus.n_lines) {
        G_warning(_("Attempt to access feature with invalid id (%d)"), line);
        return -1;
    }
    
    Line = Map->plus.Line[line];
    if (Line == NULL) {
        G_warning(_("Attempt to access dead feature %d"), line);
        return 0;
    }
    
    G_debug(4, "V2_read_line_pg() line = %d type = %d offset = %"PRI_OFF_T,
            line, Line->type, Line->offset);
    
    if (!line_p && !line_c)
        return Line->type;

    if (line_p)
        Vect_reset_line(line_p);
    if (Line->type == GV_CENTROID && !pg_info->toposchema_name) {
        /* simple features access: get centroid from sidx */
        return get_centroid(Map, line, line_p);
    }
    
    /* get feature id */
    if (pg_info->toposchema_name)
        fid = Line->offset;
    else
        fid = pg_info->offset.array[Line->offset];

    /* read feature */
    get_feature(pg_info, fid, Line->type);
    
    /* check sf type */
    if (pg_info->cache.sf_type == SF_NONE) {
        G_warning(_("Feature %d without geometry skipped"), line);
        return -1;
    }
    if (0 > (int)pg_info->cache.sf_type) /* -1 || - 2 */
        return -1;

    if (line_c) {
        int cat;

        Vect_reset_cats(line_c);
        if (!pg_info->toposchema_name) /* simple features access */
            cat = (int) Line->offset;
        else                           /* PostGIS Topology (cats are cached) */
            cat = pg_info->cache.lines_cats[0];
        if (cat != -1)
            Vect_cat_set(line_c, 1, cat);
    }

    if (line_p)
        Vect_append_points(line_p, pg_info->cache.lines[0], GV_FORWARD);
    
    return Line->type;
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

#ifdef HAVE_POSTGRES
/*!
   \brief Read next feature from PostGIS layer. 

   \param Map pointer to Map_info structure
   \param[out] line_p container used to store line points within
   (pointer to line_pnts struct)
   \param[out] line_c container used to store line categories within
   (pointer line_cats struct)
   \param ignore_constraints TRUE to ignore constraints (type, region)

   \return feature type
   \return -2 no more features (EOF)
   \return -1 out of memory
 */
int read_next_line_pg(struct Map_info *Map,
                      struct line_pnts *line_p, struct line_cats *line_c,
                      int ignore_constraints)
{
    int itype;
    SF_FeatureType sf_type;

    struct Format_info_pg *pg_info;
    struct bound_box mbox, lbox;
    struct line_pnts *iline;

    pg_info = &(Map->fInfo.pg);

    if (Map->constraint.region_flag && !ignore_constraints)
        Vect_get_constraint_box(Map, &mbox);

    while (TRUE) {
        /* reset data structures */
        if (line_p != NULL)
            Vect_reset_line(line_p);
        if (line_c != NULL)
            Vect_reset_cats(line_c);

        /* read feature to cache if necessary */
        while (pg_info->cache.lines_next == pg_info->cache.lines_num) {
            /* cache feature -> line_p & line_c */
            sf_type = get_feature(pg_info, -1, -1);
            
            if (sf_type == SF_NONE) {
                G_warning(_("Feature %ld without geometry skipped"), pg_info->cache.fid);
                return -1;
            }

            if ((int)sf_type < 0)       /* -1 || - 2 */
                return (int)sf_type;

            if (sf_type == SF_UNKNOWN || sf_type == SF_NONE) {
                G_warning(_("Feature without geometry. Skipped."));
                pg_info->cache.lines_next = pg_info->cache.lines_num = 0;
                continue;
            }

            G_debug(4, "%d lines read to cache", pg_info->cache.lines_num);
            /* store fid as offset to be used (used for topo access only */
            Map->head.last_offset = pg_info->cache.fid;
        }

        /* get data from cache */
        itype = pg_info->cache.lines_types[pg_info->cache.lines_next];
        iline = pg_info->cache.lines[pg_info->cache.lines_next];

        G_debug(4, "read next cached line %d (type = %d)",
                pg_info->cache.lines_next, itype);

        /* apply constraints */
        if (Map->constraint.type_flag && !ignore_constraints) {
            /* skip feature by type */
            if (!(itype & Map->constraint.type))
                continue;
        }

        if (line_p && Map->constraint.region_flag && !ignore_constraints) {
            /* skip feature by region */
            Vect_line_box(iline, &lbox);

            if (!Vect_box_overlap(&lbox, &mbox))
                continue;
        }

        /* skip feature by field ignored */

        if (line_p)
            Vect_append_points(line_p, iline, GV_FORWARD);

        if (line_c) {
            int cat;
            if (!pg_info->toposchema_name) /* simple features access */
                cat = (int)pg_info->cache.fid;
            else                           /* PostGIS Topology (cats are cached) */
                cat = pg_info->cache.lines_cats[pg_info->cache.lines_next];
            if (cat != -1)
                Vect_cat_set(line_c, 1, cat);
        }

        pg_info->cache.lines_next++;

        return itype;
    }

    return -1;                  /* not reached */
}

/*!
   \brief Read feature geometry

   Geometry is stored in lines cache.

   \param[in,out] pg_info pointer to Format_info_pg struct
   \param fid feature id to be read (-1 for next)
   \param type feature type (GV_POINT, GV_LINE, ...) - use only for topological access
   
   \return simple feature type (SF_POINT, SF_LINESTRING, ...)
   \return -1 on error
 */
SF_FeatureType get_feature(struct Format_info_pg *pg_info, int fid, int type)
{
    int seq_type;
    int force_type; /* force type (GV_BOUNDARY or GV_CENTROID) for topo access only */
    char *data;
    
    if (!pg_info->geom_column && !pg_info->topogeom_column) {
        G_warning(_("No geometry or topo geometry column defined"));
        return -1;
    }
    if (fid < 1) { /* sequantial access */
        if (pg_info->cursor_name == NULL &&
            Vect__open_cursor_next_line_pg(pg_info, FALSE) != 0)
        return -1;
    }
    else {         /* random access */
        if (!pg_info->fid_column && !pg_info->toposchema_name) {
            G_warning(_("Random access not supported. "
                        "Primary key not defined."));
            return -1;
        }

#ifdef USE_CURSOR_RND
        if (pg_info->cursor_fid > 0)
            pg_info->next_line = fid - pg_info->cursor_fid;
        else
            pg_info->next_line = 0;
        
        if (pg_info->next_line < 0 || pg_info->next_line > CURSOR_PAGE)
            Vect__close_cursor_pg(pg_info);
        
        if (pg_info->cursor_name == NULL &&
            Vect__open_cursor_line_pg(pg_info, fid, type) != 0)
            return -1;
#else
        pg_info->next_line = 0;
        if (Vect__select_line_pg(pg_info, fid, type) != 0)
            return -1;
#endif
    }

    /* do we need to fetch more records ? */
    if (PQntuples(pg_info->res) == CURSOR_PAGE &&
        PQntuples(pg_info->res) == pg_info->next_line) {
        char stmt[DB_SQL_MAX];

        PQclear(pg_info->res);

        sprintf(stmt, "FETCH %d in %s", CURSOR_PAGE, pg_info->cursor_name);
        G_debug(3, "SQL: %s", stmt);
        pg_info->res = PQexec(pg_info->conn, stmt);
        if (!pg_info->res || PQresultStatus(pg_info->res) != PGRES_TUPLES_OK) {
            error_tuples(pg_info);
            return -1;
        }
        pg_info->next_line = 0;
    }

    G_debug(3, "get_feature(): next_line = %d", pg_info->next_line);
    
    /* out of results ? */
    if (PQntuples(pg_info->res) == pg_info->next_line) {
        if (Vect__close_cursor_pg(pg_info) != 0)
            return -1; /* failure */
        else 
            return -2; /* nothing to read */
    }

    force_type = -1;
    if (pg_info->toposchema_name) {
        if (fid < 0) {
            /* sequatial access */
            seq_type = atoi(PQgetvalue(pg_info->res, pg_info->next_line, 2));
            if (seq_type == GV_BOUNDARY ||
                (seq_type == GV_LINE && pg_info->feature_type == SF_POLYGON))
                force_type = GV_BOUNDARY;
            else if (seq_type == GV_CENTROID)
                force_type = GV_CENTROID;
        }
        else {
            /* random access: check topological elemenent type consistency */
            if (type & GV_POINTS) {
                if (type == GV_POINT &&
                    strlen(PQgetvalue(pg_info->res, pg_info->next_line, 1)) != 0)
                    G_warning(_("Inconsistency in topology: detected centroid (should be point)"));
            }
            else {
                int left_face, right_face;
                
                left_face  = atoi(PQgetvalue(pg_info->res, pg_info->next_line, 1));
                right_face = atoi(PQgetvalue(pg_info->res, pg_info->next_line, 2));
                
                if (type == GV_LINE &&
                    (left_face != 0 || right_face != 0)) 
                    G_warning(_("Inconsistency in topology: detected boundary (should be line)"));
            }
        }
    }

    /* get geometry data */
    data = (char *)PQgetvalue(pg_info->res, pg_info->next_line, 0);
    
    /* load feature to the cache */
    pg_info->cache.sf_type = Vect__cache_feature_pg(data,
                                                    FALSE, force_type,
                                                    &(pg_info->cache), NULL);
    
    /* cache also categories (only for PostGIS Topology) */
    if (pg_info->toposchema_name) {
        if (!PQgetisnull(pg_info->res, pg_info->next_line, 3))
            pg_info->cache.lines_cats[pg_info->cache.lines_next] =
                atoi(PQgetvalue(pg_info->res, pg_info->next_line, 3)); 
        else
            pg_info->cache.lines_cats[pg_info->cache.lines_next] = -1; /* no cat */
    }

    /* set feature id */
    if (fid < 0) {
        pg_info->cache.fid =
            atoi(PQgetvalue(pg_info->res, pg_info->next_line, 1)); 
        pg_info->next_line++;
    }
    else {
        pg_info->cache.fid = fid;
    }
    
    return pg_info->cache.sf_type;
}

/*!
   \brief Convert HEX to WKB data

   \param hex_data HEX data
   \param[out] nbytes number of bytes in output buffer

   \return pointer to WKB data buffer
 */
unsigned char *hex_to_wkb(const char *hex_data, int *nbytes)
{
    unsigned int length;
    int i;

    length = strlen(hex_data) / 2 + 1;
    if (length > wkb_data_length) {
        wkb_data_length = length;
        wkb_data = G_realloc(wkb_data, wkb_data_length);
    }

    *nbytes = length - 1;
    for (i = 0; i < (*nbytes); i++) {
        wkb_data[i] =
            (unsigned
             char)((hex_data[2 * i] >
                    'F' ? hex_data[2 * i] - 0x57 : hex_data[2 * i] >
                    '9' ? hex_data[2 * i] - 0x37 : hex_data[2 * i] -
                    0x30) << 4);
        wkb_data[i] |=
            (unsigned char)(hex_data[2 * i + 1] >
                            'F' ? hex_data[2 * i + 1] -
                            0x57 : hex_data[2 * i + 1] >
                            '9' ? hex_data[2 * i + 1] -
                            0x37 : hex_data[2 * i + 1] - 0x30);
    }

    wkb_data[(*nbytes)] = 0;

    return wkb_data;
}

/*!
   \brief Read geometry from HEX data

   This code is inspired by OGRGeometryFactory::createFromWkb() from
   GDAL/OGR library.

   \param data HEX data
   \param skip_polygon skip polygons (level 1)
   \param force_type force GV_BOUNDARY or GV_CENTROID (used for PostGIS topology only)
   \param[out] cache lines cache
   \param[out] fparts used for building pseudo-topology (or NULL)

   \return simple feature type
   \return SF_UNKNOWN on error
 */
SF_FeatureType Vect__cache_feature_pg(const char *data, int skip_polygon,
                                      int force_type,
                                      struct Format_info_cache *cache,
                                      struct feat_parts * fparts)
{
    int ret, byte_order, nbytes, is3D;
    unsigned char *wkb_data;
    unsigned int wkb_flags;
    SF_FeatureType ftype;

    /* reset cache */
    cache->lines_num = 0;
    cache->fid = -1;
    /* next to be read from cache */
    cache->lines_next = 0;

    if (fparts)
        fparts->n_parts = 0;

    wkb_flags = 0;
    wkb_data = hex_to_wkb(data, &nbytes);

    if (nbytes < 5) {
        /* G_free(wkb_data); */
        if (nbytes > 0) {
            G_debug(3, "Vect__cache_feature_pg(): invalid geometry");
            G_warning(_("Invalid WKB content: %d bytes"), nbytes);
            return SF_UNKNOWN;
        }
        else {
            G_debug(3, "Vect__cache_feature_pg(): no geometry");
            return SF_NONE;
        }
    }

    /* parsing M coordinate not supported */
    memcpy(&wkb_flags, wkb_data + 1, 4);
    byte_order = (wkb_data[0] == 0 ? ENDIAN_BIG : ENDIAN_LITTLE);
    if (byte_order == ENDIAN_BIG)
        wkb_flags = SWAP32(wkb_flags);

    if (wkb_flags & 0x40000000) {
        G_warning(_("Reading EWKB with 4-dimensional coordinates (XYZM) "
                    "is not supported"));
        /* G_free(wkb_data); */
        return SF_UNKNOWN;
    }

    /* PostGIS EWKB format includes an  SRID, but this won't be       
       understood by OGR, so if the SRID flag is set, we remove the    
       SRID (bytes at offset 5 to 8).                                 
     */
    if (nbytes > 9 &&
        ((byte_order == ENDIAN_BIG && (wkb_data[1] & 0x20)) ||
         (byte_order == ENDIAN_LITTLE && (wkb_data[4] & 0x20)))) {
        memmove(wkb_data + 5, wkb_data + 9, nbytes - 9);
        nbytes -= 4;
        if (byte_order == ENDIAN_BIG)
            wkb_data[1] &= (~0x20);
        else
            wkb_data[4] &= (~0x20);
    }

    if (nbytes < 9 && nbytes != -1) {
        /* G_free(wkb_data); */
        return SF_UNKNOWN;
    }

    /* Get the geometry feature type. For now we assume that geometry
       type is between 0 and 255 so we only have to fetch one byte.
     */
    if (byte_order == ENDIAN_LITTLE) {
        ftype = (SF_FeatureType) wkb_data[1];
        is3D = wkb_data[4] & 0x80 || wkb_data[2] & 0x80;
    }
    else {
        ftype = (SF_FeatureType) wkb_data[4];
        is3D = wkb_data[1] & 0x80 || wkb_data[3] & 0x80;
    }
    G_debug(3, "Vect__cache_feature_pg(): sf_type = %d", ftype);

    /* allocate space in lines cache - be minimalistic

       more lines require eg. polygon with more rings, multi-features
       or geometry collections
     */
    if (!cache->lines) {
        reallocate_cache(cache, 1);
    }

    ret = -1;
    if (ftype == SF_POINT) {
        cache->lines_num = 1;
        cache->lines_types[0] = force_type == GV_CENTROID ? force_type : GV_POINT;
        ret = point_from_wkb(wkb_data, nbytes, byte_order,
                             is3D, cache->lines[0]);
        add_fpart(fparts, ftype, 0, 1);
    }
    else if (ftype == SF_LINESTRING) {
        cache->lines_num = 1;
        cache->lines_types[0] = force_type == GV_BOUNDARY ? force_type : GV_LINE;
        ret = linestring_from_wkb(wkb_data, nbytes, byte_order,
                                  is3D, cache->lines[0], FALSE);
        add_fpart(fparts, ftype, 0, 1);
    }
    else if (ftype == SF_POLYGON && !skip_polygon) {
        int nrings;

        ret = polygon_from_wkb(wkb_data, nbytes, byte_order,
                               is3D, cache, &nrings);
        add_fpart(fparts, ftype, 0, nrings);
    }
    else if (ftype == SF_MULTIPOINT ||
             ftype == SF_MULTILINESTRING ||
             ftype == SF_MULTIPOLYGON || ftype == SF_GEOMETRYCOLLECTION) {
        ret = geometry_collection_from_wkb(wkb_data, nbytes, byte_order,
                                           is3D, cache, fparts);
    }
    else {
        G_warning(_("Unsupported feature type %d"), ftype);
    }

    /* read next feature from cache */
    cache->lines_next = 0;

    /* G_free(wkb_data); */

    return ret > 0 ? ftype : SF_UNKNOWN;
}

/*!
   \brief Read point for WKB data

   See OGRPoint::importFromWkb() from GDAL/OGR library

   \param wkb_data WKB data
   \param nbytes number of bytes (WKB data buffer)
   \param byte_order byte order (ENDIAN_LITTLE, ENDIAN_BIG)
   \param with_z WITH_Z for 3D data
   \param[out] line_p point geometry (pointer to line_pnts struct)

   \return wkb size
   \return -1 on error
 */
int point_from_wkb(const unsigned char *wkb_data, int nbytes, int byte_order,
                   int with_z, struct line_pnts *line_p)
{
    double x, y, z;

    if (nbytes < 21 && nbytes != -1)
        return -1;

    /* get vertex */
    memcpy(&x, wkb_data + 5, 8);
    memcpy(&y, wkb_data + 5 + 8, 8);

    if (byte_order == ENDIAN_BIG) {
        SWAPDOUBLE(&x);
        SWAPDOUBLE(&y);
    }

    if (with_z) {
        if (nbytes < 29 && nbytes != -1)
            return -1;

        memcpy(&z, wkb_data + 5 + 16, 8);
        if (byte_order == ENDIAN_BIG) {
            SWAPDOUBLE(&z);
        }
    }
    else {
        z = 0.0;
    }

    if (line_p) {
        Vect_reset_line(line_p);
        Vect_append_point(line_p, x, y, z);
    }

    return 5 + 8 * (with_z == WITH_Z ? 3 : 2);
}

/*!
   \brief Read line for WKB data

   See OGRLineString::importFromWkb() from GDAL/OGR library

   \param wkb_data WKB data
   \param nbytes number of bytes (WKB data buffer)
   \param byte_order byte order (ENDIAN_LITTLE, ENDIAN_BIG)
   \param with_z WITH_Z for 3D data
   \param[out] line_p line geometry (pointer to line_pnts struct)

   \return wkb size
   \return -1 on error
 */
int linestring_from_wkb(const unsigned char *wkb_data, int nbytes,
                        int byte_order, int with_z, struct line_pnts *line_p,
                        int is_ring)
{
    int npoints, point_size, buff_min_size, offset;
    int i;
    double x, y, z;

    if (is_ring)
        offset = 5;
    else
        offset = 0;

    if (is_ring && nbytes < 4 && nbytes != -1)
        return error_corrupted_data(NULL);

    /* get the vertex count */
    memcpy(&npoints, wkb_data + (5 - offset), 4);

    if (byte_order == ENDIAN_BIG) {
        npoints = SWAP32(npoints);
    }

    /* check if the wkb stream buffer is big enough to store fetched
       number of points.  16 or 24 - size of point structure
     */
    point_size = with_z ? 24 : 16;
    if (npoints < 0 || npoints > INT_MAX / point_size)
        return error_corrupted_data(NULL);

    buff_min_size = point_size * npoints;

    if (nbytes != -1 && buff_min_size > nbytes - (9 - offset))
        return error_corrupted_data(_("Length of input WKB is too small"));

    if (line_p)
        Vect_reset_line(line_p);

    /* get the vertex */
    for (i = 0; i < npoints; i++) {
        memcpy(&x, wkb_data + (9 - offset) + i * point_size, 8);
        memcpy(&y, wkb_data + (9 - offset) + 8 + i * point_size, 8);
        if (with_z)
            memcpy(&z, wkb_data + (9 - offset) + 16 + i * point_size, 8);
        else
            z = 0.0;

        if (byte_order == ENDIAN_BIG) {
            SWAPDOUBLE(&x);
            SWAPDOUBLE(&y);
            if (with_z)
                SWAPDOUBLE(&z);
        }

        if (line_p)
            Vect_append_point(line_p, x, y, z);
    }

    return (9 - offset) + (with_z == WITH_Z ? 3 : 2) * 8 * line_p->n_points;
}

/*!
   \brief Read polygon for WKB data

   See OGRPolygon::importFromWkb() from GDAL/OGR library

   \param wkb_data WKB data
   \param nbytes number of bytes (WKB data buffer)
   \param byte_order byte order (ENDIAN_LITTLE, ENDIAN_BIG)
   \param with_z WITH_Z for 3D data
   \param[out] line_p array of rings (pointer to line_pnts struct)
   \param[out] nrings number of rings

   \return wkb size
   \return -1 on error
 */
int polygon_from_wkb(const unsigned char *wkb_data, int nbytes,
                     int byte_order, int with_z,
                     struct Format_info_cache *cache, int *nrings)
{
    int data_offset, i, nsize, isize;
    struct line_pnts *line_i;

    if (nbytes < 9 && nbytes != -1)
        return -1;

    /* get the ring count */
    memcpy(nrings, wkb_data + 5, 4);
    if (byte_order == ENDIAN_BIG) {
        *nrings = SWAP32(*nrings);
    }
    if (*nrings < 0) {
        return -1;
    }

    /* reallocate space for islands if needed */
    reallocate_cache(cache, *nrings);
    cache->lines_num += *nrings;

    /* each ring has a minimum of 4 bytes (point count) */
    if (nbytes != -1 && nbytes - 9 < (*nrings) * 4) {
        return error_corrupted_data(_("Length of input WKB is too small"));
    }

    data_offset = 9;
    if (nbytes != -1)
        nbytes -= data_offset;

    /* get the rings */
    nsize = 9;
    for (i = 0; i < (*nrings); i++) {
        if (cache->lines_next >= cache->lines_num)
            G_fatal_error(_("Invalid cache index %d (max: %d)"),
                          cache->lines_next, cache->lines_num);
        line_i = cache->lines[cache->lines_next];
        cache->lines_types[cache->lines_next++] = GV_BOUNDARY;

        linestring_from_wkb(wkb_data + data_offset, nbytes, byte_order,
                            with_z, line_i, TRUE);

        if (nbytes != -1) {
            isize = 4 + 8 * (with_z == WITH_Z ? 3 : 2) * line_i->n_points;
            nbytes -= isize;
        }

        nsize += isize;
        data_offset += isize;
    }

    return nsize;
}

/*!
   \brief Read geometry collection for WKB data

   See OGRGeometryCollection::importFromWkbInternal() from GDAL/OGR library

   \param wkb_data WKB data
   \param nbytes number of bytes (WKB data buffer)
   \param byte_order byte order (ENDIAN_LITTLE, ENDIAN_BIG)
   \param with_z WITH_Z for 3D data
   \param ipart part to cache (starts at 0)
   \param[out] cache lines cache
   \param[in,out] fparts feature parts (required for building pseudo-topology)

   \return number of parts
   \return -1 on error
 */
int geometry_collection_from_wkb(const unsigned char *wkb_data, int nbytes,
                                 int byte_order, int with_z,
                                 struct Format_info_cache *cache,
                                 struct feat_parts *fparts)
{
    int ipart, nparts, data_offset, nsize;
    unsigned char *wkb_subdata;
    SF_FeatureType ftype;

    if (nbytes < 9 && nbytes != -1)
        return error_corrupted_data(NULL);

    /* get the geometry count */
    memcpy(&nparts, wkb_data + 5, 4);
    if (byte_order == ENDIAN_BIG) {
        nparts = SWAP32(nparts);
    }
    if (nparts < 0 || nparts > INT_MAX / 9) {
        return error_corrupted_data(NULL);
    }
    G_debug(5, "\t(geometry collections) parts: %d", nparts);

    /* each geometry has a minimum of 9 bytes */
    if (nbytes != -1 && nbytes - 9 < nparts * 9) {
        return error_corrupted_data(_("Length of input WKB is too small"));
    }

    data_offset = 9;
    if (nbytes != -1)
        nbytes -= data_offset;

    /* reallocate space for parts if needed */
    reallocate_cache(cache, nparts);

    /* get parts */
    for (ipart = 0; ipart < nparts; ipart++) {
        wkb_subdata = (unsigned char *)wkb_data + data_offset;
        if (nbytes < 9 && nbytes != -1)
            return error_corrupted_data(NULL);

        if (byte_order == ENDIAN_LITTLE) {
            ftype = (SF_FeatureType) wkb_subdata[1];
        }
        else {
            ftype = (SF_FeatureType) wkb_subdata[4];
        }

        if (ftype == SF_POINT) {
            cache->lines_types[cache->lines_next] = GV_POINT;
            nsize = point_from_wkb(wkb_subdata, nbytes, byte_order, with_z,
                                   cache->lines[cache->lines_next]);
            cache->lines_num++;
            add_fpart(fparts, ftype, cache->lines_next, 1);
            cache->lines_next++;
        }
        else if (ftype == SF_LINESTRING) {
            cache->lines_types[cache->lines_next] = GV_LINE;
            nsize =
                linestring_from_wkb(wkb_subdata, nbytes, byte_order, with_z,
                                    cache->lines[cache->lines_next], FALSE);
            cache->lines_num++;
            add_fpart(fparts, ftype, cache->lines_next, 1);
            cache->lines_next++;
        }
        else if (ftype == SF_POLYGON) {
            int idx, nrings;

            idx = cache->lines_next;
            nsize = polygon_from_wkb(wkb_subdata, nbytes, byte_order,
                                     with_z, cache, &nrings);
            add_fpart(fparts, ftype, idx, nrings);
        }
        else if (ftype == SF_GEOMETRYCOLLECTION ||
                 ftype == SF_MULTIPOLYGON ||
                 ftype == SF_MULTILINESTRING || ftype == SF_MULTIPOLYGON) {
            geometry_collection_from_wkb(wkb_subdata, nbytes, byte_order,
                                         with_z, cache, fparts);
        }
        else {
            G_warning(_("Unsupported feature type %d"), ftype);
        }

        if (nbytes != -1) {
            nbytes -= nsize;
        }

        data_offset += nsize;
    }

    return nparts;
}

/*!
   \brief Report error message

   \param msg message (NULL)

   \return -1
 */
int error_corrupted_data(const char *msg)
{
    if (msg)
        G_warning(_("Corrupted data. %s."), msg);
    else
        G_warning(_("Corrupted data"));

    return -1;
}

/*!
  \brief Create select cursor for sequential access (internal use only)
  
  Allocated cursor name should be freed by G_free().
  
  \param pg_info pointer to Format_info_pg struct
  \param fetch_all TRUE to fetch all records
  \param[out] cursor name
  
  \return 0 on success
  \return -1 on failure
*/
int Vect__open_cursor_next_line_pg(struct Format_info_pg *pg_info, int fetch_all)
{
    char stmt[DB_SQL_MAX];
    
    if (Vect__execute_pg(pg_info->conn, "BEGIN") == -1)
        return -1;
    
    /* set cursor name */
    G_asprintf(&(pg_info->cursor_name),
               "%s_%s_%p",  pg_info->schema_name, pg_info->table_name, pg_info->conn);
    
    if (!pg_info->toposchema_name) {
        /* simple feature access (geom, fid) */
        /* TODO: start_fid */
        sprintf(stmt,
                "DECLARE %s CURSOR FOR SELECT %s,%s FROM \"%s\".\"%s\" ORDER BY %s",
                pg_info->cursor_name, pg_info->geom_column, pg_info->fid_column, pg_info->schema_name,
                pg_info->table_name, pg_info->fid_column);
    }
    else {
        /* topology access (geom,fid,type) */
        /* TODO: optimize SQL statement (for points/centroids) */
        sprintf(stmt,
                "DECLARE %s CURSOR FOR "
                "SELECT geom,id,type,fid FROM ("
                "SELECT tt.node_id AS id,tt.geom, %d AS type, ft.fid AS fid FROM \"%s\".node AS tt "
                "LEFT JOIN \"%s\" AS ft ON (%s).type = 1 AND (%s).id = node_id "
                "WHERE containing_face IS NULL AND node_id NOT IN "
                "(SELECT node FROM (SELECT start_node AS node FROM \"%s\".edge GROUP BY start_node UNION ALL "
                "SELECT end_node AS node FROM \"%s\".edge GROUP BY end_node) AS foo) UNION ALL "
                "SELECT tt.node_id AS id,tt.geom, %d AS type, ft.fid AS fid FROM \"%s\".node AS tt "
                "LEFT JOIN \"%s\" AS ft ON (%s).type = 3 AND (%s).id = containing_face "
                "WHERE containing_face IS NOT NULL AND node_id NOT IN "
                "(SELECT node FROM (SELECT start_node AS node FROM \"%s\".edge GROUP BY start_node UNION ALL "
                "SELECT end_node AS node FROM \"%s\".edge GROUP BY end_node) AS foo) UNION ALL "
                "SELECT tt.edge_id AS id, tt.geom, %d AS type, ft.fid AS fid FROM \"%s\".edge AS tt "
                "LEFT JOIN \"%s\" AS ft ON (%s).type = 2 AND (%s).id = edge_id "
                "WHERE left_face = 0 AND right_face = 0 UNION ALL "
                "SELECT tt.edge_id AS id, tt.geom, %d AS type, ft.fid AS fid FROM \"%s\".edge AS tt "
                "LEFT JOIN \"%s\" AS ft ON (%s).type = 2 AND (%s).id = edge_id "
                "WHERE left_face != 0 OR right_face != 0 ) AS foo ORDER BY type,id",
                pg_info->cursor_name, 
                GV_POINT, pg_info->toposchema_name, pg_info->table_name, pg_info->topogeom_column, pg_info->topogeom_column,
                pg_info->toposchema_name, pg_info->toposchema_name,
                GV_CENTROID, pg_info->toposchema_name, pg_info->table_name, pg_info->topogeom_column, pg_info->topogeom_column,
                pg_info->toposchema_name, pg_info->toposchema_name,
                GV_LINE, pg_info->toposchema_name, pg_info->table_name, pg_info->topogeom_column, pg_info->topogeom_column,
                GV_BOUNDARY, pg_info->toposchema_name, pg_info->table_name, pg_info->topogeom_column, pg_info->topogeom_column);
    }
    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    if (fetch_all)
        sprintf(stmt, "FETCH ALL in %s", pg_info->cursor_name);
    else
        sprintf(stmt, "FETCH %d in %s", CURSOR_PAGE, pg_info->cursor_name);
    G_debug(3, "SQL: %s", stmt);
    pg_info->res = PQexec(pg_info->conn, stmt); /* fetch records from select cursor */
    if (!pg_info->res || PQresultStatus(pg_info->res) != PGRES_TUPLES_OK) {
        error_tuples(pg_info);
        return -1;
    }
    pg_info->next_line = 0;

    return 0;
}

/*!
  \brief Open select cursor for random access (internal use only)

  Fetch number of feature (given by CURSOR_PAGE) starting with
  <em>fid</em>.

  Allocated cursor name should be freed by G_free().

  \param pg_info pointer to Format_info_pg struct
  \param fid feature id to get
  \param type feature type

  \return 0 on success
  \return -1 on failure
*/
int Vect__open_cursor_line_pg(struct Format_info_pg *pg_info, int fid, int type)
{
    char stmt[DB_SQL_MAX];
    
    G_debug(3, "Vect__open_cursor_line_pg(): fid range = %d-%d, type = %d",
            fid, fid + CURSOR_PAGE, type);

    if (Vect__execute_pg(pg_info->conn, "BEGIN") == -1)
        return -1;

    pg_info->cursor_fid = fid;
    G_asprintf(&(pg_info->cursor_name),
               "%s_%s_%d_%p",  pg_info->schema_name, pg_info->table_name, fid, pg_info->conn);
    
    if (!pg_info->toposchema_name) {
        /* simple feature access (geom) */
        sprintf(stmt,
                "DECLARE %s CURSOR FOR SELECT %s FROM \"%s\".\"%s\" "
                "WHERE %s BETWEEN %d AND %d ORDER BY %s", pg_info->cursor_name,
                pg_info->geom_column, pg_info->schema_name, pg_info->table_name,
                pg_info->fid_column, fid, fid + CURSOR_PAGE, pg_info->fid_column);
    }
    else {
        /* topological access */
        if (!(type & (GV_POINTS | GV_LINES))) {
            G_warning(_("Unsupported feature type %d"), type);
            Vect__execute_pg(pg_info->conn, "ROLLBACK");
            return -1;
        }
        
        if (type & GV_POINTS) {
            /* points (geom,containing_face) */
            sprintf(stmt,
                    "DECLARE %s CURSOR FOR SELECT geom,containing_face "
                    " FROM \"%s\".node WHERE node_id BETWEEN %d AND %d ORDER BY node_id",
                    pg_info->cursor_name,
                    pg_info->toposchema_name, fid, fid + CURSOR_PAGE);
        }
        else {
            /* edges (geom,left_face,right_face) */
            sprintf(stmt,
                    "DECLARE %s CURSOR FOR SELECT geom,left_face,right_face "
                    " FROM \"%s\".edge WHERE edge_id BETWEEN %d AND %d ORDER BY edge_id",
                    pg_info->cursor_name,
                    pg_info->toposchema_name, fid, fid + CURSOR_PAGE);
        }
    }
    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }
    pg_info->next_line = 0;

    sprintf(stmt, "FETCH ALL in %s", pg_info->cursor_name);
    pg_info->res = PQexec(pg_info->conn, stmt);
    if (!pg_info->res || PQresultStatus(pg_info->res) != PGRES_TUPLES_OK) {
        error_tuples(pg_info);
        return -1;
    }
    
    return 0;
}

/*!
  \brief Close select cursor

  \param pg_info pointer to Format_info_pg struct

  \return 0 on success
  \return -1 on failure
*/ 
int Vect__close_cursor_pg(struct Format_info_pg *pg_info)
{
    if (pg_info->res) {
        PQclear(pg_info->res);
        pg_info->res = NULL;
    }
    
    if (pg_info->cursor_name) {
        char stmt[DB_SQL_MAX];
        
        sprintf(stmt, "CLOSE %s", pg_info->cursor_name);
        if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
            G_warning(_("Unable to close cursor %s"), pg_info->cursor_name);
            return -1;
        }
        Vect__execute_pg(pg_info->conn, "COMMIT");
        G_free(pg_info->cursor_name);
        pg_info->cursor_name = NULL;
    }
    
    return 0;
}

/*!
  \brief Select feature (internal use only)

  \param pg_info pointer to Format_info_pg struct
  \param fid feature id to get
  \param type feature type

  \return 0 on success
  \return -1 on failure
*/
int Vect__select_line_pg(struct Format_info_pg *pg_info, int fid, int type)
{
    char stmt[DB_SQL_MAX];
    
    if (!pg_info->toposchema_name) {
        /* simple feature access */
        sprintf(stmt,
                "SELECT %s FROM \"%s\".\"%s\" WHERE %s = %d",
                pg_info->geom_column, pg_info->schema_name, pg_info->table_name,
                pg_info->fid_column, fid);
    }
    else {
        /* topological access */
        if (!(type & (GV_POINTS | GV_LINES))) {
            G_warning(_("Unsupported feature type %d"), type);
            return -1;
        }
        
        if (type & GV_POINTS) {
            sprintf(stmt,
                    "SELECT tt.geom,tt.containing_face,ft.fid FROM \"%s\".node AS tt "
                    "LEFT JOIN \"%s\" AS ft ON (%s).type = 1 and (%s).id = edge_id "
                    "WHERE node_id = %d",
                    pg_info->toposchema_name, pg_info->table_name, pg_info->topogeom_column,
                    pg_info->topogeom_column, fid);
        }
        else {
            sprintf(stmt,
                    "SELECT tt.geom,tt.left_face,tt.right_face,ft.fid FROM \"%s\".edge AS tt "
                    "LEFT JOIN \"%s\" AS ft ON (%s).type = 2 and (%s).id = edge_id "
                    "WHERE edge_id = %d",
                    pg_info->toposchema_name, pg_info->table_name, pg_info->topogeom_column,
                    pg_info->topogeom_column, fid);
        }
    }
    G_debug(3, "SQL: %s", stmt);
    
    pg_info->next_line = 0;
    
    pg_info->res = PQexec(pg_info->conn, stmt);
    if (!pg_info->res || PQresultStatus(pg_info->res) != PGRES_TUPLES_OK) {
        error_tuples(pg_info);
        return -1;
    }
    
    return 0;
}

/*!
   \brief Execute SQL statement

   See pg_local_proto.h

   \param conn pointer to PGconn
   \param stmt query

   \return 0 on success
   \return -1 on error
 */
int Vect__execute_pg(PGconn * conn, const char *stmt)
{
    PGresult *result;

    result = NULL;

    G_debug(3, "Vect__execute_pg(): %s", stmt);
    result = PQexec(conn, stmt);
    if (!result || PQresultStatus(result) != PGRES_COMMAND_OK) {
        PQclear(result);
        
        G_warning(_("Execution failed: %s"), PQerrorMessage(conn));
        return -1;
    }

    PQclear(result);
    return 0;
}

/*!
   \brief Execute SQL statement and get value.

   \param conn pointer to PGconn
   \param stmt query

   \return value on success
   \return -1 on error
 */
int Vect__execute_get_value_pg(PGconn *conn, const char *stmt)
{
    int ret;
    PGresult *result;

    result = NULL;

    G_debug(3, "Vect__execute_get_value_pg(): %s", stmt);
    result = PQexec(conn, stmt);
    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK ||
        PQntuples(result) != 1) {
        PQclear(result);

        G_warning(_("Execution failed: %s"), PQerrorMessage(conn));
        return -1;
    }

    ret = atoi(PQgetvalue(result, 0, 0));
    PQclear(result);
    
    return ret;
}

/*!
   \brief Reallocate lines cache
 */
void reallocate_cache(struct Format_info_cache *cache, int num)
{
    int i;

    if (cache->lines_alloc >= num)
        return;

    if (!cache->lines) {
        /* most of features requires only one line cache */
        cache->lines_alloc = 1;
    }
    else {
        cache->lines_alloc += num;
    }

    cache->lines = (struct line_pnts **)G_realloc(cache->lines,
                                                  cache->lines_alloc *
                                                  sizeof(struct line_pnts *));
    cache->lines_types = (int *)G_realloc(cache->lines_types,
                                          cache->lines_alloc * sizeof(int));
    cache->lines_cats = (int *)G_realloc(cache->lines_cats,
                                         cache->lines_alloc * sizeof(int));

    if (cache->lines_alloc > 1) {
        for (i = cache->lines_alloc - num; i < cache->lines_alloc; i++) {
            cache->lines[i] = Vect_new_line_struct();
            cache->lines_types[i] = -1;
            cache->lines_cats[i] = -1;
        }
    }
    else {
        cache->lines[0] = Vect_new_line_struct();
        cache->lines_types[0] = -1;
        cache->lines_cats[0] = -1;
    }
}

void add_fpart(struct feat_parts *fparts, SF_FeatureType ftype,
               int idx, int nlines)
{
    if (!fparts)
        return;

    if (fparts->a_parts == 0 || fparts->n_parts >= fparts->a_parts) {
        if (fparts->a_parts == 0)
            fparts->a_parts = 1;
        else
            fparts->a_parts += fparts->n_parts;

        fparts->ftype = (SF_FeatureType *) G_realloc(fparts->ftype,
                                                     fparts->a_parts *
                                                     sizeof(SF_FeatureType));
        fparts->nlines =
            (int *)G_realloc(fparts->nlines, fparts->a_parts * sizeof(int));
        fparts->idx =
            (int *)G_realloc(fparts->idx, fparts->a_parts * sizeof(int));
    }

    fparts->ftype[fparts->n_parts] = ftype;
    fparts->idx[fparts->n_parts] = idx;
    fparts->nlines[fparts->n_parts] = nlines;

    fparts->n_parts++;
}

/*
  \brief Get centroid
  
  \param pg_info pointer to Format_info_pg
  \param centroid centroid id
  \param[out] line_p output geometry

  \return GV_CENTROID on success
  \return -1 on error
*/
int get_centroid(struct Map_info *Map, int centroid,
                 struct line_pnts *line_p)
{
    int i, found;
    struct bound_box box;
    struct boxlist list;
    struct P_line *Line;
    struct P_topo_c *topo;
        
    Line = Map->plus.Line[centroid];
    topo = (struct P_topo_c *)Line->topo;
    
    /* get area bbox */
    Vect_get_area_box(Map, topo->area, &box);
    /* search in spatial index for centroid with area bbox */
    dig_init_boxlist(&list, TRUE);
    Vect_select_lines_by_box(Map, &box, Line->type, &list);
    
    found = -1;
    for (i = 0; i < list.n_values; i++) {
        if (list.id[i] == centroid) {
            found = i;
            break;
        }
    }
    
    if (found == -1)
        return -1;
    
    if (line_p) {
        Vect_reset_line(line_p);
        Vect_append_point(line_p, list.box[found].E, list.box[found].N, 0.0);
    }
    
    return GV_CENTROID;
}

void error_tuples(struct Format_info_pg *pg_info)
{
    if (pg_info->res) {
        PQclear(pg_info->res);
        pg_info->res = NULL;
    }
    
    Vect__execute_pg(pg_info->conn, "ROLLBACK");
    G_warning(_("Unable to read PostGIS features\n%s"),
              PQresultErrorMessage(pg_info->res));
}
#endif
