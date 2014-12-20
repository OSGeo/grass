/*!
 * \file lib/gis/area_sphere.c
 *
 * \brief GIS Library - Sphereical area calculation routines.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <math.h>
#include <grass/gis.h>
#include "pi.h"


static struct state {
    double M;
} state;

static struct state *st = &state;

/*!
 * \brief Initialize calculations for sphere.
 *
 * Initializes raster area calculations for a sphere.
 * The radius of the sphere is <i>r</i> and <i>s</i> is a scale factor to
 * allow for calculations of a part of the zone (see
 * G_begin_zone_area_on_ellipsoid()).
 *
 * \param r radius of sphere
 * \param s scale factor
 */
void G_begin_zone_area_on_sphere(double r, double s)
{
    st->M = s * 2.0 * r * r * M_PI;
}

/*!
 * \brief Calculates integral for area between two latitudes.
 *
 * \param lat latitude
 *
 * \return area value
 */
double G_darea0_on_sphere(double lat)
{
    return (st->M * sin(Radians(lat)));
}

/*!
 * \brief Calculates area between latitudes.
 *
 * This routine shows how to calculate area between two lats, but
 * isn't efficient for row by row since G_darea0_on_sphere() will 
 * be called twice for the same lat, once as a <i>south</i> then 
 * again as a <i>north</i>.
 *
 * Returns the area between latitudes <i>north</i> and <i>south</i>
 * scaled by the factor <i>s</i> passed to
 * G_begin_zone_area_on_sphere().
 *
 * \param north
 * \param[in] south
 * \return double
 */

double G_area_for_zone_on_sphere(double north, double south)
{
    return (G_darea0_on_sphere(north) - G_darea0_on_sphere(south));
}
