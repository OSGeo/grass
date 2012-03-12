/*!
   \file lib/vector/Vlib/write_pg.c

   \brief Vector library - write vector feature (PostGIS format)

   Higher level functions for reading/writing/manipulating vectors.

   Inspired by OGR PostgreSQL driver.

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

#ifdef HAVE_POSTGRES
#include "pg_local_proto.h"

#define WKBSRIDFLAG 0x20000000

static char *binary_to_hex(int, const unsigned char *);
static unsigned char *point_to_wkb(int, const struct line_pnts *,
				   int, int*);
static unsigned char *linestring_to_wkb(int, const struct line_pnts *,
					int, int*);
static unsigned char *polygon_to_wkb(int, const struct line_pnts *,
				     int, int*);
static int write_feature(const struct Format_info_pg *, 
			 int, const struct line_pnts *, int, int);
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
    int cat;
    off_t offset;
    
    SF_FeatureType sf_type;

    struct field_info *Fi;
    struct Format_info_pg *pg_info;
    struct Format_info_offset *offset_info;
    
    pg_info = &(Map->fInfo.pg);
    offset_info = &(pg_info->offset);
    
    if (!pg_info->conn || !pg_info->table_name) {
	G_warning(_("No connection defined"));
	return -1;
    }
    
    /* create PostGIS layer if doesn't exist ? */
    
    cat = -1; /* no attributes to be written */
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
	if (sf_type != SF_POINT &&
	    sf_type != SF_POINT25D) {
	    G_warning(_("Feature is not a point. Skipping."));
	    return -1;
	}
    }
    else if (type & GV_LINE) {
	if (sf_type != SF_LINESTRING &&
	    sf_type != SF_LINESTRING25D) {
	    G_warning(_("Feature is not a line. Skipping."));
	    return -1;
	}
    }
    else if (type & GV_BOUNDARY) {
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
	G_warning(_("Unsupported feature type (%d)"), type);
	return -1;
    }

    G_debug(3, "V1_write_line_pg(): type = %d n_points = %d cat = %d",
	    type, points->n_points, cat);

    if (sf_type == SF_POLYGON || sf_type == SF_POLYGON25D) {
	int npoints;
	
	npoints = points->n_points - 1;
	if (points->x[0] != points->x[npoints] ||
	    points->y[0] != points->y[npoints] ||
	    points->z[0] != points->z[npoints]) {
	    G_warning(_("Boundary is not closed. Skipping."));
	    return -1;
	}
    }

    if (execute(pg_info->conn, "BEGIN") == -1)
	return -1;
    
    /* write feature's geometry and fid */
    if (-1 == write_feature(pg_info, type, points,
			    Vect_is_3d(Map) ? WITH_Z : WITHOUT_Z, cat))
	return -1;

    /* write attributes */
    /* ? */
    
    if (execute(pg_info->conn, "COMMIT") == -1)
	return -1;
    
    /* update offset array */
    if (offset_info->array_num >= offset_info->array_alloc) {
	offset_info->array_alloc += 1000;
	offset_info->array = (int *) G_realloc(offset_info->array,
					       offset_info->array_alloc *
					       sizeof(int));
    }

    offset = offset_info->array_num;
    
    offset_info->array[offset_info->array_num++] = (int) cat;
    if (sf_type == SF_POLYGON || sf_type == SF_POLYGON25D) {
	/* register exterior ring in offset array */
	offset_info->array[offset_info->array_num++] = 0; 
    }
    
    G_debug(3, "V1_write_line_ogr(): -> offset = %lu", (unsigned long) offset);

    return offset;
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
off_t V1_rewrite_line_pg(struct Map_info *Map,
			 int line, int type, off_t offset,
			 const struct line_pnts *points, const struct line_cats *cats)
{
    G_debug(3, "V1_rewrite_line_pg(): line=%d type=%d offset=%llu",
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
	    (unsigned long) offset, fid);

    if (execute(pg_info->conn, "BEGIN") == -1)
	return -1;

    sprintf(stmt, "DELETE FROM %s WHERE %s = %ld",
	    pg_info->table_name, pg_info->fid_column, fid);
    G_debug(2, "SQL: %s", stmt);
    
    if (execute(pg_info->conn, stmt) == -1) {
	G_warning(_("Unable to delete feature"));
	return -1;
    }
    
    if (execute(pg_info->conn, "COMMIT") == -1)
	return -1;

    return 0;
#else
    G_fatal_error(_("GRASS is not compiled with PostgreSQL support"));
    return -1;
#endif
}

