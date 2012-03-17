/*!
  \file lib/vector/Vlib/read_pg.c
  
  \brief Vector library - reading features (PostGIS format)
  
  Higher level functions for reading/writing/manipulating vectors.
  
  \todo Currently only points, linestrings and polygons are supported,
  implement also other types
  
  (C) 2011-2012 by the GRASS Development Team
  
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

static int read_next_line_pg(struct Map_info *,
		      struct line_pnts *, struct line_cats *, int);
SF_FeatureType get_feature(struct Format_info_pg *, int);
static unsigned char *hex_to_wkb(const char *, int *);
static int point_from_wkb(const unsigned char *, int, int, int,
			  struct line_pnts *);
static int linestring_from_wkb(const unsigned char *, int, int, int,
			       struct line_pnts *, int);
static int polygon_from_wkb(const unsigned char *, int, int, int,
			    struct Format_info_cache *);
static int geometry_collection_from_wkb(const unsigned char *, int, int, int,
					struct Format_info_cache *,
					struct feat_parts *);
static int error_corrupted_data(const char *);
static int set_initial_query();
static void reallocate_cache(struct Format_info_cache *, int);
static void add_fpart(struct feat_parts *, SF_FeatureType, int, int);
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
			 struct line_pnts *line_p,
			 struct line_cats *line_c)
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
  \brief Read next feature from PostGIS layer on topological level.

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
    
    G_debug(3, "V2_read_next_line_pg()");
    
    if (Map->constraint.region_flag)
	Vect_get_constraint_box(Map, &mbox);
    
    ret = -1;
    while(TRUE) {
	line = Map->next_line;

	if (Map->next_line > Map->plus.n_lines)
	    return -2;

	Line = Map->plus.Line[line];
	if (Line == NULL) {	/* skip dead features */
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

	if (Line->type == GV_CENTROID) {
	    G_debug(4, "Centroid");
	    
	    Map->next_line++;
	    
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
		    Vect_append_point(line_p, list.box[found].E, list.box[found].N, 0.0);
		}
	    }
	    if (line_c != NULL) {
		/* cat = FID and offset = FID for centroid */
		Vect_reset_cats(line_c);
		Vect_cat_set(line_c, 1, (int) Line->offset);
	    }

	    ret = GV_CENTROID;
	}
	else {
	    /* ignore constraints, Map->next_line incremented */
	    ret = read_next_line_pg(Map, line_p, line_c, TRUE);
	    if (ret != Line->type)
		G_fatal_error(_("Unexpected feature type (%s) - should be (%d)"),
			      ret, Line->type);
	}

	if (Map->constraint.region_flag) {
	    /* skip by region */
	    Vect_line_box(line_p, &lbox);
	    if (!Vect_box_overlap(&lbox, &mbox)) {
		continue;
	    }
	}
	
	/* skip by field ignored */
	
	return ret;
    }
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
#endif

    return -1; /* not reached */
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
		    struct line_pnts *line_p, struct line_cats *line_c, off_t offset)
{
#ifdef HAVE_POSTGRES
    long fid;
    int  ipart, type;
    static SF_FeatureType sf_type;
    
    struct Format_info_pg     *pg_info;
    
    pg_info = &(Map->fInfo.pg);
    
    G_debug(3, "V1_read_line_pg(): offset = %lu offset_num = %lu",
	    (long) offset, (long) pg_info->offset.array_num);
    
    if (offset >= pg_info->offset.array_num)
	return -2; /* nothing to read */
    
    if (line_p != NULL)
	Vect_reset_line(line_p);
    if (line_c != NULL)
	Vect_reset_cats(line_c);

    fid = pg_info->offset.array[offset];
    G_debug(4, "  fid = %ld", fid);
    
    /* read feature to cache if necessary */
    if (pg_info->cache.fid != fid) {
	G_debug(4, "read feature (fid = %ld) to cache", fid);
	sf_type = get_feature(pg_info, fid);
	
	if (sf_type == SF_NONE) {
	    G_warning(_("Feature %d without geometry skipped"), fid);
	    return -1;
	}

	if ((int) sf_type < 0) /* -1 || - 2 */
	    return (int) sf_type;
    }
    
    /* get data from cache */
    if (sf_type == SF_POINT || sf_type == SF_LINESTRING)
	ipart = 0;
    else 
	ipart = pg_info->offset.array[offset + 1];
    type  = pg_info->cache.lines_types[ipart];
    G_debug(4, "read feature part: %d -> type = %d",
	    ipart, type);    
	
