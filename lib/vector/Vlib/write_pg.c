/*!
   \file lib/vector/Vlib/write_pg.c

   \brief Vector library - write vector feature (PostGIS format)

   Higher level functions for reading/writing/manipulating vectors.

   Write subroutine inspired by OGR PostgreSQL driver.

   \todo PostGIS version of V2__add_line_to_topo_nat()
   \todo OGR version of V2__delete_area_cats_from_cidx_nat()
   \todo function to delete corresponding entry in fidx
   \todo OGR version of V2__add_area_cats_to_cidx_nat
   \todo OGR version of V2__add_line_to_topo_nat

   (C) 2012 by Martin Landa, and the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Martin Landa <landa.martin gmail.com>
 */

#include <string.h>

#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

#ifdef HAVE_POSTGRES
#include "pg_local_proto.h"

#define WKBSRIDFLAG 0x20000000

/*! Use SQL statements from PostGIS Topology extension (this options
  is quite slow. By default are used simple SQL statements (INSERT, UPDATE)
*/
#define USE_TOPO_STMT 0

static off_t write_line_sf(struct Map_info *, int,
                           const struct line_pnts **, int,
                           const struct line_cats *);
static off_t write_line_tp(struct Map_info *, int, int,
                           const struct line_pnts *,
                           const struct line_cats *);
static char *binary_to_hex(int, const unsigned char *);
static unsigned char *point_to_wkb(int, const struct line_pnts *, int, int *);
static unsigned char *linestring_to_wkb(int, const struct line_pnts *,
                                        int, int *);
static unsigned char *polygon_to_wkb(int, const struct line_pnts **, int,
                                     int, int *);
static int write_feature(struct Map_info *, int, int,
                         const struct line_pnts **, int, int,
                         int, const struct field_info *);
static char *build_insert_stmt(const struct Format_info_pg *, const char *,
                               int, const struct field_info *);
static int insert_topo_element(struct Map_info *, int, int, const char *);
static int update_next_edge(struct Map_info*, int, int);
static int delete_face(const struct Map_info *, int);
static int update_topo_edge(struct Map_info *, int);
static int update_topo_face(struct Map_info *, int);
#endif

/*!
   \brief Writes feature on level 1 (PostGIS interface)

   Note:
   - centroids are not supported in PostGIS, pseudotopo holds virtual
   centroids
   - boundaries are not supported in PostGIS, pseudotopo treats polygons
   as boundaries

   \param Map pointer to Map_info structure
   \param type feature type (GV_POINT, GV_LINE, ...)
   \param points pointer to line_pnts structure (feature geometry) 
   \param cats pointer to line_cats structure (feature categories)

   \return feature offset into file
   \return -1 on error
 */
