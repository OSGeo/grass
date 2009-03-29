/*!
 * \file geos.c
 *
 * \brief Vector library - GEOS support
 *
 * Higher level functions for reading/writing/manipulating vectors.
 *
 * (C) 2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2).  Read the file COPYING that comes with GRASS for details.
 *
 * \author Martin Landa <landa.martin gmail.com>
 */

#include <grass/config.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

#ifdef HAVE_GEOS
#include <geos_c.h>

static struct line_pnts *Points;

/*!
   \brief Read vector feature and stores it as GEOSGeometry instance

   Note: Free allocated memory by GEOSGeom_destroy().

   \param Map pointer to Map_info structure
   \param line feature id

   \return pointer to GEOSGeometry instance
   \return NULL on error
 */
GEOSGeometry *Vect_read_line_geos(const struct Map_info *Map, int line)
{
    int type, dim;
    GEOSGeometry *geom;
    GEOSCoordSequence *pseq;

    if (!Points)
	Points = Vect_new_line_struct();

    G_debug(3, "Vect_read_line_geos(line=%d)", line);

    if (Vect_is_3d(Map))
	dim = 3;
    else
	dim = 2;

    type = Vect_read_line(Map, Points, NULL, line);
    if (type < 0)
	return NULL;

    return Vect_line_to_geos(Map, Points, type);
}

/*!
   \brief Read vector area and stores it as GEOSGeometry instance

   Note: Free allocated memory by GEOSGeom_destroy().

   \param Map pointer to Map_info structure
   \param area area id 

   \return pointer to GEOSGeometry instance
   \return NULL on error
 */
GEOSGeometry *Vect_read_area_geos(const struct Map_info * Map, int area)
{
    int i, type, dim, nholes, isle;
    GEOSGeometry *boundary, **holes;

    if (!Points)
	Points = Vect_new_line_struct();

    G_debug(3, "Vect_read_area_geos(area=%d)", area);

    if (Vect_is_3d(Map))
	dim = 3;
    else
	dim = 2;

    if (Vect_get_area_points(Map, area, Points) == -1) {
	G_fatal_error(_("Vect_read_area_geos(): unable to read area id %d"),
		      area);
    }
    boundary = Vect_line_to_geos(Map, Points, GV_BOUNDARY);
    if (!boundary) {
	G_fatal_error(_("Vect_read_area_geos(): unable to read area id %d"),
		      area);
    }

    nholes = Vect_get_area_num_isles(Map, area);
    holes = (GEOSGeometry **) G_malloc(nholes * sizeof(GEOSGeometry *));
    for (i = 0; i < nholes; i++) {
	isle = Vect_get_area_isle(Map, area, i);
	if (isle < 1)
	    continue;
	if (Vect_get_isle_points(Map, isle, Points) < 0)
	    G_fatal_error(_("Vect_read_area_geos(): unable to read isle id %d of area id %d"),
			  isle, area);
	holes[i] = Vect_line_to_geos(Map, Points, GV_BOUNDARY);
    }

    return GEOSGeom_createPolygon(boundary, holes, nholes);
}

/*!
   \brief Create GEOSGeometry of given type from feature points.

   Supported types:
   - GV_POINT    -> POINT
   - GV_LINE     -> LINESTRING
   - GV_BOUNDARY -> LINERING

   Note: Free allocated memory by GEOSGeom_destroy().

   \param Map pointer to Map_info structure
   \param type feature type (see supported types)

   \return pointer to GEOSGeometry instance
   \return NULL on error
 */
GEOSGeometry *Vect_line_to_geos(const struct Map_info * Map,
				const struct line_pnts * points, int type)
{
    int i, dim;
    GEOSGeometry *geom;
    GEOSCoordSequence *pseq;

    G_debug(3, "Vect_line_to_geos(type=%d)", type);

    if (Vect_is_3d(Map))
	dim = 3;
    else
	dim = 2;

    if (type == GV_POINT) {
	if (points->n_points != 1)
	    return NULL;
    }
    else {			/* GV_LINES */
	if (points->n_points < 2)
	    return NULL;
    }

    if (!(type & (GV_POINT | GV_LINES)))
	return NULL;

    pseq = GEOSCoordSeq_create(points->n_points, dim);

    for (i = 0; i < points->n_points; i++) {
	GEOSCoordSeq_setX(pseq, i, points->x[i]);
	GEOSCoordSeq_setY(pseq, i, points->y[i]);
	if (dim == 3)
	    GEOSCoordSeq_setZ(pseq, i, points->z[i]);
    }

    if (type == GV_POINT)
	geom = GEOSGeom_createPoint(pseq);
    else if (type == GV_LINE)
	geom = GEOSGeom_createLineString(pseq);
    else {			/* GV_BOUNDARY */
	geom = GEOSGeom_createLineString(pseq);
	if (GEOSisRing(geom)) {
	    /* GEOSGeom_destroy(geom); */
	    geom = GEOSGeom_createLinearRing(pseq);
	}
    }

    /* GEOSCoordSeq_destroy(pseq); */

    return geom;
}
#endif /* HAVE_GEOS */
