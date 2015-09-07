
/*!
 * \file func2d.c
 * 
 * \author
 * Lubos Mitas (original program and various modifications)
 *
 * \author
 * H. Mitasova,
 * I. Kosinovsky, D. Gerdes,
 * D. McCauley
 * (GRASS4.1 version of the program and GRASS4.2 modifications)
 *
 * \author
 * L. Mitas ,
 * H. Mitasova ,
 * I. Kosinovsky, 
 * D.Gerdes 
 * D. McCauley (1993, 1995)
 *
 * \author modified by McCauley in August 1995
 * \author modified by Mitasova in August 1995, Nov. 1996
 *
 * \copyright
 * (C) 1993-1999 by Lubos Mitas and the GRASS Development Team
 *
 * \copyright
 * This program is free software under the
 * GNU General Public License (>=v2).
 * Read the file COPYING that comes with GRASS
 * for details.
 */


#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/interpf.h>


/* parameter description from DESCRIPTION.INTERP */
/*!
 * Radial basis function
 *
 * Radial basis function - completely regularized spline with tension (d=2)
 *
 */
double IL_crst(double r,  /**< distance squared */
               double fi  /**< tension */
               )
{
    double rfsta2 = fi * fi * r / 4.;

    static double c[4] = { 8.5733287401, 18.0590169730, 8.6347608925,
	0.2677737343
    };
    static double b[4] = { 9.5733223454, 25.6329561486, 21.0996530827,
	3.9584969228
    };
    double ce = 0.57721566;

    static double u[10] = { 1.e+00, -.25e+00,
	.055555555555556e+00, -.010416666666667e+00,	/*fixed bug 415.. repl. by 416.. */
	.166666666666667e-02, -2.31481481481482e-04,
	2.83446712018141e-05, -3.10019841269841e-06,
	3.06192435822065e-07, -2.75573192239859e-08
    };
    double x = rfsta2;
    double res;

    double e1, ea, eb;


    if (x < 1.e+00) {
	res = x * (u[0] + x * (u[1] + x * (u[2] + x * (u[3] + x * (u[4] + x *
								   (u[5] +
								    x *
								    (u[6] +
								     x *
								     (u[7] +
								      x *
								      (u[8] +
								       x *
								       u
								       [9])))))))));
	return (res);
    }

    if (x > 25.e+00)
	e1 = 0.00;
    else {
	ea = c[3] + x * (c[2] + x * (c[1] + x * (c[0] + x)));
	eb = b[3] + x * (b[2] + x * (b[1] + x * (b[0] + x)));
	e1 = (ea / eb) / (x * exp(x));
    }
    res = e1 + ce + log(x);
    return (res);
}


/*!
 * Function for calculating derivatives (d=2)
 *
 * Derivatives of radial basis function - regularized spline with tension(d=2)
 */
int IL_crstg(double r,  /**< distance squared */
             double fi,  /**< tension */
             double *gd1,  /**< G1(r) */
             double *gd2  /**< G2(r) */
             )
{
    double r2 = r;
    double rfsta2 = fi * fi * r / 4.;
    double x, exm, oneme, hold;
    double fsta2 = fi * fi / 2.;

    x = rfsta2;
    if (x < 0.001) {
	*gd1 = 1. - x / 2. + x * x / 6. - x * x * x / 24.;
	*gd2 = fsta2 * (-.5 + x / 3. - x * x / 8. + x * x * x / 30.);
    }
    else {
	if (x < 35.e+00) {
	    exm = exp(-x);
	    oneme = 1. - exm;
	    *gd1 = oneme / x;
	    hold = x * exm - oneme;
	    *gd2 = (hold + hold) / (r2 * x);
	}
	else {
	    *gd1 = 1. / x;
	    *gd2 = -2. / (x * r2);
	}
    }
    return 1;
}
