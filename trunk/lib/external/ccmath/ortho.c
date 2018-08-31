/*  ortho.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include "ccmath.h"
static double tpi = 6.28318530717958647;

void ortho(double *e, int n)
{
    int i, j, k, m;

    double *p, *q, c, s, a, unfl();

    for (i = 0, p = e; i < n; ++i) {
	for (j = 0; j < n; ++j) {
	    if (i == j)
		*p++ = 1.;
	    else
		*p++ = 0.;
	}
    }
    for (i = 0, m = n - 1; i < m; ++i) {
	for (j = i + 1; j < n; ++j) {
	    a = tpi * unfl();
	    c = cos(a);
	    s = sin(a);
	    p = e + n * i;
	    q = e + n * j;
	    for (k = 0; k < n; ++k) {
		a = *p * c + *q * s;
		*q = *q * c - *p * s;
		*p++ = a;
		++q;
	    }
	}
    }
}
