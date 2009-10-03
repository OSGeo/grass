/*  utrnhm.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include <stdlib.h>
#include "ccmath.h"
void utrnhm(Cpx * hm, Cpx * a, Cpx * b, int n)
{
    Cpx z, *q0, *p, *s, *t;

    int i, j, k;

    q0 = (Cpx *) calloc(n, sizeof(Cpx));
    for (i = 0; i < n; ++i) {
	for (j = 0, t = b; j < n; ++j) {
	    z.re = z.im = 0.;
	    for (k = 0, s = a + i * n; k < n; ++k, ++s, ++t) {
		z.re += t->re * s->re + t->im * s->im;
		z.im += t->im * s->re - t->re * s->im;
	    }
	    q0[j] = z;
	}
	for (j = 0, p = hm + i, t = a; j <= i; ++j, p += n) {
	    z.re = z.im = 0.;
	    for (k = 0, s = q0; k < n; ++k, ++t, ++s) {
		z.re += t->re * s->re - t->im * s->im;
		z.im += t->im * s->re + t->re * s->im;
	    }
	    *p = z;
	    if (j < i) {
		z.im = -z.im;
		hm[i * n + j] = z;
	    }
	}
    }
    free(q0);
}
