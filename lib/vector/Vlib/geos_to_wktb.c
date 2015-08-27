/*!
  \file lib/vector/Vlib/geos_to_wktb.c

  \brief Vector library - GEOS powered WKT and WKB export

  Higher level functions for reading/writing/manipulating vectors.

  (C) 2015 by the GRASS Development Team

  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.

  \author Soeren Gebbert <soerengebbert googlemail.com>
 */

#include <stdlib.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#ifdef HAVE_GEOS

/*!
   \brief Read vector area and stores it as WKB unsigned char array

   \param Map pointer to Map_info structure
   \param area area idmetry instance
   \param size The size of the returned byte array
   \return NULL on error

   \return pointer to char array
   \return NULL on error
 */
unsigned char *Vect_read_area_to_wkb(struct Map_info * Map, int area, size_t *size)
{
    static int init = 0;
    /* The writer is static for performance reasons */
    static GEOSWKBWriter *writer = NULL;
    unsigned char *wkb = NULL;

    if(init == 0) {
        initGEOS(NULL, NULL);
        writer = GEOSWKBWriter_create();
        init += 1;
    }

    GEOSWKBWriter_setOutputDimension(writer, 2);

    GEOSGeometry *geom = Vect_read_area_geos(Map, area);

    if(!geom) {
        return(NULL);
    }

    wkb = GEOSWKBWriter_write(writer, geom, size);

    GEOSGeom_destroy(geom);

    return(wkb);
}

/*!
   \brief Read vector area and stores it as WKT char array

   \param Map pointer to Map_info structure
   \param area area idmetry instance
   \return NULL on error

   \return pointer to char array
   \return NULL on error
 */
char *Vect_read_area_to_wkt(struct Map_info * Map, int area)
{
    static int init = 0;
    /* The writer is static for performance reasons */
    static GEOSWKTWriter *writer = NULL;
    char *wkt = NULL;

    if(init == 0) {
        initGEOS(NULL, NULL);
        writer = GEOSWKTWriter_create();
        init += 1;
    }

    GEOSWKTWriter_setOutputDimension(writer, 2);

    GEOSGeometry *geom = Vect_read_area_geos(Map, area);

    if(!geom) {
        return(NULL);
    }

    wkt = GEOSWKTWriter_write(writer, geom);

    GEOSGeom_destroy(geom);

    return(wkt);
}

/*!
   \brief Create a WKB representation of given type from feature points.

   This function is not thread safe, it uses static variables for speedup.

   Supported types:
   - GV_POINT    -> POINT
   - GV_CENTROID -> POINT
   - GV_LINE     -> LINESTRING
   - GV_BOUNDARY -> LINEARRING

   \param points pointer to line_pnts structure
   \param type feature type (see supported types)
   \param with_z Set to 1 if the feature is 3d, 0 otherwise
   \param size The size of the returned byte array

   \return pointer to char array
   \return NULL on error
 */
unsigned char *Vect_line_to_wkb(const struct line_pnts *points,
                       int type, int with_z, size_t *size)
{
    static int init = 0;
    /* The writer is static for performance reasons */
    static GEOSWKBWriter *writer = NULL;
    unsigned char *wkb = NULL;

    if(init == 0) {
        initGEOS(NULL, NULL);
        writer = GEOSWKBWriter_create();
        init += 1;
    }

    GEOSWKBWriter_setOutputDimension(writer, with_z ? 3 : 2);

    GEOSGeometry *geom = Vect_line_to_geos(points, type, with_z);

    if(!geom) {
        return(NULL);
    }

    wkb = GEOSWKBWriter_write(writer, geom, size);

    GEOSGeom_destroy(geom);

    return(wkb);
}

/*!
   \brief Create a WKT representation of given type from feature points.

   This function is not thread safe, it uses static variables for speedup.

   Supported types:
   - GV_POINT    -> POINT
   - GV_CENTROID -> POINT
   - GV_LINE     -> LINESTRING
   - GV_BOUNDARY -> LINEARRING

   \param points pointer to line_pnts structure
   \param type feature type (see supported types)
   \param with_z Set to 1 if the feature is 3d, 0 otherwise
   \param with_z Set to 1 if the feature is 3d, 0 otherwise

   \return pointer to char array
   \return NULL on error
 */
char *Vect_line_to_wkt(const struct line_pnts *points,
                       int type, int with_z)
{
    static int init = 0;
    /* The writer is static for performance reasons */
    static GEOSWKTWriter *writer = NULL;
    char *wkt = NULL;

    if(init == 0) {
        initGEOS(NULL, NULL);
        writer = GEOSWKTWriter_create();
        init += 1;
    }

    GEOSWKTWriter_setOutputDimension(writer, with_z ? 3 : 2);

    GEOSGeometry *geom = Vect_line_to_geos(points, type, with_z);

    if(!geom) {
        return(NULL);
    }

    wkt = GEOSWKTWriter_write(writer, geom);

    GEOSGeom_destroy(geom);

    return(wkt);
}

#endif /* HAVE_GEOS */
