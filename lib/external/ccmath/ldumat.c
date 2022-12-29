/*  ldumat.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include <stdlib.h>
void ldumat(double *a, double *u, int m, int n)
{
    double *p0, *q0, *p, *q, *w;

    int i, j, k, mm;

    double s, h;

    w = (double *)calloc(m, sizeof(double));
    for (i = 0, mm = m * m, q = u; i < mm; ++i)
	*q++ = 0.;
    p0 = a + n * n - 1;
    q0 = u + m * m - 1;
    mm = m - n;
    i = n - 1;
    for (j = 0; j < mm; ++j, q0 -= m + 1)
	*q0 = 1.;
    if (mm == 0) {
	p0 -= n + 1;
	*q0 = 1.;
	q0 -= m + 1;
	--i;
	++mm;
    }
    for (; i >= 0; --i, ++mm, p0 -= n + 1, q0 -= m + 1) {
	if (*p0 != 0.) {
	    for (j = 0, p = p0 + n, h = 1.; j < mm; p += n)
		w[j++] = *p;
	    h = *p0;
	    *q0 = 1. - h;
	    for (j = 0, q = q0 + m; j < mm; q += m)
		*q = -h * w[j++];
	    for (k = i + 1, q = q0 + 1; k < m; ++k) {
		for (j = 0, p = q + m, s = 0.; j < mm; p += m)
		    s += w[j++] * *p;
		s *= h;
		for (j = 0, p = q + m; j < mm; p += m)
		    *p -= s * w[j++];
		*q++ = -s;
	    }
	}
	else {
	    *q0 = 1.;
	    for (j = 0, p = q0 + 1, q = q0 + m; j < mm; ++j, q += m)
		*q = *p++ = 0.;
	}
    }
    free(w);
}
