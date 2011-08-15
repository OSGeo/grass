/* min/brent.c
 * 
 * Copyright (C) 1996, 1997, 1998, 1999, 2000 Brian Gough
 *
 * Taken from 'GSL - The GNU Scientific Library':
 *             "One dimensional Minimization"
 *             http://sources.redhat.com/gsl/
 * modified by Stefano Menegon 2004
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#define GSL_SQRT_DBL_EPSILON   1.e-4
#define GSL_DBL_EPSILON  1.e-8

/*
   #define SAFE_FUNC_CALL(f, x, yp) \
   do { \
   *yp = GSL_FN_EVAL(f,x); \
   if (!finite(*yp)) \
   fprintf(stderr,"function not continuous\n");\
   } while (0)
 */


typedef struct
{
    double d, e, v, w;
    double f_v, f_w;
}
brent_state_t;



static int
brent(void *vstate, double (*f) (), double *x_minimum, double *f_minimum,
      double *x_lower, double *f_lower, double *x_upper, double *f_upper)
{
    brent_state_t *state = (brent_state_t *) vstate;

    const double x_left = *x_lower;
    const double x_right = *x_upper;

    const double z = *x_minimum;
    double d = state->e;
    double e = state->d;
    double u, f_u;
    const double v = state->v;
    const double w = state->w;
    const double f_v = state->f_v;
    const double f_w = state->f_w;
    const double f_z = *f_minimum;

    const double golden = 0.3819660;	/* golden = (3 - sqrt(5))/2 */

    const double w_lower = (z - x_left);
    const double w_upper = (x_right - z);

    const double tolerance = GSL_SQRT_DBL_EPSILON * fabs(z);

    double p = 0, q = 0, r = 0;

    const double midpoint = 0.5 * (x_left + x_right);

    if (fabs(e) > tolerance) {
	/* fit parabola */

	r = (z - w) * (f_z - f_v);
	q = (z - v) * (f_z - f_w);
	p = (z - v) * q - (z - w) * r;
	q = 2 * (q - r);

	if (q > 0) {
	    p = -p;
	}
	else {
	    q = -q;
	}

	r = e;
	e = d;
    }

    if (fabs(p) < fabs(0.5 * q * r) && p < q * w_lower && p < q * w_upper) {
	double t2 = 2 * tolerance;

	d = p / q;
	u = z + d;

	if ((u - x_left) < t2 || (x_right - z) < t2) {
	    d = (z < midpoint) ? tolerance : -tolerance;
	}
    }
    else {
	e = (z < midpoint) ? x_right - z : -(z - x_left);
	d = golden * e;
    }


    if (fabs(d) >= tolerance) {
	u = z + d;
    }
    else {
	u = z + ((d > 0) ? tolerance : -tolerance);
    }

    state->e = e;
    state->d = d;

    /*  SAFE_FUNC_CALL(f, u, &f_u); */
    f_u = (*f) (u);

    if (f_u > f_z) {
	if (u < z) {
	    *x_lower = u;
	    *f_lower = f_u;
	    return 0;
	}
	else {
	    *x_upper = u;
	    *f_upper = f_u;
	    return 0;
	}
    }
    else if (f_u < f_z) {
	if (u < z) {
	    *x_upper = z;
	    *f_upper = f_z;
	}
	else {
	    *x_lower = z;
	    *f_lower = f_z;
	}

	state->v = w;
	state->f_v = f_w;
	state->w = z;
	state->f_w = f_z;
	*x_minimum = u;
	*f_minimum = f_u;
	return 0;
    }
    else if (f_u <= f_w || w == z) {
	state->v = w;
	state->f_v = f_w;
	state->w = u;
	state->f_w = f_u;
	return 0;
    }
    else if (f_u <= f_v || v == z || v == w) {
	state->v = u;
	state->f_v = f_u;
	return 0;
    }
    else {
	return -1;
    }
}


/* Code modified by Stefano Menegon 1st of February 2004 */

double brent_iterate(double (*f) (), double x_lower, double x_upper,
		     int maxiter)
{
    int i;
    double x_minimum = (x_upper + x_lower) / 2.;
    double f_minimum;
    double f_lower;
    double f_upper;

    /* stato iterazione */
    brent_state_t itstate;
    const double golden = 0.3819660;	/* golden = (3 - sqrt(5))/2 */
    double v = x_lower + golden * (x_upper - x_lower);
    double w = v;
    double f_vw;

    f_lower = (*f) (x_lower);
    f_upper = (*f) (x_upper);
    f_minimum = (*f) (x_minimum);

    itstate.v = v;
    itstate.w = w;

    itstate.d = 0.;
    itstate.e = 0.;

    /*  SAFE_FUNC_CALL (f, v, &f_vw); */

    f_vw = (*f) (v);

    itstate.f_v = f_vw;
    itstate.f_w = f_vw;

    for (i = 0; i < maxiter; i++) {
	brent(&itstate, f, &x_minimum, &f_minimum, &x_lower, &f_lower,
	      &x_upper, &f_upper);
	if (fabs(f_upper - f_lower) < GSL_DBL_EPSILON * fabs(f_minimum)) {
	    return x_minimum;
	}
    }

    return x_minimum;
}
