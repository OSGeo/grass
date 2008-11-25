
/**
 * \file area_poly1.c
 *
 * \brief GIS Library - Polygon area calculation routines.
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


#define TWOPI M_PI + M_PI

static struct state {
    double QA, QB, QC;
    double QbarA, QbarB, QbarC, QbarD;
    double AE;  /** a^2(1-e^2) */
    double Qp;  /** Q at the north pole */
    double E;   /** Area of the earth */
} state;

static struct state *st = &state;

static double Q(double x)
{
    double sinx, sinx2;

    sinx = sin(x);
    sinx2 = sinx * sinx;

    return sinx * (1 + sinx2 * (st->QA + sinx2 * (st->QB + sinx2 * st->QC)));
}

static double Qbar(double x)
{
    double cosx, cosx2;

    cosx = cos(x);
    cosx2 = cosx * cosx;

    return cosx * (st->QbarA + cosx2 * (st->QbarB + cosx2 * (st->QbarC + cosx2 * st->QbarD)));
}


/**
 * \brief Begin area calculations.
 *
 * This initializes the polygon area calculations for the
 * ellipsoid with semi-major axis <b>a</b> (in meters) and ellipsoid
 * eccentricity squared <b>e2</b>.
 *
 * \param[in] a semi-major axis
 * \param[in] e2 ellipsoid eccentricity
 * \return
 */

void G_begin_ellipsoid_polygon_area(double a, double e2)
{
    double e4, e6;

    e4 = e2 * e2;
    e6 = e4 * e2;

    st->AE = a * a * (1 - e2);

    st->QA = (2.0 / 3.0) * e2;
    st->QB = (3.0 / 5.0) * e4;
    st->QC = (4.0 / 7.0) * e6;

    st->QbarA = -1.0 - (2.0 / 3.0) * e2 - (3.0 / 5.0) * e4 - (4.0 / 7.0) * e6;
    st->QbarB = (2.0 / 9.0) * e2 + (2.0 / 5.0) * e4 + (4.0 / 7.0) * e6;
    st->QbarC = -(3.0 / 25.0) * e4 - (12.0 / 35.0) * e6;
    st->QbarD = (4.0 / 49.0) * e6;

    st->Qp = Q(M_PI_2);
    st->E = 4 * M_PI * st->Qp * st->AE;
    if (st->E < 0.0)
	st->E = -st->E;
}


/**
 * \brief Area of lat-long polygon.
 *
 * Returns the area in square meters of the polygon described by the 
 * <b>n</b> pairs of <b>lat,long</b> vertices for latitude-longitude 
 * grids.
 * <br>
 * <b>Note:</b> This routine assumes grid lines on the connecting the 
 * vertices (as opposed to geodesics).
 *
 * \param[in] lon array of longitudes
 * \param[in] lat array of latitudes
 * \param[in] n number of lat,lon pairs
 * \return double Area in square meters
 */

double G_ellipsoid_polygon_area(const double *lon, const double *lat, int n)
{
    double x1, y1, x2, y2, dx, dy;
    double Qbar1, Qbar2;
    double area;

    x2 = Radians(lon[n - 1]);
    y2 = Radians(lat[n - 1]);
    Qbar2 = Qbar(y2);

    area = 0.0;

    while (--n >= 0) {
	x1 = x2;
	y1 = y2;
	Qbar1 = Qbar2;

	x2 = Radians(*lon++);
	y2 = Radians(*lat++);
	Qbar2 = Qbar(y2);

	if (x1 > x2)
	    while (x1 - x2 > M_PI)
		x2 += TWOPI;
	else if (x2 > x1)
	    while (x2 - x1 > M_PI)
		x1 += TWOPI;

	dx = x2 - x1;
	area += dx * (st->Qp - Q(y2));

	if ((dy = y2 - y1) != 0.0)
	    area += dx * Q(y2) - (dx / dy) * (Qbar2 - Qbar1);
    }
    if ((area *= st->AE) < 0.0)
	area = -area;

    /* kludge - if polygon circles the south pole the area will be
     * computed as if it cirlced the north pole. The correction is
     * the difference between total surface area of the earth and
     * the "north pole" area.
     */
    if (area > st->E)
	area = st->E;
    if (area > st->E / 2)
	area = st->E - area;

    return area;
}
