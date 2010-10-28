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
    int i, j;
    double uweight, vweight[5], d;
    DCELL result = 0;

    for (i = 0; i < 5; i++) {
	d = u - i + 2;
	if (d == 0)
	    uweight = 1;
	else {
	    d *= M_PI;
	    uweight = LANCZOS_FILTER(d);
	}

	for (j = 0; j < 5; j++) {
	    if (i == 0) {
		d = v - j + 2;
		if (d == 0)
		    vweight[j] = 1;
		else {
		    d *= M_PI;
		    vweight[j] = LANCZOS_FILTER(d);
		}
	    }

	    result += *(c++) * uweight * vweight[j];
	}
    }
    
    return result;
}

DCELL Rast_interp_cubic_bspline(double u, DCELL c0, DCELL c1, DCELL c2, DCELL c3)
{
    return (u * (u * (u * ( 1 * c3 - 3 * c2 + 3 * c1 - 1 * c0) +
			  ( 0 * c3 + 3 * c2 - 6 * c1 + 3 * c0)) +
			  ( 0 * c3 + 3 * c2 + 0 * c1 - 3 * c0)) +
			    0 * c3 + 1 * c2 + 4 * c1 + 1 * c0) / 6;
}

DCELL Rast_interp_bicubic_bspline(double u, double v,
			  DCELL c00, DCELL c01, DCELL c02, DCELL c03,
			  DCELL c10, DCELL c11, DCELL c12, DCELL c13,
			  DCELL c20, DCELL c21, DCELL c22, DCELL c23,
			  DCELL c30, DCELL c31, DCELL c32, DCELL c33)
{
    DCELL c0 = Rast_interp_bspline_cubic(u, c00, c01, c02, c03);
    DCELL c1 = Rast_interp_bspline_cubic(u, c10, c11, c12, c13);
    DCELL c2 = Rast_interp_bspline_cubic(u, c20, c21, c22, c23);
    DCELL c3 = Rast_interp_bspline_cubic(u, c30, c31, c32, c33);

    return Rast_interp_bspline_cubic(v, c0, c1, c2, c3);
}
