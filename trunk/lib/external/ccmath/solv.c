/*  solv.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU general
 *  public license. ( See the gpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include <stdlib.h>
#include "ccmath.h"
int solv(double *a, double *b, int n)
{
    int i, j, k, lc;

    double *ps, *p, *q, *pa, *pd;

    double *q0, s, t, tq = 0., zr = 1.e-15;

    q0 = (double *)calloc(n, sizeof(double));
    for (j = 0, pa = a, pd = a; j < n; ++j, ++pa, pd += n + 1) {
	if (j) {
	    for (i = 0, q = q0, p = pa; i < n; ++i, p += n)
		*q++ = *p;
	    for (i = 1; i < n; ++i) {
		lc = i < j ? i : j;
		for (k = 0, p = pa + i * n - j, q = q0, t = 0.; k < lc; ++k)
		    t += *p++ * *q++;
		q0[i] -= t;
	    }
	    for (i = 0, q = q0, p = pa; i < n; ++i, p += n)
		*p = *q++;
	}
	s = fabs(*pd);
	lc = j;
	for (k = j + 1, ps = pd; k < n; ++k) {
	    if ((t = fabs(*(ps += n))) > s) {
		s = t;
		lc = k;
	    }
	}
	tq = tq > s ? tq : s;
	if (s < zr * tq) {
	    free(q0);
	    return -1;
	}
	if (lc != j) {
	    t = b[j];
	    b[j] = b[lc];
	    b[lc] = t;
	    for (k = 0, p = a + n * j, q = a + n * lc; k < n; ++k) {
		t = *p;
		*p++ = *q;
		*q++ = t;
	    }
	}
	for (k = j + 1, ps = pd, t = 1. / *pd; k < n; ++k)
	    *(ps += n) *= t;
    }
    for (j = 1, ps = b + 1; j < n; ++j) {
	for (k = 0, p = a + n * j, q = b, t = 0.; k < j; ++k)
	    t += *p++ * *q++;
	*ps++ -= t;
    }
    for (j = n - 1, --ps, pd = a + n * n - 1; j >= 0; --j, pd -= n + 1) {
	for (k = j + 1, p = pd, q = b + j, t = 0.; k < n; ++k)
	    t += *++p * *++q;
	*ps -= t;
	*ps-- /= *pd;
    }
    free(q0);
    return 0;
}
