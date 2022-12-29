/*!
  \file lib/gis/pole_in_poly.c

  \brief GIS Library - Pole in polygon
  
  (C) 2001-2009 by the GRASS Development Team

  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.

  \author CERL
 */

#include <grass/gis.h>

static void mystats(double, double, double, double, double *, double *);

/*!
 * \brief Check if pole is in polygon
 *
 * For latitude-longitude coordinates, this routine determines if the polygon
 * defined by the <i>n</i> coordinate vertices <i>x,y</i> contains one of the
 * poles.
 *
 * <b>Note:</b> Use this routine only if the projection is PROJECTION_LL.
 *
 *  \param x array of x coordinates
 *  \param y array of y coordinates
 *  \param n number of coordinates
 * 
 * \return -1 if it contains the south pole
 * \return 1 if it contains the north pole
 * \return 0 if it contains neither pole.
 */
int G_pole_in_polygon(const double *x, const double *y, int n)
{
    int i;
    double len, area, total_len, total_area;

    if (n <= 1)
	return 0;

    mystats(x[n - 1], y[n - 1], x[0], y[0], &total_len, &total_area);
    for (i = 1; i < n; i++) {
	mystats(x[i - 1], y[i - 1], x[i], y[i], &len, &area);
	total_len += len;
	total_area += area;
    }

    /* if polygon contains a pole then the x-coordinate length of
     * the perimeter should compute to 0, otherwise it should be about 360
     * (or -360, depending on the direction of perimeter traversal)
     *
     * instead of checking for exactly 0, check from -1 to 1 to avoid
     * roundoff error.
     */
    if (total_len < 1.0 && total_len > -1.0)
	return 0;

    return total_area >= 0.0 ? 1 : -1;
}

static void mystats(double x0, double y0, double x1, double y1, double *len,
		    double *area)
{
    if (x1 > x0)
	while (x1 - x0 > 180)
	    x0 += 360;
    else if (x0 > x1)
	while (x0 - x1 > 180)
	    x0 -= 360;

    *len = x0 - x1;

    if (x0 > x1)
	*area = (x0 - x1) * (y0 + y1) / 2.0;
    else
	*area = (x1 - x0) * (y1 + y0) / 2.0;
}