#ifdef HAVE_POSTGRES
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

    hex_data = (char *) G_malloc(nbytes * 2 + 1);
    hex_data[nbytes * 2] = '\0';

    for (i = 0; i < nbytes; i++) {
        nlow  = wkb_data[i] & 0x0f;
        nhigh = (wkb_data[i] & 0xf0) >> 4;

        hex_data[i * 2]     = ach_hex[nhigh];
        hex_data[i * 2 + 1] = ach_hex[nlow];
    }

    return hex_data;
}

/*!
  \bried Write point into WKB buffer

  See OGRPoint::exportToWkb from GDAL/OGR library

  \param byte_order byte order (LITTLE_ENDIAN or BIG_ENDIAN)
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
    if (byte_order == LITTLE_ENDIAN)
	wkb_data[0] = '\001';
    else
	wkb_data[0] = '\000';
    
    /* set the geometry feature type */
    sf_type = with_z ? SF_POINT25D : SF_POINT;
    
    if (byte_order == LITTLE_ENDIAN)
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

  \param byte_order byte order (LITTLE_ENDIAN or BIG_ENDIAN)
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
    if (byte_order == LITTLE_ENDIAN)
	wkb_data[0] = '\001';
    else
	wkb_data[0] = '\000';

    /* set the geometry feature type */
    sf_type = with_z ? SF_LINESTRING25D : SF_LINESTRING;
    
    if (byte_order == LITTLE_ENDIAN)
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
        memcpy(wkb_data+5, &npoints, 4);

	nitems = (with_z ? 3 : 2) * points->n_points;
        for(i = 0; i < nitems; i++ )
        {
            SWAPDOUBLE(wkb_data + 9 + 4 + 8 * i);
        }
    }

    return wkb_data;
}

