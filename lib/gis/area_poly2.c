/*!
 * \file lib/gis/area_poly2.c
 *
 * \brief GIS Library - Planimetric polygon area calculation routines.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <grass/gis.h>

/*!
 * \brief Calculates planimetric polygon area.
 *
 * \param x array of x values
 * \param y array of y values
 * \param n number of x,y pairs

 * \return polygon area in map units
 */
double G_planimetric_polygon_area(const double *x, const double *y, int n)
{
    double x1, y1, x2, y2, xshift, yshift;
    double area;

    /* Get a reference point close to the polygon as new origin.
     * The first point would be good enough, particularly for small
     * polygons. The average of the first and the mid-point is a fast
     * approximation of a reference point with reduced distance to the
     * ring's vertices, also for larger rings.
     *
     * Shift coordinates towards this reference point to make
     * calculation of the signed area more robust by increasing the
     * accuracy for given fp precision limits.
     *
     * Considering the basic formula
     *    area += (x2 - x1) * (y2 + y1)
     * the shift is in theory only needed for addition, not subtraction,
     * but does no harm and is kept for symmetry treating x and y coords.
     *
     * Keep in sync with dig_find_area_poly() in lib/vector/diglib/poly.c */

    xshift = (x[0] + x[n / 2]) / 2.;
    yshift = (y[0] + y[n / 2]) / 2.;

    x2 = x[n - 1] - xshift;
    y2 = y[n - 1] - yshift;

    area = 0;
    while (--n >= 0) {
        x1 = x2;
        y1 = y2;

        x2 = *x++;
        y2 = *y++;

        x2 -= xshift;
        y2 -= yshift;

        area += (y2 + y1) * (x2 - x1);
    }

    if ((area /= 2.0) < 0.0)
        area = -area;

    return area;
}
