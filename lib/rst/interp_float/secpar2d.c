
/*!
 * \file secpar2d.c
 *
 * \author H. Mitasova, L. Mitas, I. Kosinovsky, D. Gerdes Fall 1994 (original authors)
 * \author modified by McCauley in August 1995
 * \author modified by Mitasova in August 1995
 * \author H. Mitasova (University of Illinois)
 * \author L. Mitas (University of Illinois)
 * \author I. Kosinovsky, (USA-CERL)
 * \author D.Gerdes (USA-CERL)   
 *
 * \copyright
 * (C) 1994-1995 by Helena Mitasova and the GRASS Development Team
 *
 * \copyright
 * This program is free software under the
 * GNU General Public License (>=v2).
 * Read the file COPYING that comes with GRASS
 * for details.
 *
 */


#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/bitmap.h>
#include <grass/interpf.h>


/*!
 * Compute slope aspect and curvatures
 *
 * Computes slope, aspect and curvatures (depending on cond1, cond2) for
 * derivative arrays adx,...,adxy between columns ngstc and nszc.
 */
int IL_secpar_loop_2d(struct interp_params *params,
                      int ngstc,  /*!< starting column */
                      int nszc,  /*!< ending column */
                      int k,  /*!< current row */
                      struct BM *bitmask,
                      double *gmin, double *gmax,
                      double *c1min, double *c1max,
                      double *c2min, double *c2max,  /*!< min,max interp. values */
                      int cond1,
                      int cond2  /*!< determine if particular values need to be computed */
    )
{
    double dnorm1, ro,		/* rad to deg conv */
      dx2 = 0, dy2 = 0, grad2 = 0,	/* gradient squared */
	slp = 0, grad,		/* gradient */
	oor = 0,		/* aspect  (orientation) */
	curn = 0,		/* profile curvature */
	curh = 0,		/* tangential curvature */
	curm = 0,		/* mean curvature */
	temp,			/* temp  variable */
	dxy2;			/* temp variable   square of part diriv. */

    double gradmin;
    int i, got, bmask = 1;
    static int first_time_g = 1;

    ro = M_R2D;
    gradmin = 0.001;


    for (i = ngstc; i <= nszc; i++) {
	if (bitmask != NULL) {
	    bmask = BM_get(bitmask, i, k);
	}
	got = 0;
	if (bmask == 1) {
	    while ((got == 0) && (cond1)) {
		dx2 = (double)(params->adx[i] * params->adx[i]);
		dy2 = (double)(params->ady[i] * params->ady[i]);
		grad2 = dx2 + dy2;
		grad = sqrt(grad2);
		/* slope in %        slp = 100. * grad; */
		/* slope in degrees */
		slp = ro * atan(grad);
		if (grad <= gradmin) {
		    oor = 0.;
		    got = 3;
		    if (cond2) {
			curn = 0.;
			curh = 0.;
			got = 3;
			break;
		    }
		}
		if (got == 3)
		    break;

	/***********aspect from r.slope.aspect, with adx, ady computed
	            from interpol. function RST **************************/

		if (params->adx[i] == 0.) {
		    if (params->ady[i] > 0.)
			oor = 90;
		    else
			oor = 270;
		}
		else {
		    oor = ro * atan2(params->ady[i], params->adx[i]);
		    if (oor <= 0.)
			oor = 360. + oor;
		}

		got = 1;
	    }			/* while */
	    if ((got != 3) && (cond2)) {

		dnorm1 = sqrt(grad2 + 1.);
		dxy2 =
		    2. * (double)(params->adxy[i] * params->adx[i] *
				  params->ady[i]);


		curn =
		    (double)(params->adxx[i] * dx2 + dxy2 +
			     params->adyy[i] * dy2) / (grad2 * dnorm1 *
						       dnorm1 * dnorm1);

		curh =
		    (double)(params->adxx[i] * dy2 - dxy2 +
			     params->adyy[i] * dx2) / (grad2 * dnorm1);

		temp = grad2 + 1.;
		curm =
		    .5 * ((1. + dy2) * params->adxx[i] - dxy2 +
			  (1. + dx2) * params->adyy[i]) / (temp * dnorm1);
	    }
	    if (first_time_g) {
		first_time_g = 0;
		*gmin = *gmax = slp;
		*c1min = *c1max = curn;
		*c2min = *c2max = curh;
	    }
	    *gmin = amin1(*gmin, slp);
	    *gmax = amax1(*gmax, slp);
	    *c1min = amin1(*c1min, curn);
	    *c1max = amax1(*c1max, curn);
	    *c2min = amin1(*c2min, curh);
	    *c2max = amax1(*c2max, curh);
	    if (cond1) {
		params->adx[i] = (FCELL) slp;
		params->ady[i] = (FCELL) oor;
		if (cond2) {
		    params->adxx[i] = (FCELL) curn;
		    params->adyy[i] = (FCELL) curh;
		    params->adxy[i] = (FCELL) curm;
		}
	    }
	}			/* bmask == 1 */
    }
    return 1;
}
