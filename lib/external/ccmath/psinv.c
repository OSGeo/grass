/*  psinv.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include "ccmath.h"
int psinv(double *v, int n)
{
    double z, *p, *q, *r, *s, *t;

    int j, k;

    for (j = 0, p = v; j < n; ++j, p += n + 1) {
	for (q = v + j * n; q < p; ++q)
	    *p -= *q * *q;
	if (*p <= 0.)
	    return -1;
	*p = sqrt(*p);
	for (k = j + 1, q = p + n; k < n; ++k, q += n) {
	    for (r = v + j * n, s = v + k * n, z = 0.; r < p;)
		z += *r++ * *s++;
	    *q -= z;
	    *q /= *p;
	}
    }
    trnm(v, n);
    for (j = 0, p = v; j < n; ++j, p += n + 1) {
	*p = 1. / *p;
	for (q = v + j, t = v; q < p; t += n + 1, q += n) {
	    for (s = q, r = t, z = 0.; s < p; s += n)
		z -= *s * *r++;
	    *q = z * *p;
	}
    }
    for (j = 0, p = v; j < n; ++j, p += n + 1) {
	for (q = v + j, t = p - j; q <= p; q += n) {
	    for (k = j, r = p, s = q, z = 0.; k < n; ++k)
		z += *r++ * *s++;
	    *t++ = (*q = z);
	}
    }
    return 0;
}
