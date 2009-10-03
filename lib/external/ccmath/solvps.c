/*  solvps.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include "ccmath.h"
int solvps(double *a, double *b, int n)
{
    double *p, *q, *r, *s, t;

    int j, k;

    for (j = 0, p = a; j < n; ++j, p += n + 1) {
	for (q = a + j * n; q < p; ++q)
	    *p -= *q * *q;
	if (*p <= 0.)
	    return -1;
	*p = sqrt(*p);
	for (k = j + 1, q = p + n; k < n; ++k, q += n) {
	    for (r = a + j * n, s = a + k * n, t = 0.; r < p;)
		t += *r++ * *s++;
	    *q -= t;
	    *q /= *p;
	}
    }
    for (j = 0, p = a; j < n; ++j, p += n + 1) {
	for (k = 0, q = a + j * n; k < j;)
	    b[j] -= b[k++] * *q++;
	b[j] /= *p;
    }
    for (j = n - 1, p = a + n * n - 1; j >= 0; --j, p -= n + 1) {
	for (k = j + 1, q = p + n; k < n; q += n)
	    b[j] -= b[k++] * *q;
	b[j] /= *p;
    }
    return 0;
}
