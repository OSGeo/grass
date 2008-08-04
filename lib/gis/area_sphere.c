
/**
 * \file area_sphere.c
 *
 * \brief GIS Library - Sphereical area calculation routines.
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


static double M;


/*
 * r is radius of sphere, s is a scaling factor
 */

/**
 * \brief Initialize calculations for sphere.
 *
 * Initializes raster area calculations for a sphere.
 * The radius of the sphere is <b>r</b> and <b>s</b> is a scale factor to
 * allow for calculations of a part of the zone (see
 * <i>G_begin_zone_area_on_ellipsoid()</i>).
 *
 * \param[in] r radius of sphere
 * \param[in] s scale factor
 * \return int
 */

int G_begin_zone_area_on_sphere(double r, double s)
{
    return (M = s * 2.0 * r * r * M_PI);
}


/**
 * \brief Calculates integral for area between two latitudes.
 *
 * \param[in] lat latitude
 * \return double
 */

double G_darea0_on_sphere(double lat)
{
    return (M * sin(Radians(lat)));
}


/**
 * \brief Calculates area between latitudes.
 *
 * This routine shows how to calculate area between two lats, but
 * isn't efficient for row by row since <i>G_darea0_on_sphere()</i> will 
 * be called twice for the same lat, once as a <b>south</b> then 
 * again as a <b>north</b>.
 * <br>
 * Returns the area between latitudes <b>north</b> and <b>south</b> 
 * scaled by the factor <b>s</b> passed to 
 * <i>G_begin_zone_area_on_sphere()</i>.
 *
 * \param[in] north
 * \param[in] south
 * \return double
 */

double G_area_for_zone_on_sphere(double north, double south)
{
    return (G_darea0_on_sphere(north) - G_darea0_on_sphere(south));
}
