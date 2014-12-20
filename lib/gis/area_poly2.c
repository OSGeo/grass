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
    double x1, y1, x2, y2;
    double area;

    x2 = x[n - 1];
    y2 = y[n - 1];

    area = 0;
    while (--n >= 0) {
	x1 = x2;
	y1 = y2;

	x2 = *x++;
	y2 = *y++;

	area += (y2 + y1) * (x2 - x1);
    }

    if ((area /= 2.0) < 0.0)
	area = -area;

    return area;
}
