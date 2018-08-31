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
   \brief Read vector area and return it as Well Known Binary (WKB)
          unsigned char array

   \param Map pointer to Map_info structure
   \param area area id
   \param size The size of the returned unsigned char array

   \return pointer to unsigned char array
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
   \brief Read vector area and return it as Well Known Text (WKT)
          unsigned char array

   \param Map pointer to Map_info structure
   \param area area id
   \param size The size of the returned unsigned char array

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
   \brief Read a Well Known Binary (WKB) representation of
          a given feature id.
           
   This function reads a specific feature and converts it into a 
   WKB representation. line_pnts and line_cats structures can be provided
   to store the result of the read operation. That is meaningful in case
   the category values of the feature are needed.
   This function is not thread safe, it uses static variables for speedup.

   Supported feature types:
   - GV_POINT    -> POINT
   - GV_CENTROID -> POINT
   - GV_LINE     -> LINESTRING
   - GV_BOUNDARY -> LINEARRING

   \param Map pointer to Map_info structure
   \param line_p pointer to line_pnts structure to use, or NULL
   \param line_c pointer to line_cats structure to use, or NULL
   \param line The id of the feature to read
   \param size The size of the returned unsigned char array

   \return pointer to unsigned char array
   \return NULL on error
 */
unsigned char *Vect_read_line_to_wkb(const struct Map_info *Map, 
                                     struct line_pnts *line_p, 
                                     struct line_cats *line_c, 
                                     int line, size_t *size,
                                     int *error)
{    
    static int init = 0;
    /* The writer is static for performance reasons */
    static GEOSWKBWriter *writer = NULL;
    unsigned char *wkb = NULL;
    int destroy_line = 0, destroy_cats = 0;

    if(init == 0) {
        initGEOS(NULL, NULL);
        writer = GEOSWKBWriter_create();
        init += 1;
    }

    if(line_p == NULL) {
        destroy_line = 1;
        line_p = Vect_new_line_struct();
    }
    
    if(line_c == NULL) {
        destroy_cats = 1;
        line_c = Vect_new_cats_struct();
    }
    
    int f_type = Vect_read_line(Map, line_p, line_c, line);
    /* Save the error state */
    *error = f_type;
    
    if(f_type < 0)
        return(NULL);
    
    GEOSWKBWriter_setOutputDimension(writer, Vect_is_3d(Map) ? 3 : 2);

    GEOSGeometry *geom = Vect_line_to_geos(line_p, f_type, Vect_is_3d(Map));
    
    if(destroy_cats == 1)
        Vect_destroy_cats_struct(line_c);

    if(destroy_line == 1)
        Vect_destroy_line_struct(line_p);

    if(!geom) {
        return(NULL);
    }

    wkb = GEOSWKBWriter_write(writer, geom, size);

    GEOSGeom_destroy(geom);

    return(wkb);
}

/*!
   \brief Create a Well Known Binary (WKB) representation of
          given feature type from points.

   This function is not thread safe, it uses static variables for speedup.

   Supported feature types:
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
   \brief Create a Well Known Text (WKT) representation of
          given feature type from points.

   This function is not thread safe, it uses static variables for speedup.

   Supported types:
   - GV_POINT    -> POINT
   - GV_CENTROID -> POINT
   - GV_LINE     -> LINESTRING
   - GV_BOUNDARY -> LINEARRING

   \param points pointer to line_pnts structure
   \param type feature type (see supported types)
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
