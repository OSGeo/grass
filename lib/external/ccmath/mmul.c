/*  mmul.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include "ccmath.h"
void mmul(double *c, double *a, double *b, int n)
{
    double *p, *q, s;

    int i, j, k;

    trnm(b, n);
    for (i = 0; i < n; ++i, a += n) {
	for (j = 0, q = b; j < n; ++j) {
	    for (k = 0, p = a, s = 0.; k < n; ++k)
		s += *p++ * *q++;
	    *c++ = s;
	}
    }
    trnm(b, n);
}