    if (line_p)
	Vect_append_points(line_p,
			   pg_info->cache.lines[ipart], GV_FORWARD);
    
    if (line_c)
	Vect_cat_set(line_c, 1, (int) fid);
    
    return type;
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
    int line, itype;
    SF_FeatureType sf_type;
    
    struct Format_info_pg    *pg_info;
    struct bound_box mbox, lbox;
    struct line_pnts *iline;

    pg_info = &(Map->fInfo.pg);
    
    if (Map->constraint.region_flag && !ignore_constraints)
	Vect_get_constraint_box(Map, &mbox);
    
    while (TRUE) {
	line = Map->next_line++; /* level 2 only */
	
	/* reset data structures */
	if (line_p != NULL)
	    Vect_reset_line(line_p);
	if (line_c != NULL)
	    Vect_reset_cats(line_c);

	/* read feature to cache if necessary */
	while (pg_info->cache.lines_next == pg_info->cache.lines_num) {
	    /* cache feature -> line_p & line_c */
	    sf_type = get_feature(pg_info, -1);
	    
	    if (sf_type == SF_NONE) {
		G_warning(_("Feature %d without geometry skipped"),
			  line);
		return -1;
	    }

	    if ((int) sf_type < 0) /* -1 || - 2 */
		return (int) sf_type;

	    if (sf_type == SF_UNKNOWN || sf_type == SF_NONE) {
		G_warning(_("Feature without geometry. Skipped."));
		pg_info->cache.lines_next = pg_info->cache.lines_num = 0;
		continue;
	    }
	    
	    G_debug(4, "%d lines read to cache", pg_info->cache.lines_num);
	    
	    /* next to be read from cache */
	    pg_info->cache.lines_next = 0;	
	}
	
	/* get data from cache */
	G_debug(4, "read next cached line %d", pg_info->cache.lines_next);

	itype = pg_info->cache.lines_types[pg_info->cache.lines_next];
	iline = pg_info->cache.lines[pg_info->cache.lines_next];

	/* apply constraints */
	if (Map->constraint.type_flag && !ignore_constraints) {
	    /* skip feature by type */
	    if (!(itype & Map->constraint.type))
		continue;
	}

	if (line_p && Map->constraint.region_flag &&
	    !ignore_constraints) {
	    /* skip feature by region */
	    Vect_line_box(iline, &lbox);
	    
	    if (!Vect_box_overlap(&lbox, &mbox))
		continue;
	}
	
	/* skip feature by field ignored */
	
	if (line_p)
	    Vect_append_points(line_p, iline, GV_FORWARD);
	
	if (line_c)
	    Vect_cat_set(line_c, 1, (int) pg_info->cache.fid);
	
	pg_info->cache.lines_next++;
	G_debug(4, "next line read, type = %d", itype);
	
	return itype;
    }

    return -1; /* not reached */
}

