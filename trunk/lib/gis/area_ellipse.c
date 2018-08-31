/*!
 * \file lib/gis/area_ellipse.c
 *
 * \brief GIS Library - Ellipse area routines.
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
    double E;
    double M;
} state;

static struct state *st = &state;

/*
 * a is semi-major axis, e2 is eccentricity squared, s is a scale factor
 * code will fail if e2==0 (sphere)
 */

/*!
 * \brief Begin area calculations for ellipsoid.
 *
 * Initializes raster area calculations for an ellipsoid, where <i>a</i> 
 * is the semi-major axis of the ellipse (in meters), <i>e2</i> is the 
 * ellipsoid eccentricity squared, and <i>s</i> is a scale factor to 
 * allow for calculations of part of the zone (<i>s</i>=1.0 is full 
 * zone, <i>s</i>=0.5 is half the zone, and <i>s</i>=360/ew_res is for a 
 * single grid cell).
 * 
 * <b>Note:</b> <i>e2</i> must be positive. A negative value makes no 
 * sense, and zero implies a sphere.
 *
 * \param a semi-major axis
 * \param e2 ellipsoid eccentricity
 * \param s scale factor
 */
void G_begin_zone_area_on_ellipsoid(double a, double e2, double s)
{
    st->E = sqrt(e2);
    st->M = s * a * a * M_PI * (1 - e2) / st->E;
}

/*!
 * \brief Calculate integral for area between two latitudes.
 *
 * This routine is part of the integral for the area between two 
 * latitudes.
 *
 * \param lat latitude
 *
 * \return cell area
 */
double G_darea0_on_ellipsoid(double lat)
{
    double x;

    x = st->E * sin(Radians(lat));

    return (st->M * (x / (1.0 - x * x) + 0.5 * log((1.0 + x) / (1.0 - x))));
}

/*!
 * \brief Calculates area between latitudes.
 *
 * This routine shows how to calculate area between two lats, but
 * isn't efficient for row by row since G_darea0_on_ellipsoid() 
 * will be called twice for the same lat, once as a <i>south</i> then 
 * again as a <i>north</i>.
 * 
 * Returns the area between latitudes <i>north</i> and <i>south</i>
 * scaled by the factor <i>s</i> passed to
 * G_begin_zone_area_on_ellipsoid().
 *
 * \param north north coordinate
 * \param south south coordinate
 *
 * \return cell area
 */
double G_area_for_zone_on_ellipsoid(double north, double south)
{
    return (G_darea0_on_ellipsoid(north) - G_darea0_on_ellipsoid(south));
}
