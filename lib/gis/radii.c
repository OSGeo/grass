/*!
  \file lib/gis/radii.c

  \brief GIS Library - Calculating the Meridional Radius of Curvature
  
  \todo Suggestion: all "lon"s in the file "radii.c" should read as "lat"
  
  Comments:
  on page http://www.mentorsoftwareinc.com/cc/gistips/TIPS0899.HTM
  down where it says "Meridional Radius of Curvature" is the exact formula
  out of "radii.c".
  Quote: "essentially, the radius of curvature, at a specific latitude ...".
  
  See also http://williams.best.vwh.net/ellipsoid/node1.html which has a nice
  picture showning the parametric latitude and phi, the geodetic latitude.
  On the next page,
  http://williams.best.vwh.net/ellipsoid/node2.html, in equation 3, the
  Meridional Radius of Curvature shows up.
  
  So, it looks like you are calculating the Meridional Radius of Curvature
  as a function of GEODETIC LATITUDE.

 Various formulas for the ellipsoid.
 Reference: Map Projections by Peter Richardus and Ron K. Alder
	    University of Illinois Library Call Number: 526.8 R39m
 Parameters are:
  - lon = longitude of the meridian
  - a   = ellipsoid semi-major axis
  - e2  = ellipsoid eccentricity squared


  meridional radius of curvature (p. 16)
  \verbatim
                        2
               a ( 1 - e )
       M = ------------------
                 2   2    3/2
           (1 - e sin lon)
  \endverbatim
  transverse radius of curvature (p. 16)
  \verbatim
                    a
       N = ------------------
                 2   2    1/2
           (1 - e sin lon)
  \endverbatim
  radius of the tangent sphere onto which angles are mapped
  conformally (p. 24)
  \verbatim
       R = sqrt ( N * M )
  \endverbatim
  
  (C) 2001-2009 by the GRASS Development Team

  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.

  \author CERL
 */

#include <math.h>
#include <grass/gis.h>
#include "pi.h"

/*!
 * \brief Meridional radius of curvature
 *
 * Returns the meridional radius of curvature at a given longitude:
 *
 \f$
 \rho = \frac{a (1-e^2)}{(1-e^2\sin^2 lon)^{3/2}}
 \f$
 *
 *  \param lon longitude
 *  \param a ellipsoid semi-major axis
 *  \param e2 ellipsoid eccentricity squared
 *
 *  \return radius value
 */
double G_meridional_radius_of_curvature(double lon, double a, double e2)
{
    double x;
    double s;

    s = sin(Radians(lon));
    x = 1 - e2 * s * s;

    return a * (1 - e2) / (x * sqrt(x));
}

/*!
 * \brief Transverse radius of curvature
 *
 * Returns the transverse radius of curvature at a given longitude:
 *
 \f$
 \nu = \frac{a}{(1-e^2\sin^2 lon)^{1/2}}
 \f$
 * 
 *  \param lon longitude
 *  \param a ellipsoid semi-major axis
 *  \param e2 ellipsoid eccentricity squared
 *
 *  \return radius value
 */
double G_transverse_radius_of_curvature(double lon, double a, double e2)
{
    double x;
    double s;

    s = sin(Radians(lon));
    x = 1 - e2 * s * s;

    return a / sqrt(x);
}

/*!
 * \brief Radius of conformal tangent sphere
 *
 * Returns the radius of the conformal sphere tangent to ellipsoid at
 * a given longitude:
 *
 \f$
 r = \frac{a (1-e^2)^{1/2}}{(1-e^2\sin^2 lon)}
 \f$
 * 
 *  \param lon longitude
 *  \param a ellipsoid semi-major axis
 *  \param e2 ellipsoid eccentricity squared
 *
 *  \return radius value
 */
double G_radius_of_conformal_tangent_sphere(double lon, double a, double e2)
{
    double x;
    double s;

    s = sin(Radians(lon));
    x = 1 - e2 * s * s;

    return a * sqrt(1 - e2) / x;
}
