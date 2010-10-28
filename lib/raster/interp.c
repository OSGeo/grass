/*!
 * \file raster/interp.c
 *
 * \brief Raster Library - Interpolation
 *
 * (C) 2001-2009 GRASS Development Team
 *
 * This program is free software under the GNU General Public License 
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>

#define LANCZOS_FILTER(x) ((2 * sin(x) * sin((x) / 2)) / ((x) * (x)))

DCELL Rast_interp_linear(double u, DCELL c0, DCELL c1)
{
    return u * (c1 - c0) + c0;
}

DCELL Rast_interp_bilinear(double u, double v,
			   DCELL c00, DCELL c01, DCELL c10, DCELL c11)
{
    DCELL c0 = Rast_interp_linear(u, c00, c01);
    DCELL c1 = Rast_interp_linear(u, c10, c11);

    return Rast_interp_linear(v, c0, c1);
}

DCELL Rast_interp_cubic(double u, DCELL c0, DCELL c1, DCELL c2, DCELL c3)
{
    return (u * (u * (u * (c3 - 3 * c2 + 3 * c1 - c0) +
		      (-c3 + 4 * c2 - 5 * c1 + 2 * c0)) + (c2 - c0)) +
	    2 * c1) / 2;
}

DCELL Rast_interp_bicubic(double u, double v,
			  DCELL c00, DCELL c01, DCELL c02, DCELL c03,
			  DCELL c10, DCELL c11, DCELL c12, DCELL c13,
			  DCELL c20, DCELL c21, DCELL c22, DCELL c23,
			  DCELL c30, DCELL c31, DCELL c32, DCELL c33)
{
    DCELL c0 = Rast_interp_cubic(u, c00, c01, c02, c03);
    DCELL c1 = Rast_interp_cubic(u, c10, c11, c12, c13);
    DCELL c2 = Rast_interp_cubic(u, c20, c21, c22, c23);
    DCELL c3 = Rast_interp_cubic(u, c30, c31, c32, c33);

    return Rast_interp_cubic(v, c0, c1, c2, c3);
}

DCELL Rast_interp_lanczos(double u, double v, DCELL *c)
{
    int i;
    double uweight[5], vweight[5], d;

    for (i = 0; i < 5; i++) {
	d = v - i + 2;
	if (d == 0)
	    vweight[i] = 1;
	else {
	    d *= M_PI;
	    vweight[i] = LANCZOS_FILTER(d);
	}
	d = u - i + 2;
	if (d == 0)
	    uweight[i] = 1;
	else {
	    d *= M_PI;
	    uweight[i] = LANCZOS_FILTER(d);
	}
    }

    return ((c[0] * vweight[0] + c[1] * vweight[1] + c[2] * vweight[2]
	    + c[3] * vweight[3] + c[4] * vweight[4]) * uweight[0] +
	    (c[5] * vweight[0] + c[6] * vweight[1] + c[7] * vweight[2]
	    + c[8] * vweight[3] + c[9] * vweight[4]) * uweight[1] + 
	    (c[10] * vweight[0] + c[11] * vweight[1] + c[12] * vweight[2]
	    + c[13] * vweight[3] + c[14] * vweight[4]) * uweight[2] + 
	    (c[15] * vweight[0] + c[16] * vweight[1] + c[17] * vweight[2]
	    + c[18] * vweight[3] + c[19] * vweight[4]) * uweight[3] + 
	    (c[20] * vweight[0] + c[21] * vweight[1] + c[22] * vweight[2]
	    + c[23] * vweight[3] + c[24] * vweight[4]) * uweight[4]);
}

DCELL Rast_interp_cubic_bspline(double u, DCELL c0, DCELL c1, DCELL c2, DCELL c3)
{
    return (u * (u * (u * (c3 - 3 * c2 + 3 * c1 - c0) +
			  (3 * c2 - 6 * c1 + 3 * c0)) +
			  (3 * c2 - 3 * c0)) +
			   c2 + 4 * c1 + c0) / 6;
}

DCELL Rast_interp_bicubic_bspline(double u, double v,
			  DCELL c00, DCELL c01, DCELL c02, DCELL c03,
			  DCELL c10, DCELL c11, DCELL c12, DCELL c13,
			  DCELL c20, DCELL c21, DCELL c22, DCELL c23,
			  DCELL c30, DCELL c31, DCELL c32, DCELL c33)
{
    DCELL c0 = Rast_interp_cubic_bspline(u, c00, c01, c02, c03);
    DCELL c1 = Rast_interp_cubic_bspline(u, c10, c11, c12, c13);
    DCELL c2 = Rast_interp_cubic_bspline(u, c20, c21, c22, c23);
    DCELL c3 = Rast_interp_cubic_bspline(u, c30, c31, c32, c33);

    return Rast_interp_cubic_bspline(v, c0, c1, c2, c3);
}