/*!
  \brief Read feature geometry

  Geometry is stored in lines cache.
  
  \param[in,out] pg_info pointer to Format_info_pg struct
  \param fid feature id to be read (-1 for next)
  \param[out] line_c pointer to line_cats structure (or NULL)

  \return simple feature type (SF_POINT, SF_LINESTRING, ...)
  \return -1 on error
*/
SF_FeatureType get_feature(struct Format_info_pg *pg_info, int fid)
{
    char *data;
    char stmt[DB_SQL_MAX];
    SF_FeatureType ftype;

    if (!pg_info->geom_column) {
	G_warning(_("No geometry column defined"));
	return -1;
    }
    if (fid < 1) {
	/* next (read n features) */
	if (!pg_info->res) {
	    if (set_initial_query(pg_info) == -1)
		return -1;
	}
    }
    else {
	if (!pg_info->fid_column) {
	    G_warning(_("Random access not supported. "
			"Primary key not defined."));
	    return -1;
	}
	
	if (execute(pg_info->conn, "BEGIN") == -1)
	    return -1;
	
	sprintf(stmt, "DECLARE %s%p CURSOR FOR SELECT %s FROM %s "
		"WHERE %s = %d",
		pg_info->table_name, pg_info->conn, 
		pg_info->geom_column, 
		pg_info->table_name, pg_info->fid_column, fid);

	if (execute(pg_info->conn, stmt) == -1)
	    return -1;
    
	sprintf(stmt, "FETCH ALL in %s%p", 
		pg_info->table_name, pg_info->conn);
	pg_info->res = PQexec(pg_info->conn, stmt);
	pg_info->next_line = 0;
    }

    if (!pg_info->res || PQresultStatus(pg_info->res) != PGRES_TUPLES_OK) {
	PQclear(pg_info->res);
	pg_info->res = NULL;
	return -1; /* reading failed */
    }
        
    /* do we need to fetch more records ? */
    if (PQntuples(pg_info->res) == CURSOR_PAGE &&
        PQntuples(pg_info->res) == pg_info->next_line) {
	char stmt[DB_SQL_MAX];
	PQclear(pg_info->res);

	sprintf(stmt, "FETCH %d in %s%p", CURSOR_PAGE,
		pg_info->table_name, pg_info->conn);
	pg_info->res = PQexec(pg_info->conn, stmt);
	pg_info->next_line = 0;
    }

    /* out of results ? */
    if (PQntuples(pg_info->res) == pg_info->next_line) {
	if (pg_info->res) {
	    PQclear(pg_info->res);
	    pg_info->res = NULL;
	    
	    sprintf(stmt, "CLOSE %s%p",
		    pg_info->table_name, pg_info->conn);
	    if (execute(pg_info->conn, stmt) == -1) {
		G_warning(_("Unable to close cursor"));
		return -1;
	    }
	    execute(pg_info->conn, "COMMIT");
	}
	return -2;
    }
    data = (char *)PQgetvalue(pg_info->res, pg_info->next_line, 0);
    
    ftype = cache_feature(data, FALSE, &(pg_info->cache), NULL);
    if (fid < 0) {
	pg_info->cache.fid = atoi(PQgetvalue(pg_info->res, pg_info->next_line, 1));
	pg_info->next_line++;
    }
    else {
	pg_info->cache.fid = fid;

	PQclear(pg_info->res);
	pg_info->res = NULL;
	
	sprintf(stmt, "CLOSE %s%p",
		pg_info->table_name, pg_info->conn);
	if (execute(pg_info->conn, stmt) == -1) {
	    G_warning(_("Unable to close cursor"));
	    return -1;
	}

	if (execute(pg_info->conn, "COMMIT") == -1)
	    return -1;
    }
    
    return ftype;
}

/*!
  \brief Convert HEX to WKB data

  This function is based on CPLHexToBinary() from GDAL/OGR library

  \param hex_data HEX data
  \param[out] nbytes number of bytes in output buffer

  \return pointer to WKB data buffer
*/
static unsigned char *hex_to_wkb(const char *hex_data, int *nbytes)
{
    unsigned char *wkb_data;
    unsigned int length, i_src, i_dst;

    i_src = i_dst = 0;
    length = strlen(hex_data);
    wkb_data = G_malloc(length / 2 + 2);
    
    while (hex_data[i_src] != '\0' ) {
        if (hex_data[i_src] >= '0' && hex_data[i_src] <= '9')
            wkb_data[i_dst] = hex_data[i_src] - '0';
        else if (hex_data[i_src] >= 'A' && hex_data[i_src] <= 'F')
            wkb_data[i_dst] = hex_data[i_src] - 'A' + 10;
        else if (hex_data[i_src] >= 'a' && hex_data[i_src] <= 'f')
            wkb_data[i_dst] = hex_data[i_src] - 'a' + 10;
        else 
            break;
	
        wkb_data[i_dst] *= 16;

        i_src++;

        if (hex_data[i_src] >= '0' && hex_data[i_src] <= '9')
            wkb_data[i_dst] += hex_data[i_src] - '0';
        else if(hex_data[i_src] >= 'A' && hex_data[i_src] <= 'F')
            wkb_data[i_dst] += hex_data[i_src] - 'A' + 10;
        else if(hex_data[i_src] >= 'a' && hex_data[i_src] <= 'f')
            wkb_data[i_dst] += hex_data[i_src] - 'a' + 10;
        else
            break;
	
        i_src++;
        i_dst++;
    }
    
    wkb_data[i_dst] = 0;
    *nbytes = i_dst;

    return wkb_data;
}

/*!
  \brief Read geometry from HEX data

  This code is inspired by OGRGeometryFactory::createFromWkb() from
  GDAL/OGR library.

  \param data HEX data
  \param skip_polygon skip polygons (level 1)
  \param[out] cache lines cache
  \param[out] fparts used for building pseudo-topology (or NULL)
  
  \return simple feature type
  \return SF_UNKNOWN on error
*/
SF_FeatureType cache_feature(const char *data, int skip_polygon,
			     struct Format_info_cache *cache,
			     struct feat_parts *fparts)
{
    int ret, byte_order, nbytes, is3D;
    unsigned char *wkb_data;
    unsigned int wkb_flags;
    SF_FeatureType ftype;

