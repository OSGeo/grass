/*  atou1.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include <stdlib.h>
void atou1(double *a, int m, int n)
{
    double *p0, *p, *q, *w;

    int i, j, k, mm;

    double s, h;

    w = (double *)calloc(m, sizeof(double));
    p0 = a + n * n - 1;
    i = n - 1;
    mm = m - n;
    if (mm == 0) {
	*p0 = 1.;
	p0 -= n + 1;
	--i;
	++mm;
    }
    for (; i >= 0; --i, ++mm, p0 -= n + 1) {
	if (*p0 != 0.) {
	    for (j = 0, p = p0 + n; j < mm; p += n)
		w[j++] = *p;
	    h = *p0;
	    *p0 = 1. - h;
	    for (j = 0, p = p0 + n; j < mm; p += n)
		*p = -h * w[j++];
	    for (k = i + 1, q = p0 + 1; k < n; ++k) {
		for (j = 0, p = q + n, s = 0.; j < mm; p += n)
		    s += w[j++] * *p;
		s *= h;
		for (j = 0, p = q + n; j < mm; p += n)
		    *p -= s * w[j++];
		*q++ = -s;
	    }
	}
	else {
	    *p0 = 1.;
	    for (j = 0, p = p0 + n, q = p0 + 1; j < mm; ++j, p += n)
		*p = *q++ = 0.;
	}
    }
    free(w);
}
