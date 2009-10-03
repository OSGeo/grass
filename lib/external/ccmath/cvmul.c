/*  cvmul.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include "ccmath.h"
void cvmul(Cpx * u, Cpx * a, Cpx * v, int n)
{
    Cpx *q;

    int i, j;

    for (i = 0; i < n; ++i, ++u) {
	u->re = u->im = 0.;
	for (j = 0, q = v; j < n; ++j, ++a, ++q) {
	    u->re += a->re * q->re - a->im * q->im;
	    u->im += a->im * q->re + a->re * q->im;
	}
    }
}

Cpx cvnrm(Cpx * u, Cpx * v, int n)
{
    int k;

    Cpx z;

    z.re = z.im = 0.;
    for (k = 0; k < n; ++k, ++u, ++v) {
	z.re += u->re * v->re + u->im * v->im;
	z.im += u->re * v->im - u->im * v->re;
    }
    return z;
}