    /* reset cache */
    cache->lines_num = 0;
    cache->fid       = -1;
    if (fparts)
	fparts->n_parts = 0;
    
    wkb_flags = 0;
    wkb_data  = hex_to_wkb(data, &nbytes);
    
    if (nbytes < 5) {
	G_free(wkb_data);
	if (nbytes > 0) {
	    G_debug(3, "cache_feature(): invalid geometry");
	    G_warning(_("Invalid WKB content: %d bytes"), nbytes);
	    return SF_UNKNOWN;
	}
	else {
	    G_debug(3, "cache_feature(): no geometry");
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
	G_free(wkb_data);
	return SF_UNKNOWN;
    }

    /* PostGIS EWKB format includes an  SRID, but this won't be       
       understood by OGR, so if the SRID flag is set, we remove the    
       SRID (bytes at offset 5 to 8).                                 
    */
    if (nbytes > 9 &&
        ((byte_order == ENDIAN_BIG && (wkb_data[1] & 0x20)) ||
	 (byte_order == ENDIAN_LITTLE && (wkb_data[4] & 0x20)))) {
        memmove(wkb_data + 5, wkb_data + 9, nbytes -9);
        nbytes -= 4;
        if(byte_order == ENDIAN_BIG)
            wkb_data[1] &= (~0x20);
        else
            wkb_data[4] &= (~0x20);
    }
    
    if (nbytes < 9 && nbytes != -1) {
	G_free(wkb_data);
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
    G_debug(3, "cache_feature(): sf_type = %d", ftype);
   
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
	cache->lines_types[0] = GV_POINT;
	ret = point_from_wkb(wkb_data, nbytes, byte_order,
			     is3D, cache->lines[0]);
	add_fpart(fparts, ftype, 0, 1);
    }
    else if (ftype == SF_LINESTRING) {
	cache->lines_num = 1;
	cache->lines_types[0] = GV_LINE;
	ret = linestring_from_wkb(wkb_data, nbytes, byte_order,
				  is3D, cache->lines[0], FALSE);
	add_fpart(fparts, ftype, 0, 1);
    }
    else if (ftype == SF_POLYGON && !skip_polygon) {
	ret = polygon_from_wkb(wkb_data, nbytes, byte_order,
			       is3D, cache);
	add_fpart(fparts, ftype, 0, 1);
    }
    else if (ftype == SF_MULTIPOINT ||
	     ftype == SF_MULTILINESTRING ||
	     ftype == SF_MULTIPOLYGON ||
	     ftype == SF_GEOMETRYCOLLECTION) {
	ret = geometry_collection_from_wkb(wkb_data, nbytes, byte_order,
					   is3D, cache, fparts);
    }
    else  {
	G_warning(_("Unsupported feature type %d"), ftype);
    }
    
    G_free(wkb_data);
    
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
    if (nbytes < 21 && nbytes != -1 )
	return -1;

    /* get vertex */
    memcpy(&x, wkb_data + 5, 8);
    memcpy(&y, wkb_data + 5 + 8, 8);
    
    if (byte_order == ENDIAN_BIG) {
        SWAPDOUBLE(&x);
        SWAPDOUBLE(&y);
    }

    if (with_z) {
        if (nbytes < 29 && nbytes != -1 )
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
int linestring_from_wkb(const unsigned char *wkb_data, int nbytes, int byte_order,
			int with_z, struct line_pnts *line_p, int is_ring)
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

  \return wkb size
  \return -1 on error
*/
int polygon_from_wkb(const unsigned char *wkb_data, int nbytes, int byte_order,
		     int with_z, struct Format_info_cache *cache)
{
    int nrings, data_offset, i, nsize, isize;
    struct line_pnts *line_i;
    
    if (nbytes < 9 && nbytes != -1)
	return -1;
    
    /* get the ring count */
    memcpy(&nrings, wkb_data + 5, 4);
    if (byte_order == ENDIAN_BIG) {
        nrings = SWAP32(nrings);
    }
    if (nrings < 0) {
        return -1;
    }
    
    /* reallocate space for islands if needed */
    reallocate_cache(cache, nrings);
    cache->lines_num += nrings;
    
    /* each ring has a minimum of 4 bytes (point count) */
    if (nbytes != -1 && nbytes - 9 < nrings * 4) {
        return error_corrupted_data(_("Length of input WKB is too small"));
    }

    data_offset = 9;
    if (nbytes != -1)
        nbytes -= data_offset;
    
    /* get the rings */
    nsize = 9;
    for (i = 0; i < nrings; i++ ) {
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
int geometry_collection_from_wkb(const unsigned char *wkb_data, int nbytes, int byte_order,
				 int with_z, struct Format_info_cache *cache,
				 struct feat_parts *fparts)
{
    int ipart, nparts, data_offset, nsize;
    unsigned char *wkb_subdata;
    SF_FeatureType ftype;
    
    if (nbytes < 9 && nbytes != -1)
       return error_corrupted_data(NULL);
    
    /* get the geometry count */
    memcpy(&nparts, wkb_data + 5, 4 );
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
    cache->lines_next = cache->lines_num = 0;
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
	    nsize = linestring_from_wkb(wkb_subdata, nbytes, byte_order, with_z,
					cache->lines[cache->lines_next],
					FALSE);
	    cache->lines_num++;
	    add_fpart(fparts, ftype, cache->lines_next, 1);
	    cache->lines_next++;
	}
	else if (ftype == SF_POLYGON) {
	    int idx = cache->lines_next;
	    nsize = polygon_from_wkb(wkb_subdata, nbytes, byte_order,
				     with_z, cache);
	    add_fpart(fparts, ftype, idx, cache->lines_num - idx);
	}
	else if (ftype == SF_GEOMETRYCOLLECTION ||
		 ftype == SF_MULTIPOLYGON ||
		 ftype == SF_MULTILINESTRING ||
		 ftype == SF_MULTIPOLYGON) {
	    // geometry_collection_from_wkb();
	}
	else  {
	    G_warning(_("Unsupported feature type %d"), ftype);
	}

        if (nbytes != -1) {
	    nbytes -= nsize;
	}
	
        data_offset += nsize;
    }
    cache->lines_next = 0;
    
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
  \brief Set initial SQL query for sequential access

  \param pg_info pointer to Format_info_pg struct

  \return 0 on success
  \return -1 on error
*/
int set_initial_query(struct Format_info_pg *pg_info)
{
    char stmt[DB_SQL_MAX];
    
    if (execute(pg_info->conn, "BEGIN") == -1)
	return -1;
    
    sprintf(stmt, "DECLARE %s%p CURSOR FOR SELECT %s,%s FROM %s",
	    pg_info->table_name, pg_info->conn, 
	    pg_info->geom_column, pg_info->fid_column,
	    pg_info->table_name);

    if (execute(pg_info->conn, stmt) == -1)
	return -1;
    
    sprintf(stmt, "FETCH %d in %s%p", CURSOR_PAGE,
	    pg_info->table_name, pg_info->conn);
    pg_info->res = PQexec(pg_info->conn, stmt);
    pg_info->next_line = 0;
    
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
int execute(PGconn *conn, const char *stmt)
{
    PGresult *result;

    result = NULL;

    G_debug(3, "execute(): %s", stmt);
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
	cache->lines_alloc += 20;
    }

    cache->lines = (struct line_pnts **) G_realloc(cache->lines,
						   cache->lines_alloc *
						   sizeof(struct line_pnts *));
    cache->lines_types = (int *) G_realloc(cache->lines_types,
					   cache->lines_alloc *
					   sizeof(int));
    
    if (cache->lines_alloc > 1) {
	for (i = cache->lines_alloc - 20; i < cache->lines_alloc; i++) {
	    cache->lines[i] = Vect_new_line_struct();
	    cache->lines_types[i] = -1;
	}
    }
    else {
	cache->lines[0] = Vect_new_line_struct();
	cache->lines_types[0] = -1;
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
	    fparts->a_parts += 20;
	
	fparts->ftype  = (SF_FeatureType *) G_realloc(fparts->ftype,
						      fparts->a_parts * sizeof(SF_FeatureType));
	fparts->nlines = (int *) G_realloc(fparts->nlines,
					   fparts->a_parts * sizeof(int));
	fparts->idx    = (int *) G_realloc(fparts->idx,
					   fparts->a_parts * sizeof(int));
    }
    
    fparts->ftype[fparts->n_parts]  = ftype;
    fparts->idx[fparts->n_parts]    = idx;
    fparts->nlines[fparts->n_parts] = nlines;
    
    fparts->n_parts++;
}
#endif
