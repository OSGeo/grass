/*  svdval.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include <stdlib.h>
#include "ccmath.h"
int svdval(double *d, double *a, int m, int n)
{
    double *p, *p1, *q, *w, *v;

    double s, h, u;

    int i, j, k, mm, nm, ms;

    if (m < n)
	return -1;
    w = (double *)calloc(m, sizeof(double));
    for (i = 0, mm = m, nm = n - 1, p = a; i < n; ++i, --mm, --nm, p += n + 1) {
	if (mm > 1) {
	    for (j = 0, q = p, s = 0.; j < mm; ++j, q += n) {
		w[j] = *q;
		s += *q * *q;
	    }
	    if (s > 0.) {
		h = sqrt(s);
		if (*p < 0.)
		    h = -h;
		s += *p * h;
		s = 1. / s;
		w[0] += h;
		for (k = 1, ms = n - i; k < ms; ++k) {
		    for (j = 0, q = p + k, u = 0.; j < mm; q += n)
			u += w[j++] * *q;
		    u *= s;
		    for (j = 0, q = p + k; j < mm; q += n)
			*q -= u * w[j++];
		}
		*p = -h;
	    }
	}
	p1 = p + 1;
	if (nm > 1) {
	    for (j = 0, q = p1, s = 0.; j < nm; ++j, ++q)
		s += *q * *q;
	    if (s > 0.) {
		h = sqrt(s);
		if (*p1 < 0.)
		    h = -h;
		s += *p1 * h;
		s = 1. / s;
		*p1 += h;
		for (k = n, ms = n * (m - i); k < ms; k += n) {
		    for (j = 0, q = p1, v = p1 + k, u = 0.; j < nm; ++j)
			u += *q++ * *v++;
		    u *= s;
		    for (j = 0, q = p1, v = p1 + k; j < nm; ++j)
			*v++ -= u * *q++;
		}
		*p1 = -h;
	    }
	}
    }

    for (j = 0, p = a; j < n; ++j, p += n + 1) {
	d[j] = *p;
	if (j != n - 1)
	    w[j] = *(p + 1);
	else
	    w[j] = 0.;
    }
    qrbdi(d, w, n);
    for (i = 0; i < n; ++i)
	if (d[i] < 0.)
	    d[i] = -d[i];
    free(w);
    return 0;
}