off_t V1_write_line_pg(struct Map_info *Map, int type,
                       const struct line_pnts *points,
                       const struct line_cats *cats)
{
#ifdef HAVE_POSTGRES
    struct Format_info_pg *pg_info;

    pg_info = &(Map->fInfo.pg);
    
    if (pg_info->feature_type == SF_UNKNOWN) {
        /* create PostGIS table if doesn't exist */
        if (V2_open_new_pg(Map, type) < 0)
            return -1;
    }

    if (!pg_info->toposchema_name) { /* simple features */
        return write_line_sf(Map, type, &points, 1, cats);
    }
    return write_line_tp(Map, type, FALSE, points, cats);
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

/*!
   \brief Writes feature on level 2 (PostGIS interface)

   \param Map pointer to Map_info structure
   \param type feature type (GV_POINT, GV_LINE, ...)
   \param points pointer to line_pnts structure (feature geometry) 
   \param cats pointer to line_cats structure (feature categories)

   \return feature offset into file
   \return -1 on error
 */
off_t V2_write_line_pg(struct Map_info *Map, int type,
                       const struct line_pnts *points,
                       const struct line_cats *cats)
{
#ifdef HAVE_POSTGRES
    struct Format_info_pg *pg_info;

    pg_info = &(Map->fInfo.pg);
    
    if (!pg_info->toposchema_name) { /* pseudo-topology */
        return V2_write_line_sfa(Map, type, points, cats);
    }
    else {                          /* PostGIS topology */
        if (Map->plus.built < GV_BUILD_BASE)
            Map->plus.built = GV_BUILD_BASE; /* update build level */
        return write_line_tp(Map, type, FALSE, points, cats);
    }
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

/*!
   \brief Rewrites feature at the given offset (level 1) (PostGIS interface)

   \param Map pointer to Map_info structure
   \param offset feature offset
   \param type feature type (GV_POINT, GV_LINE, ...)
   \param points feature geometry
   \param cats feature categories

   \return feature offset (rewriten feature)
   \return -1 on error
 */
off_t V1_rewrite_line_pg(struct Map_info * Map,
                         int line, int type, off_t offset,
                         const struct line_pnts * points,
                         const struct line_cats * cats)
{
    G_debug(3, "V1_rewrite_line_pg(): line=%d type=%d offset=%lu",
            line, type, offset);
#ifdef HAVE_POSTGRES
    if (type != V1_read_line_pg(Map, NULL, NULL, offset)) {
        G_warning(_("Unable to rewrite feature (incompatible feature types)"));
        return -1;
    }

    /* delete old */
    V1_delete_line_pg(Map, offset);

    return V1_write_line_pg(Map, type, points, cats);
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

/*!
   \brief Deletes feature at the given offset (level 1)

   \param Map pointer Map_info structure
   \param offset feature offset

   \return  0 on success
   \return -1 on error
 */
int V1_delete_line_pg(struct Map_info *Map, off_t offset)
{
#ifdef HAVE_POSTGRES
    long fid;
    char stmt[DB_SQL_MAX];

    struct Format_info_pg *pg_info;

    pg_info = &(Map->fInfo.pg);

    if (!pg_info->conn || !pg_info->table_name) {
        G_warning(_("No connection defined"));
        return -1;
    }

    if (offset >= pg_info->offset.array_num) {
        G_warning(_("Invalid offset (%d)"), offset);
        return -1;
    }

    fid = pg_info->offset.array[offset];

    G_debug(3, "V1_delete_line_pg(), offset = %lu -> fid = %ld",
            (unsigned long)offset, fid);

    if (!pg_info->inTransaction) {
        /* start transaction */
        pg_info->inTransaction = TRUE;
        if (Vect__execute_pg(pg_info->conn, "BEGIN") == -1)
            return -1;
    }

    sprintf(stmt, "DELETE FROM %s WHERE %s = %ld",
            pg_info->table_name, pg_info->fid_column, fid);
    G_debug(2, "SQL: %s", stmt);

    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
        G_warning(_("Unable to delete feature"));
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

/*!
   \brief Writes node on level 2 (PostGIS Topology interface) - internal use only

   \param Map pointer to Map_info structure
   \param points pointer to line_pnts structure
   
   \return 0 on success
   \return -1 on error
*/
off_t V2__write_node_pg(struct Map_info *Map, const struct line_pnts *points)
{
#ifdef HAVE_POSTGRES
    struct Format_info_pg *pg_info;

    pg_info = &(Map->fInfo.pg);
    
    if (!pg_info->toposchema_name)
        return -1; /* PostGIS Topology required */
    
    return write_line_tp(Map, GV_POINT, TRUE, points, NULL);
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

/*!
   \brief Writes area on level 2 (PostGIS Simple Features interface) - internal use only

   \param Map pointer to Map_info structure
   \param type feature type (GV_POINT, GV_LINE, ...)
   \param points pointer to line_pnts structure (boundary geometry) 
   \param cats pointer to line_cats structure (feature categories)
   \param ipoints pointer to line_pnts structure (isles geometry) 
   \param nisles number of isles
   
   \return feature offset into file
   \return -1 on error
*/
off_t V2__write_area_pg(struct Map_info *Map, 
                        const struct line_pnts *bpoints,
                        const struct line_cats *cats,
                        const struct line_pnts **ipoints, int nisles)
{
#ifdef HAVE_POSTGRES
    int i;
    off_t ret;
    const struct line_pnts **points;

    if (nisles > 0) {
        points = (const struct line_pnts **) G_calloc(nisles + 1, sizeof(struct line_pnts *));
        points[0] = bpoints;
        for (i = 0; i < nisles; i++)
            points[i + 1] = ipoints[i];
    }
    else {
        points = &bpoints;
    }
    
    ret = write_line_sf(Map, GV_BOUNDARY, points, nisles + 1, cats);

    if (nisles > 0)
        G_free(points);

    return ret;
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

#ifdef HAVE_POSTGRES
/*!
  \brief Write vector features as PostGIS simple feature element
  
  \return 0 on success
  \return -1 on error
*/
off_t write_line_sf(struct Map_info *Map, int type,
                    const struct line_pnts **points, int nparts,
                    const struct line_cats *cats)
{
    int cat;
    off_t offset;

    SF_FeatureType sf_type;

    struct field_info *Fi;
    struct Format_info_pg *pg_info;
    struct Format_info_offset *offset_info;

    pg_info = &(Map->fInfo.pg);
    offset_info = &(pg_info->offset);

    /* check required PG settings */
    if (!pg_info->conn) {
        G_warning(_("No connection defined"));
        return -1;
    }
    if (!pg_info->table_name) {
        G_warning(_("PostGIS feature table not defined"));
        return -1;
    }

    /* create PostGIS table if doesn't exist */
    if (pg_info->feature_type == SF_UNKNOWN) {
        if (V2_open_new_pg(Map, type) < 0)
            return -1;
    }
    
    Fi = NULL; /* no attributes to be written */
    cat = -1;
    if (cats->n_cats > 0 && Vect_get_num_dblinks(Map) > 0) {
        /* check for attributes */
        Fi = Vect_get_dblink(Map, 0);
        if (Fi) {
            if (!Vect_cat_get(cats, Fi->number, &cat))
                G_warning(_("No category defined for layer %d"), Fi->number);
            if (cats->n_cats > 1) {
                G_warning(_("Feature has more categories, using "
                            "category %d (from layer %d)"),
                          cat, cats->field[0]);
            }
        }
    }

    sf_type = pg_info->feature_type;

    /* determine matching PostGIS feature geometry type */
    if (type & (GV_POINT | GV_KERNEL)) {
        if (sf_type != SF_POINT && sf_type != SF_POINT25D) {
            G_warning(_("Feature is not a point. Skipping."));
            return -1;
        }
    }
    else if (type & GV_LINE) {
        if (sf_type != SF_LINESTRING && sf_type != SF_LINESTRING25D) {
            G_warning(_("Feature is not a line. Skipping."));
            return -1;
        }
    }
    else if (type & GV_BOUNDARY || type & GV_CENTROID) {
        if (sf_type != SF_POLYGON) {
            G_warning(_("Feature is not a polygon. Skipping."));
            return -1;
        }
    }
    else if (type & GV_FACE) {
        if (sf_type != SF_POLYGON25D) {
            G_warning(_("Feature is not a face. Skipping."));
            return -1;
        }
    }
    else {
        G_warning(_("Unsupported feature type %d"), type);
        return -1;
    }
    
    G_debug(3, "write_line_sf(): type = %d n_points = %d cat = %d",
            type, points[0]->n_points, cat);

    if (sf_type == SF_POLYGON || sf_type == SF_POLYGON25D) {
        /* skip this check when writing PostGIS topology */
        int part, npoints;

        for (part = 0; part < nparts; part++) { 
            npoints = points[part]->n_points - 1;
            if (points[part]->x[0] != points[part]->x[npoints] ||
                points[part]->y[0] != points[part]->y[npoints] ||
                points[part]->z[0] != points[part]->z[npoints]) {
                G_warning(_("Boundary is not closed. Skipping."));
                return -1;
            }
        }
    }

    /* write feature's geometry and fid */
    if (-1 == write_feature(Map, -1, type, points, nparts,
                            Vect_is_3d(Map) ? WITH_Z : WITHOUT_Z, cat, Fi)) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    /* update offset array */
    if (offset_info->array_num >= offset_info->array_alloc) {
        offset_info->array_alloc += 1000;
        offset_info->array = (int *)G_realloc(offset_info->array,
                                              offset_info->array_alloc *
                                              sizeof(int));
    }
    offset = offset_info->array_num;

    offset_info->array[offset_info->array_num++] = cat;
    if (sf_type == SF_POLYGON || sf_type == SF_POLYGON25D) {
        /* register first part in offset array */
        offset_info->array[offset_info->array_num++] = 0;
    }
    G_debug(3, "write_line_sf(): -> offset = %lu offset_num = %d cat = %d",
            (unsigned long)offset, offset_info->array_num, cat);

    return offset;
}

/*! 
  \brief Write vector feature in PostGIS topology schema and
  updates internal topology structures

  \param Map vector map
  \param type feature type to be written
  \param points feature geometry
  \param is_node TRUE for nodes (written as points)
  
  \return 0 on success
  \return -1 on error
*/
off_t write_line_tp(struct Map_info *Map, int type, int is_node,
                    const struct line_pnts *points,
                    const struct line_cats *cats)
{
    int line, cat;

    struct field_info *Fi;
    struct Format_info_pg *pg_info;
    struct Plus_head *plus;
    
    pg_info = &(Map->fInfo.pg);
    plus = &(Map->plus);
    
    /* check type for nodes */
    if (is_node && type != GV_POINT) {
        G_warning(_("Invalid type (%d) for nodes"), type);
        return -1;
    }

    /* check required PG settings */
    if (!pg_info->conn) {
        G_warning(_("No connection defined"));
        return -1;
    }
    if (!pg_info->table_name) {
        G_warning(_("PostGIS feature table not defined"));
        return -1;
    }
    if (!pg_info->toposchema_name) {
        G_warning(_("PostGIS topology schema not defined"));
        return -1;
    }
    
    /* create PostGIS table if doesn't exist */
    if (pg_info->feature_type == SF_UNKNOWN) {
        if (V2_open_new_pg(Map, type) < 0)
            return -1;
    }
    
    G_debug(3, "write_line_pg(): type = %d n_points = %d",
            type, points->n_points);

    line = -1; /* used only for topological access (lines, boundaries, and centroids) */
    
    Fi = NULL; /* no attributes to be written */
    cat = -1;
    if (cats && cats->n_cats > 0) {
        if (Vect_get_num_dblinks(Map) > 0) {
            /* check for attributes */
            Fi = Vect_get_dblink(Map, 0);
            if (Fi) {
                if (!Vect_cat_get(cats, Fi->number, &cat))
                    G_warning(_("No category defined for layer %d"), Fi->number);
                if (cats->n_cats > 1) {
                    G_warning(_("Feature has more categories, using "
                                "category %d (from layer %d)"),
                              cat, cats->field[0]);
                }
            }
        }
        /* assume layer=1 */
        Vect_cat_get(cats, 1, &cat);
    }

    /* update GRASS-like topology before writing feature */
    if (is_node) {
        dig_add_node(plus, points->x[0], points->y[0], points->z[0]);
    }
    else {
        off_t offset;
        struct bound_box box;
        
        dig_line_box(points, &box);
        /* better is probably to check nextval directly */
        if (type & GV_POINTS) {
            offset = Vect_get_num_primitives(Map, GV_POINTS) + 1; /* next */
            offset += Vect_get_num_nodes(Map); /* nodes are also stored in 'node' table */
        }
        else { /* LINES */
            offset = Vect_get_num_primitives(Map, GV_LINES) + 1; /* next */
        }
        line = dig_add_line(plus, type, points, &box, offset);
        G_debug(3, "  line added to topo with id = %d", line);
        
        if (line == 1)
            Vect_box_copy(&(plus->box), &box);
        else
            Vect_box_extend(&(plus->box), &box);
    }
    
    /* write new feature to PostGIS
       - feature table for simple features
       - feature table and topo schema for topological access 
    */
    if (-1 == write_feature(Map, line, type, &points, 1,
                            Vect_is_3d(Map) ? WITH_Z : WITHOUT_Z,
                            cat, Fi)) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    return 0;
}

/*!
   \brief Binary data to HEX

   Allocated buffer should be freed by G_free().

   \param nbytes number of bytes to allocate
   \param wkb_data WKB data

   \return allocated buffer with HEX data
 */
char *binary_to_hex(int nbytes, const unsigned char *wkb_data)
{
    char *hex_data;
    int i, nlow, nhigh;
    static const char ach_hex[] = "0123456789ABCDEF";

    hex_data = (char *)G_malloc(nbytes * 2 + 1);
    hex_data[nbytes * 2] = '\0';

    for (i = 0; i < nbytes; i++) {
        nlow = wkb_data[i] & 0x0f;
        nhigh = (wkb_data[i] & 0xf0) >> 4;

        hex_data[i * 2] = ach_hex[nhigh];
        hex_data[i * 2 + 1] = ach_hex[nlow];
    }

    return hex_data;
}

/*!
   \bried Write point into WKB buffer

   See OGRPoint::exportToWkb from GDAL/OGR library

   \param byte_order byte order (ENDIAN_LITTLE or BIG_ENDIAN)
   \param points feature geometry
   \param with_z WITH_Z for 3D data
   \param[out] nsize buffer size

   \return allocated WKB buffer
   \return NULL on error
 */
unsigned char *point_to_wkb(int byte_order,
                            const struct line_pnts *points, int with_z,
                            int *nsize)
{
    unsigned char *wkb_data;
    unsigned int sf_type;

    if (points->n_points != 1)
        return NULL;

    /* allocate buffer */
    *nsize = with_z ? 29 : 21;
    wkb_data = G_malloc(*nsize);
    G_zero(wkb_data, *nsize);

    G_debug(5, "\t->point size=%d (with_z = %d)", *nsize, with_z);

    /* set the byte order */
    if (byte_order == ENDIAN_LITTLE)
        wkb_data[0] = '\001';
    else
        wkb_data[0] = '\000';

    /* set the geometry feature type */
    sf_type = with_z ? SF_POINT25D : SF_POINT;

    if (byte_order == ENDIAN_LITTLE)
        sf_type = LSBWORD32(sf_type);
    else
        sf_type = MSBWORD32(sf_type);
    memcpy(wkb_data + 1, &sf_type, 4);

    /* copy in the raw data */
    memcpy(wkb_data + 5, &(points->x[0]), 8);
    memcpy(wkb_data + 5 + 8, &(points->y[0]), 8);

    if (with_z) {
        memcpy(wkb_data + 5 + 16, &(points->z[0]), 8);
    }

    /* swap if needed */
    if (byte_order == ENDIAN_BIG) {
        SWAPDOUBLE(wkb_data + 5);
        SWAPDOUBLE(wkb_data + 5 + 8);

        if (with_z)
            SWAPDOUBLE(wkb_data + 5 + 16);
    }

    return wkb_data;
}

/*!
   \bried Write linestring into WKB buffer

   See OGRLineString::exportToWkb from GDAL/OGR library

   \param byte_order byte order (ENDIAN_LITTLE or ENDIAN_BIG)
   \param points feature geometry
   \param with_z WITH_Z for 3D data
   \param[out] nsize buffer size

   \return allocated WKB buffer
   \return NULL on error
 */
unsigned char *linestring_to_wkb(int byte_order,
                                 const struct line_pnts *points, int with_z,
                                 int *nsize)
{
    int i, point_size;
    unsigned char *wkb_data;
    unsigned int sf_type;

    if (points->n_points < 1)
        return NULL;

    /* allocate buffer */
    point_size = 8 * (with_z ? 3 : 2);
    *nsize = 5 + 4 + points->n_points * point_size;
    wkb_data = G_malloc(*nsize);
    G_zero(wkb_data, *nsize);

    G_debug(5, "\t->linestring size=%d (with_z = %d)", *nsize, with_z);

    /* set the byte order */
    if (byte_order == ENDIAN_LITTLE)
        wkb_data[0] = '\001';
    else
        wkb_data[0] = '\000';

    /* set the geometry feature type */
    sf_type = with_z ? SF_LINESTRING25D : SF_LINESTRING;

    if (byte_order == ENDIAN_LITTLE)
        sf_type = LSBWORD32(sf_type);
    else
        sf_type = MSBWORD32(sf_type);
    memcpy(wkb_data + 1, &sf_type, 4);

    /* copy in the data count */
    memcpy(wkb_data + 5, &(points->n_points), 4);

    /* copy in the raw data */
    for (i = 0; i < points->n_points; i++) {
        memcpy(wkb_data + 9 + point_size * i, &(points->x[i]), 8);
        memcpy(wkb_data + 9 + 8 + point_size * i, &(points->y[i]), 8);

        if (with_z) {
            memcpy(wkb_data + 9 + 16 + point_size * i, &(points->z[i]), 8);
        }
    }

    /* swap if needed */
    if (byte_order == ENDIAN_BIG) {
        int npoints, nitems;

        npoints = SWAP32(points->n_points);
        memcpy(wkb_data + 5, &npoints, 4);

        nitems = (with_z ? 3 : 2) * points->n_points;
        for (i = 0; i < nitems; i++) {
            SWAPDOUBLE(wkb_data + 9 + 4 + 8 * i);
        }
    }

    return wkb_data;
}

/*!
   \bried Write polygon into WKB buffer

   See OGRPolygon::exportToWkb from GDAL/OGR library

   \param byte_order byte order (ENDIAN_LITTLE or ENDIAN_BIG)
   \param ipoints list of ring geometries (first is outer ring)
   \param nrings number of rings
   \param with_z WITH_Z for 3D data
   \param[out] nsize buffer size

   \return allocated WKB buffer
   \return NULL on error
 */
unsigned char *polygon_to_wkb(int byte_order,
                              const struct line_pnts** points, int nrings,
                              int with_z, int *nsize)
{
    int i, ring, point_size, offset;
    unsigned char *wkb_data;
    unsigned int sf_type;

    /* check data validity */
    if (nrings < 1)
        return NULL;
    for (ring = 0; ring < nrings; ring++) {
        if (points[ring]->n_points < 3)
            return NULL;
    }

    /* allocate buffer */
    point_size = 8 * (with_z ? 3 : 2);
    *nsize = 9;
    for (ring = 0; ring < nrings; ring++)
        *nsize += 4 + point_size * points[ring]->n_points;
    wkb_data = G_malloc(*nsize);
    G_zero(wkb_data, *nsize);

    G_debug(5, "\t->polygon size=%d (with_z = %d)", *nsize, with_z);

    /* set the byte order */
    if (byte_order == ENDIAN_LITTLE)
        wkb_data[0] = '\001';
    else
        wkb_data[0] = '\000';

    /* set the geometry feature type */
    sf_type = with_z ? SF_POLYGON25D : SF_POLYGON;

    if (byte_order == ENDIAN_LITTLE)
        sf_type = LSBWORD32(sf_type);
    else
        sf_type = MSBWORD32(sf_type);
    memcpy(wkb_data + 1, &sf_type, 4);

    /* copy in the raw data */
    if (byte_order == ENDIAN_BIG) {
        int ncount;

        ncount = SWAP32(nrings);
        memcpy(wkb_data + 5, &ncount, 4);
    }
    else {
        memcpy(wkb_data + 5, &nrings, 4);
    }

    /* serialize rings */
    offset = 9;
    for (ring = 0; ring < nrings; ring++) {
        memcpy(wkb_data + offset, &(points[ring]->n_points), 4);
        for (i = 0; i < points[ring]->n_points; i++) {
            memcpy(wkb_data + offset +
                   4 + point_size * i, &(points[ring]->x[i]), 8);
            memcpy(wkb_data + offset +
                   4 + 8 + point_size * i, &(points[ring]->y[i]), 8);
            
            if (with_z) {
                memcpy(wkb_data + offset +
                       4 + 16 + point_size * i, &(points[ring]->z[i]), 8);
            }
        }
        
        offset += 4 + point_size * points[ring]->n_points;
        
        /* swap if needed */
        if (byte_order == ENDIAN_BIG) {
            int npoints, nitems;
            
            npoints = SWAP32(points[ring]->n_points);
            memcpy(wkb_data + 5, &npoints, 4);
            
            nitems = (with_z ? 3 : 2) * points[ring]->n_points;
            for (i = 0; i < nitems; i++) {
                SWAPDOUBLE(wkb_data + offset + 4 + 8 * i);
            }
        }
    }
    
    return wkb_data;
}

/*!
   \brief Insert feature into table

   \param Map pointer to Map_info structure
   \param line feature id (topo access only)
   \param type feature type (GV_POINT, GV_LINE, ...)
   \param points pointer to line_pnts struct
   \param nparts number of parts (rings for polygon)
   \param with_z WITH_Z for 3D data
   \param cat category number (-1 for no category)
   \param Fi pointer to field_info (attributes to copy, NULL for no attributes)

   \return -1 on error
   \retirn 0 on success
 */
int write_feature(struct Map_info *Map, int line, int type,
                  const struct line_pnts **points, int nparts, int with_z,
                  int cat, const struct field_info *Fi)
{
    int byte_order, nbytes, nsize;
    unsigned int sf_type;
    unsigned char *wkb_data;
    char *stmt, *text_data, *text_data_p, *hex_data;

    struct Format_info_pg *pg_info;
    
    pg_info = &(Map->fInfo.pg);
    
    if (with_z && pg_info->coor_dim != 3) {
        G_warning(_("Trying to insert 3D data into feature table "
                    "which store 2D data only"));
        return -1;
    }
    if (!with_z && pg_info->coor_dim != 2) {
        G_warning(_("Trying to insert 2D data into feature table "
                    "which store 3D data only"));
        return -1;
    }

    byte_order = dig__byte_order_out();

    /* get wkb data */
    nbytes = -1;
    wkb_data = NULL;
    if (type == GV_POINT || type == GV_CENTROID)
        wkb_data = point_to_wkb(byte_order, points[0], with_z, &nbytes);
    else if (type == GV_LINE)
        wkb_data = linestring_to_wkb(byte_order, points[0], with_z, &nbytes);
    else if (type == GV_BOUNDARY) {
        if (!pg_info->toposchema_name) {
            /* PostGIS simple feature access */
            wkb_data = polygon_to_wkb(byte_order, points, nparts,
                                      with_z, &nbytes);
        }
        else {
            /* PostGIS topology access */
            wkb_data = linestring_to_wkb(byte_order, points[0], with_z, &nbytes);
        }
    }
    
    if (!wkb_data || nbytes < 1) {
        G_warning(_("Unsupported feature type %d"), type);
        return -1;
    }

    /* When converting to hex, each byte takes 2 hex characters. In
       addition we add in 8 characters to represent the SRID integer
       in hex, and one for a null terminator */
    nsize = nbytes * 2 + 8 + 1;
    text_data = text_data_p = (char *)G_malloc(nsize);

    /* convert the 1st byte, which is the endianess flag, to hex */
    hex_data = binary_to_hex(1, wkb_data);
    strcpy(text_data_p, hex_data);
    G_free(hex_data);
    text_data_p += 2;

    /* get the geom type which is bytes 2 through 5 */
    memcpy(&sf_type, wkb_data + 1, 4);

    /* add the SRID flag if an SRID is provided */
    if (pg_info->srid > 0) {
        unsigned int srs_flag;

        /* change the flag to little endianess */
        srs_flag = LSBWORD32(WKBSRIDFLAG);
        /* apply the flag */
        sf_type = sf_type | srs_flag;
    }

    /* write the geom type which is 4 bytes */
    hex_data = binary_to_hex(4, (unsigned char *)&sf_type);
    strcpy(text_data_p, hex_data);
    G_free(hex_data);
    text_data_p += 8;

    /* include SRID if provided */
    if (pg_info->srid > 0) {
        unsigned int srs_id;

        /* force the srsid to little endianess */
        srs_id = LSBWORD32(pg_info->srid);
        hex_data = binary_to_hex(sizeof(srs_id), (unsigned char *)&srs_id);
        strcpy(text_data_p, hex_data);
        G_free(hex_data);
        text_data_p += 8;
    }

    /* copy the rest of the data over - subtract 5 since we already
       copied 5 bytes above */
    hex_data = binary_to_hex(nbytes - 5, wkb_data + 5);
    strcpy(text_data_p, hex_data);
    G_free(hex_data);

    /* build INSERT statement
       simple feature geometry + attributes
    */
    stmt = build_insert_stmt(pg_info, text_data, cat, Fi);
    G_debug(2, "SQL: %s", stmt);

    if (!pg_info->inTransaction) {
        /* start transaction */
        pg_info->inTransaction = TRUE;
        if (Vect__execute_pg(pg_info->conn, "BEGIN") == -1)
            return -1;
    }

    /* stmt can NULL when writing PostGIS topology with no attributes
     * attached */
    if (stmt && Vect__execute_pg(pg_info->conn, stmt) == -1) {
        /* rollback transaction */
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }
    G_free(stmt);
    
    /* write feature in PostGIS topology schema if enabled */
    if (pg_info->toposchema_name) {
        /* insert feature into topology schema (node or edge) */
        if (insert_topo_element(Map, line, type, text_data) != 0) {
            G_warning(_("Unable to insert topological element into PostGIS Topology schema"));
            return -1;
        }
        
        /* update GRASS-like topo */
        if (line > 0) /* skip nodes */
            V2__add_line_to_topo_nat(Map, line, points[0], NULL, /* TODO: cats */
                                     delete_face);
        
        /* update PostGIS-line topo */
        if (type & GV_LINES)
            update_topo_edge(Map, line);
        if (type == GV_BOUNDARY)
            update_topo_face(Map, line);
    }

    G_free(wkb_data);
    G_free(text_data);
    
    return 0;
}

/*!
   \brief Build INSERT statement to add new feature to the feature
   table

   Note: Allocated string should be freed.
   
   \param pg_info pointer to Format_info_pg structure
   \param geom_data geometry data
   \param cat category number (or -1 for no category)
   \param Fi pointer to field_info structure (NULL for no attributes)

   \return allocated string with INSERT statement
 */
char *build_insert_stmt(const struct Format_info_pg *pg_info,
                        const char *geom_data,
                        int cat, const struct field_info *Fi)
{
    char *stmt, buf[DB_SQL_MAX];

    stmt = NULL;
    if (Fi && cat > -1) { /* write attributes (simple features and topology elements) */
        int col, ncol, more;
        int sqltype, ctype, is_fid;
        char buf_val[DB_SQL_MAX], buf_tmp[DB_SQL_MAX];

        const char *colname;

        dbString dbstmt;
        dbCursor cursor;
        dbTable *table;
        dbColumn *column;
        dbValue *value;

        db_init_string(&dbstmt);
        buf_val[0] = '\0';

        /* read & set attributes */
        sprintf(buf, "SELECT * FROM %s WHERE %s = %d", Fi->table, Fi->key,
                cat);
        G_debug(4, "SQL: %s", buf);
        db_set_string(&dbstmt, buf);

        /* prepare INSERT statement */
        sprintf(buf, "INSERT INTO \"%s\".\"%s\" (",
                pg_info->schema_name, pg_info->table_name);
        
        /* select data */
        if (db_open_select_cursor(pg_info->dbdriver, &dbstmt,
                                  &cursor, DB_SEQUENTIAL) != DB_OK) {
            G_warning(_("Unable to select attributes for category %d"), cat);
        }
        else {
            if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK) {
                G_warning(_("Unable to fetch data from table <%s>"),
                          Fi->table);
            }

            if (!more) {
                G_warning(_("No database record for category %d, "
                            "no attributes will be written"), cat);
            }
            else {
                table = db_get_cursor_table(&cursor);
                ncol = db_get_table_number_of_columns(table);

                for (col = 0; col < ncol; col++) {
                    column = db_get_table_column(table, col);
                    colname = db_get_column_name(column);

		    /* -> values */
                    value = db_get_column_value(column);
                    /* for debug only */
                    db_convert_column_value_to_string(column, &dbstmt);
                    G_debug(2, "col %d : val = %s", col,
                            db_get_string(&dbstmt));

                    sqltype = db_get_column_sqltype(column);
                    ctype = db_sqltype_to_Ctype(sqltype);

		    is_fid = strcmp(pg_info->fid_column, colname) == 0;
		    
		    /* check fid column (must be integer) */
                    if (is_fid == TRUE &&
			ctype != DB_C_TYPE_INT) {
			G_warning(_("FID column must be integer, column <%s> ignored!"),
				  colname);
                        continue;
		    }

                    /* -> columns */
                    sprintf(buf_tmp, "%s", colname);
                    strcat(buf, buf_tmp);
                    if (col < ncol - 1)
                        strcat(buf, ",");
		    
                    /* prevent writing NULL values */
                    if (!db_test_value_isnull(value)) {
                        switch (ctype) {
                        case DB_C_TYPE_INT:
                            sprintf(buf_tmp, "%d", db_get_value_int(value));
                            break;
                        case DB_C_TYPE_DOUBLE:
                            sprintf(buf_tmp, "%.14f",
                                    db_get_value_double(value));
                            break;
                        case DB_C_TYPE_STRING: {
                            char *value_tmp;
                            value_tmp = G_str_replace(db_get_value_string(value), "'", "''");
                            sprintf(buf_tmp, "'%s'", value_tmp);
                            G_free(value_tmp);
                            break;
                        }
                        case DB_C_TYPE_DATETIME:
                            db_convert_column_value_to_string(column,
                                                              &dbstmt);
                            sprintf(buf_tmp, "%s", db_get_string(&dbstmt));
                            break;
                        default:
                            G_warning(_("Unsupported column type %d"), ctype);
                            sprintf(buf_tmp, "NULL");
                            break;
                        }
                    }
                    else {
			if (is_fid == TRUE)
			    G_warning(_("Invalid value for FID column: NULL"));
                        sprintf(buf_tmp, "NULL");
                    }
                    strcat(buf_val, buf_tmp);
                    if (col < ncol - 1)
                        strcat(buf_val, ",");
                }
                
                if (!pg_info->toposchema_name) {
                    /* simple feature access */
                    G_asprintf(&stmt, "%s,%s) VALUES (%s,'%s'::GEOMETRY)",
                               buf, pg_info->geom_column, buf_val, geom_data);
                }
                else {
                    /* PostGIS topology access, write geometry in
                     * topology schema, skip geometry at this point */
		    if (buf[strlen(buf)-1] == ',') { /* last column skipped */
			buf[strlen(buf)-1] = '\0';
			buf_val[strlen(buf_val)-1] = '\0';
		    }
                    G_asprintf(&stmt, "%s) VALUES (%s)",
                               buf, buf_val);
                }
            }
        }
    }
    else {
        /* no attributes */
        if (!pg_info->toposchema_name) {
            /* no attributes (simple features access) */
            G_asprintf(&stmt, "INSERT INTO \"%s\".\"%s\" (%s) VALUES "
                       "('%s'::GEOMETRY)",
                       pg_info->schema_name, pg_info->table_name,
                       pg_info->geom_column, geom_data);
        }
        else if (cat > 0) {
            /* no attributes (topology elements) */
            G_asprintf(&stmt, "INSERT INTO \"%s\".\"%s\" (%s) VALUES (NULL)",
                       pg_info->schema_name, pg_info->table_name,
                       pg_info->geom_column); 
	}
    }
    
    return stmt;
}

/*!
  \brief Insert topological element into 'node' or 'edge' table

  \param Map pointer to Map_info struct
  \param line feature id (-1 for nodes/points)
  \param type feature type (GV_POINT, GV_LINE, ...)
  \param geom_data geometry in wkb

  \return 0 on success
  \return -1 on error
*/
int insert_topo_element(struct Map_info *Map, int line, int type,
                        const char *geom_data)
{
    char *stmt;
    struct Format_info_pg *pg_info;
    struct P_line *Line;
    
    pg_info = &(Map->fInfo.pg);
    
    if (line > 0)
        Line = Map->plus.Line[line];

    stmt = NULL;
    switch(type) {
    case GV_POINT: {
#if USE_TOPO_STMT
        G_asprintf(&stmt, "SELECT topology.AddNode('%s', '%s'::GEOMETRY)",
                   pg_info->toposchema_name, geom_data);
#else
        G_asprintf(&stmt, "INSERT INTO \"%s\".node (geom) VALUES ('%s'::GEOMETRY)",
                   pg_info->toposchema_name, geom_data);
#endif
        break;
    }
    case GV_LINE:
    case GV_BOUNDARY: {
#if USE_TOPO_STMT
        G_asprintf(&stmt, "SELECT topology.AddEdge('%s', '%s'::GEOMETRY)",
                   pg_info->toposchema_name, geom_data);
#else
        int nle, nre;
        
        if (!Line) {
            G_warning(_("Topology not available. Unable to insert new edge."));
            return -1;
        }
        
        struct P_topo_l *topo = (struct P_topo_l *) Line->topo;
        
        /* assuming isolated lines */
        nle = -Line->offset;
        nre = Line->offset;
        
        G_debug(3, "new edge: id=%lu next_left_edge=%d next_right_edge=%d",
                Line->offset, nle, nre);
        
        G_asprintf(&stmt, "INSERT INTO \"%s\".edge_data (geom, start_node, end_node, "
                   "next_left_edge, abs_next_left_edge, next_right_edge, abs_next_right_edge, "
                   "left_face, right_face) "
                   "VALUES ('%s'::GEOMETRY, %d, %d, %d, %d, %d, %d, 0, 0)",
                   pg_info->toposchema_name, geom_data, topo->N1, topo->N2, nle, abs(nle),
                   nre, abs(nre));
#endif
        break;
    }
    case GV_CENTROID: {
#if USE_TOPO_STMT
        G_asprintf(&stmt, "SELECT topology.AddNode('%s', '%s'::GEOMETRY)",
                   pg_info->toposchema_name, geom_data);
#else
        if (!Line) {
            G_warning(_("Topology not available. Unable to insert new node (centroid)"));
            return -1;
        }
        
        struct P_topo_c *topo = (struct P_topo_c *) Line->topo;

        /* get id - see write_line_tp()
           
        sprintf(stmt_next, "SELECT nextval('\"%s\".node_node_id_seq')",
                pg_info->toposchema_name);
        Line->offset = Vect__execute_get_value_pg(pg_info->conn, stmt_next);
        if (Line->offset < 1) {
            G_warning(_("Invalid feature offset"));
            return NULL;
        }
        */
        G_asprintf(&stmt, "INSERT INTO \"%s\".node (containing_face, geom) "
                   "VALUES (%d, '%s'::GEOMETRY)",
                   pg_info->toposchema_name, topo->area, geom_data);
#endif
        break;
    }
    default:
        G_warning(_("Unsupported feature type %d"), type);
        break;
    }
    
    if(Vect__execute_pg(pg_info->conn, stmt) == -1) {
        /* rollback transaction */
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    return 0;
}

/*!
  \brief Find next line (topo only) 

  \param Map pointer to Map_info struct
  \param nlines number of lines
  \param line current line
  \param[out] left left line
  \param[out] right right line
  
  \return left (line < 0) or right (line > 0) next edge
  \return 0 on failure
*/
int update_next_edge(struct Map_info* Map, int nlines, int line)
{
    int ret, next_line, edge;
    char stmt[DB_SQL_MAX];
    
    const struct Format_info_pg *pg_info;
    struct P_line *Line_next, *Line;
    
    Line = Line_next = NULL;
    
    pg_info = &(Map->fInfo.pg);

    /* find next line
       start node -> next on the left
       end node   -> next on the right
    */ 
    next_line = dig_angle_next_line(&(Map->plus), line, GV_LEFT, GV_LINES, NULL);
    G_debug(3, "line=%d next_line=%d", line, next_line);
    if (next_line == 0) {
        G_warning(_("Invalid topology"));
        return 0; 
    }
    
    Line      = Map->plus.Line[abs(line)];
    Line_next = Map->plus.Line[abs(next_line)];
    if (!Line || !Line_next) {
        G_warning(_("Invalid topology"));
        return 0;
    }
    
    if (line > 0) {
        edge = Line->offset;
        ret = next_line > 0 ? Line_next->offset : -Line_next->offset;
    }
    else {
        edge = -Line->offset;
        ret = next_line > 0 ? Line_next->offset : -Line_next->offset;
    }
    
    if (next_line < 0) {
        sprintf(stmt, "UPDATE \"%s\".edge_data SET next_left_edge = %d, "
                "abs_next_left_edge = %d WHERE edge_id = %lu AND abs_next_left_edge = %lu",
                pg_info->toposchema_name, edge, abs(edge), Line_next->offset,  Line_next->offset);
        G_debug(3, "update edge=%lu next_left_edge=%d (?)", Line_next->offset, edge);
    }
    else {
        sprintf(stmt, "UPDATE \"%s\".edge_data SET next_right_edge = %d, "
                "abs_next_right_edge = %d WHERE edge_id = %lu AND abs_next_right_edge = %lu",
                pg_info->toposchema_name, edge, abs(edge), Line_next->offset, Line_next->offset);
        G_debug(3, "update edge=%lu next_right_edge=%d (?)", Line_next->offset, edge);
    }
    
    if(Vect__execute_pg(pg_info->conn, stmt) == -1) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return 0;
    }
    
    if (nlines > 2) {
        /* more lines connected to the node

           start node -> next on the right
           end node   -> next on the left
        */
        next_line = dig_angle_next_line(&(Map->plus), line, GV_RIGHT, GV_LINES, NULL);
        Line_next = Map->plus.Line[abs(next_line)];
        
        if (next_line < 0) {
            sprintf(stmt, "UPDATE \"%s\".edge_data SET next_left_edge = %d, "
                    "abs_next_left_edge = %d WHERE edge_id = %lu",
                    pg_info->toposchema_name, edge, abs(edge), Line_next->offset);
            G_debug(3, "update edge=%lu next_left_edge=%d", Line_next->offset, edge);
        }
        else {
            sprintf(stmt, "UPDATE \"%s\".edge_data SET next_right_edge = %d, "
                    "abs_next_right_edge = %d WHERE edge_id = %lu",
                    pg_info->toposchema_name, edge, abs(edge), Line_next->offset);
            G_debug(3, "update edge=%lu next_right_edge=%d", Line_next->offset, edge);
        }
     
        if(Vect__execute_pg(pg_info->conn, stmt) == -1) {
            Vect__execute_pg(pg_info->conn, "ROLLBACK");
            return 0;
        }
    }
    
    return ret;
}

/*!
  \brief Insert new face to the 'face' table (topo only)

  \param Map pointer to Map_info struct
  \param area area id (negative id for isles)

  \return 0 on error
  \return area id on success (>0)
*/
int Vect__insert_face_pg(struct Map_info *Map, int area)
{
    char *stmt;
    
    struct Format_info_pg *pg_info;
    struct bound_box box;
    
    if (area == 0)
        return 0; /* universal face has id '0' in PostGIS Topology */

    stmt = NULL;
    pg_info = &(Map->fInfo.pg);

    /* check if face exists */
    
    /* get mbr of the area */
    if (area > 0)
        Vect_get_area_box(Map, area, &box);
    else
        Vect_get_isle_box(Map, abs(area), &box);
    
    /* insert face if not exists */
    G_asprintf(&stmt, "INSERT INTO \"%s\".face (face_id, mbr) VALUES "
               "(%d, ST_GeomFromText('POLYGON((%.12f %.12f, %.12f %.12f, %.12f %.12f, %.12f %.12f, "
               "%.12f %.12f))', %d))", pg_info->toposchema_name, area,
               box.W, box.S, box.W, box.N, box.E, box.N,
               box.E, box.S, box.W, box.S, pg_info->srid);
    G_debug(3, "new face id=%d", area);
    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return 0;
    }
    G_free(stmt);

    return area;
}

/*!
  \brief Delete existing face

  \todo Set foreign keys as DEFERRABLE INITIALLY DEFERRED and use SET
  CONSTRAINTS ALL DEFERRED
  
  \param Map pointer to Map_info struct
  \param area area id to delete

  \return 0 on success
  \return -1 on error
*/
int delete_face(const struct Map_info *Map, int area)
{
    char stmt[DB_SQL_MAX];

    const struct Format_info_pg *pg_info;

    pg_info = &(Map->fInfo.pg);
    
    /* update centroids first */
    sprintf(stmt, "UPDATE \"%s\".node SET containing_face = 0 "
            "WHERE containing_face = %d",
            pg_info->toposchema_name, area);
    G_debug(3, "SQL: %s", stmt);
    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    /* update also edges (left face) */
    sprintf(stmt, "UPDATE \"%s\".edge_data SET left_face = 0 "
            "WHERE left_face = %d",
            pg_info->toposchema_name, area);
    G_debug(3, "SQL: %s", stmt);
    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    /* update also edges (left face) */
    sprintf(stmt, "UPDATE \"%s\".edge_data SET right_face = 0 "
            "WHERE right_face = %d",
            pg_info->toposchema_name, area);
    G_debug(3, "SQL: %s", stmt);
    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    /* delete face */
    sprintf(stmt, "DELETE FROM \"%s\".face WHERE face_id = %d",
            pg_info->toposchema_name, area);
    G_debug(3, "delete face id=%d", area);
    if (Vect__execute_pg(pg_info->conn, stmt) == -1) {
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }

    return 0;
}

/*!
  \brief Update lines (next left and right edges)
  
  - isolated edges
  next left  edge: -edge 
  next right edge:  edge
  
  - connected edges
  next left  edge: next edge or -edge
  next right edge: next edge or  edge

  \param Map pointer to Map_info struct
  \param line feature id 

  \return 0  on success
  \return -1 on error
*/ 
int update_topo_edge(struct Map_info *Map, int line)
{
    int i, n;
    int nle, nre, next_edge;
    char stmt[DB_SQL_MAX];
    
    struct Format_info_pg *pg_info;
    struct P_line *Line;

    pg_info = &(Map->fInfo.pg);
    
    if (line < 1 || line > Vect_get_num_lines(Map)) {
        G_warning(_("Inconsistency in topology: invalid feature id"));
        return -1;
    }
    Line = Map->plus.Line[line];
    if (!Line) {
        G_warning(_("Inconsistency in topology: dead line found"));
        return -1;
    }
    
    struct P_topo_l *topo = (struct P_topo_l *) Line->topo;
    
    nre = nle = 0; /* edge = 0 is an illegal value */
    
    /* check for line connection */
    for (i = 0; i < 2; i++) {
        /* first check start node then end node */
        n = i == 0 ? Vect_get_node_n_lines(Map, topo->N1)
            : Vect_get_node_n_lines(Map, topo->N2); 
        
        if (n < 2) /* no connection */
            continue;
        
        next_edge = update_next_edge(Map, n,
                                     i == 0 ? line : -line);
        if (next_edge != 0) {
            if (i == 0)
                nre = next_edge; /* update next right edge for start node */
            else
                nle = next_edge; /* update next left edge for end node */
        }
        else {
            G_warning(_("Inconsistency in topology detected: "
                        "unable to determine next left/right edge."));
            return -1;
        }
    }

    if (nle == 0 && nre == 0) /* nothing changed */
        return 0;
    
    if (nle != 0 && nre != 0) {
        /* update both next left and right edge */
        sprintf(stmt, "UPDATE \"%s\".edge_data SET "
                "next_left_edge = %d, abs_next_left_edge = %d, "
                "next_right_edge = %d, abs_next_right_edge = %d "
                "WHERE edge_id = %lu", pg_info->toposchema_name,
                nle, abs(nle), nre, abs(nre), Line->offset);
    }
    else if (nle != 0) {
        /* update next left edge only */
        sprintf(stmt, "UPDATE \"%s\".edge_data SET "
                "next_left_edge = %d, abs_next_left_edge = %d "
                "WHERE edge_id = %lu", pg_info->toposchema_name,
                nle, abs(nle), Line->offset);
    }
    else {
        /* update next right edge only */
        sprintf(stmt, "UPDATE \"%s\".edge_data SET "
                "next_right_edge = %d, abs_next_right_edge = %d "
                "WHERE edge_id = %lu", pg_info->toposchema_name,
                nre, abs(nre), Line->offset);
    }
    G_debug(3, "update edge=%lu next_left_edge=%d next_right_edge=%d",
            Line->offset, nle, nre);
    
    if(Vect__execute_pg(pg_info->conn, stmt) == -1) {
        /* rollback transaction */
        Vect__execute_pg(pg_info->conn, "ROLLBACK");
        return -1;
    }
    
    return 0;
}

/*!
  \brief Update lines (left and right faces)

  TODO: handle isles
  
  \param Map pointer to Map_info struct
  \param line feature id 

  \return 0  on success
  \return -1 on error
*/  
int update_topo_face(struct Map_info *Map, int line)
{
    int i, s, area, face[2];
    char stmt[DB_SQL_MAX];
    
    struct Format_info_pg *pg_info;
    struct P_line *Line, *Line_i;
    struct P_area *Area;
    struct P_topo_b *topo, *topo_i;
    
    pg_info = &(Map->fInfo.pg);
    
    if (line < 1 || line > Vect_get_num_lines(Map)) {
        G_warning(_("Inconsistency in topology detected: invalid feature id"));
        return -1;
    }
    Line = Map->plus.Line[line];
    if (!Line) {
        G_warning(_("Inconsistency in topology detected: dead line found"));
        return -1;
    }
    
    topo = (struct P_topo_b *)Line->topo;
    
    /* for both side on the current boundary (line) */
    /* create new faces */
    for (s = 0; s < 2; s++) { /* for each side */
        area = s == 0 ? topo->left : topo->right;
        if (area <= 0) /* no area - skip */
            continue;

        face[s] = Vect__insert_face_pg(Map, area);
        if (face[s] < 1) {
            G_warning(_("Unable to create new face"));
            return -1;
        }
    }
    
    /* update edges forming faces */
    for (s = 0; s < 2; s++) { /* for each side */
        area = s == 0 ? topo->left : topo->right;
        if (area <= 0) /* no area - skip */
          continue;
        
        Area = Map->plus.Area[area];
        for (i = 0; i < Area->n_lines; i++) {
            Line_i = Map->plus.Line[abs(Area->lines[i])];
            topo_i = (struct P_topo_b *)Line_i->topo;
            
            sprintf(stmt, "UPDATE \"%s\".edge_data SET "
                    "left_face = %d, right_face = %d "
                    "WHERE edge_id = %lu", pg_info->toposchema_name,
                    topo_i->left > 0 ? topo_i->left : 0,
                    topo_i->right > 0 ? topo_i->right : 0,
                    Line_i->offset);
            G_debug(2, "SQL: %s", stmt);
            
            if(Vect__execute_pg(pg_info->conn, stmt) == -1) {
                Vect__execute_pg(pg_info->conn, "ROLLBACK");
                return -1;
            }
        }
        
        /* update also centroids (stored as nodes) */
        if (Area->centroid > 0) {
            Line_i = Map->plus.Line[Area->centroid];
            sprintf(stmt, "UPDATE \"%s\".node SET containing_face = %d "
                    "WHERE node_id = %lu", pg_info->toposchema_name,
                    face[s], Line_i->offset);
            G_debug(2, "SQL: %s", stmt);
            
            if(Vect__execute_pg(pg_info->conn, stmt) == -1) {
                /* rollback transaction */
                Vect__execute_pg(pg_info->conn, "ROLLBACK");
                return -1;
            }
        }
    }
    
    return 0;
}

#endif
