
/**
 * \file area_ellipse.c
 *
 * \brief GIS Library - Ellipse area routines.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2008
 */

#include <math.h>
#include <grass/gis.h>
#include "pi.h"


static double E;
static double M;


/*
 * a is semi-major axis, e2 is eccentricity squared, s is a scale factor
 * code will fail if e2==0 (sphere)
 */

/**
 * \brief Begin area calculations for ellipsoid.
 *
 * Initializes raster area calculations for an ellipsoid, where <b>a</b> 
 * is the semi-major axis of the ellipse (in meters), <b>e2</b> is the 
 * ellipsoid eccentricity squared, and <b>s</b> is a scale factor to 
 * allow for calculations of part of the zone (<b>s</b>=1.0 is full 
 * zone, <b>s</b>=0.5 is half the zone, and <b>s</b>=360/ew_res is for a 
 * single grid cell).
 * <br>
 * <b>Note:</b> <b>e2</b> must be positive. A negative value makes no 
 * sense, and zero implies a sphere.
 *
 * \param[in] a semi-major axis
 * \param[in] e2 ellipsoid eccentricity
 * \param[in] s scale factor
 * \return always returns 0
 */

int G_begin_zone_area_on_ellipsoid(double a, double e2, double s)
{
    E = sqrt(e2);
    M = s * a * a * M_PI * (1 - e2) / E;

    return 0;
}


/**
 * \brief Calculate integral for area between two latitudes.
 *
 * This routine is part of the integral for the area between two 
 * latitudes.
 *
 * \param[in] lat
 * \return double
 */

double G_darea0_on_ellipsoid(double lat)
{
    double x;

    x = E * sin(Radians(lat));

    return (M * (x / (1.0 - x * x) + 0.5 * log((1.0 + x) / (1.0 - x))));
}


/**
 * \brief Calculates area between latitudes.
 *
 * This routine shows how to calculate area between two lats, but
 * isn't efficient for row by row since <i>G_darea0_on_ellipsoid()</i> 
 * will be called twice for the same lat, once as a <b>south</b> then 
 * again as a <b>north</b>.
 * <br>
 * Returns the area between latitudes <b>north</b> and <b>south</b> 
 * scaled by the factor <b>s</b> passed to
 * <i>G_begin_zone_area_on_ellipsoid()</i>.
 *
 * \param[in] north
 * \param[in] south
 * \return double
 */

double G_area_for_zone_on_ellipsoid(double north, double south)
{
    return (G_darea0_on_ellipsoid(north) - G_darea0_on_ellipsoid(south));
}
