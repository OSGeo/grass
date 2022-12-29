/*  hevmax.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include <stdlib.h>
#include "ccmath.h"
double hevmax(Cpx * a, Cpx * u, int n)
{
    Cpx *x, *p, h;

    double e, ep, s, t, te = 1.e-12;

    int k, j;

    x = (Cpx *) calloc(n, sizeof(Cpx));
    x[0].re = 1.;
    e = 0.;
    do {
	for (k = 0, p = a, s = t = 0.; k < n; ++k) {
	    for (j = 0, h.re = h.im = 0.; j < n; ++j, ++p) {
		h.re += p->re * x[j].re - p->im * x[j].im;
		h.im += p->im * x[j].re + p->re * x[j].im;
	    }
	    s += h.re * h.re + h.im * h.im;
	    t += h.re * x[k].re + h.im * x[k].im;
	    u[k] = h;
	}
	ep = e;
	e = s / t;
	s = 1. / sqrt(s);
	for (k = 0; k < n; ++k) {
	    u[k].re *= s;
	    u[k].im *= s;
	    x[k] = u[k];
	}
    } while (fabs(e - ep) > fabs(te * e));
    free(x);
    return e;
}