/*!
  \bried Write polygon into WKB buffer

  See OGRPolygon::exportToWkb from GDAL/OGR library

  \param byte_order byte order (LITTLE_ENDIAN or BIG_ENDIAN)
  \param points feature geometry
  \param with_z WITH_Z for 3D data
  \param[out] nsize buffer size
  
  \return allocated WKB buffer
  \return NULL on error
*/
unsigned char *polygon_to_wkb(int byte_order,
			      const struct line_pnts *points, int with_z,
			      int *nsize)
{
    int i, point_size, nrings;
    unsigned char *wkb_data;
    unsigned int sf_type;

    if (points->n_points < 3)
	return NULL;

    /* allocate buffer */
    point_size = 8 * (with_z ? 3 : 2);
    /* one ring only */
    nrings = 1;
    *nsize = 9 + (4 + point_size * points->n_points);
    wkb_data = G_malloc(*nsize);
    G_zero(wkb_data, *nsize);

    G_debug(5, "\t->polygon size=%d (with_z = %d)", *nsize, with_z);
    
    /* set the byte order */
    if (byte_order == LITTLE_ENDIAN)
	wkb_data[0] = '\001';
    else
	wkb_data[0] = '\000';

    /* set the geometry feature type */
    sf_type = with_z ? SF_POLYGON25D : SF_POLYGON;
    
    if (byte_order == LITTLE_ENDIAN)
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
    
    /* serialize ring */
    memcpy(wkb_data + 9, &(points->n_points), 4);
    for (i = 0; i < points->n_points; i++) {
	memcpy(wkb_data + 9 + 4 + point_size * i, &(points->x[i]), 8);
	memcpy(wkb_data + 9 + 4 + 8 + point_size * i, &(points->y[i]), 8);
	
	if (with_z) {
	    memcpy(wkb_data + 9 + 4 + 16 + point_size * i, &(points->z[i]), 8);
	}
    }

    /* swap if needed */
    if (byte_order == ENDIAN_BIG) {
        int npoints, nitems;
	
        npoints = SWAP32(points->n_points);
        memcpy(wkb_data+5, &npoints, 4);

	nitems = (with_z ? 3 : 2) * points->n_points;
        for(i = 0; i < nitems; i++ ) {
            SWAPDOUBLE(wkb_data + 9 + 4 + 8 * i);
        }
    }

    return wkb_data;
}
/*!
  \brief Insert feature into table

  \param pg_info pointer to Format_info_pg struct
  \param type feature type (GV_POINT, GV_LINE, ...)
  \param points pointer to line_pnts struct
  \param with_z WITH_Z for 3D data
  \param fid feature id

  \return -1 on error
  \retirn 0 on success
*/
int write_feature(const struct Format_info_pg *pg_info,
		  int type, const struct line_pnts *points, int with_z,
		  int fid)
{
    int   byte_order, nbytes, nsize;
    unsigned int sf_type;
    unsigned char *wkb_data;
    char  *stmt, *text_data, *text_data_p, *hex_data;
    
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
    
    byte_order = LITTLE_ENDIAN; /* ? */
    
    /* get wkb data */
    nbytes = -1;
    wkb_data = NULL;
    if (type == GV_POINT) 
	wkb_data = point_to_wkb(byte_order, points, with_z, &nbytes);
    else if (type == GV_LINE)
	wkb_data = linestring_to_wkb(byte_order, points, with_z, &nbytes);
    else if (type == GV_BOUNDARY) 
	wkb_data = polygon_to_wkb(byte_order, points, with_z, &nbytes);
    
    if (!wkb_data || nbytes < 1) {
	G_warning(_("Unsupported feature type %d"), type);
	return -1;
    }
    
    /* When converting to hex, each byte takes 2 hex characters. In
       addition we add in 8 characters to represent the SRID integer
       in hex, and one for a null terminator */
    nsize = nbytes * 2 + 8 + 1;
    text_data = text_data_p = (char *) G_malloc(nsize);
    
    /* convert the 1st byte, which is the endianess flag, to hex */
    hex_data = binary_to_hex(1, wkb_data);
    strcpy(text_data_p, hex_data);
    G_free (hex_data);
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
    hex_data = binary_to_hex(4, (unsigned char*) &sf_type);
    strcpy(text_data_p, hex_data);
    G_free(hex_data);
    text_data_p += 8;

    /* include SRID if provided */
    if (pg_info->srid > 0) {
	unsigned int srs_id;
        /* force the srsid to little endianess */
        srs_id = LSBWORD32(pg_info->srid);
        hex_data = binary_to_hex(sizeof(srs_id), (unsigned char*) &srs_id);
        strcpy(text_data_p, hex_data);
        G_free(hex_data);
        text_data_p += 8;
    }

    /* copy the rest of the data over - subtract 5 since we already
       copied 5 bytes above */
    hex_data = binary_to_hex(nbytes - 5, wkb_data + 5);
    strcpy(text_data_p, hex_data);
    G_free(hex_data);

    /* build INSERT statement */
    stmt = NULL;
    G_asprintf(&stmt, "INSERT INTO %s (%s, %s) VALUES (%d, '%s'::GEOMETRY)",
	       pg_info->table_name, pg_info->fid_column, pg_info->geom_column,
	       fid, text_data);
    G_debug(2, "SQL: %s", stmt);
    
    if (execute(pg_info->conn, stmt) == -1)
	return -1;

    G_free(wkb_data);
    G_free(text_data);
    G_free(stmt);
    
    return 0;
}
#endif
