/*  svdu1v.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include <stdlib.h>
#include "ccmath.h"
int svdu1v(double *d, double *a, int m, double *v, int n)
{
    double *p, *p1, *q, *pp, *w, *e;

    double s, h, r, t, sv;

    int i, j, k, mm, nm, ms;

    if (m < n)
	return -1;
    w = (double *)calloc(m + n, sizeof(double));
    e = w + m;
    for (i = 0, mm = m, nm = n - 1, p = a; i < n; ++i, --mm, --nm, p += n + 1) {
	if (mm > 1) {
	    sv = h = 0.;
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
		t = 1. / (w[0] += h);
		sv = 1. + fabs(*p / h);
		for (k = 1, ms = n - i; k < ms; ++k) {
		    for (j = 0, q = p + k, r = 0.; j < mm; q += n)
			r += w[j++] * *q;
		    r *= s;
		    for (j = 0, q = p + k; j < mm; q += n)
			*q -= r * w[j++];
		}
		for (j = 1, q = p; j < mm;)
		    *(q += n) = t * w[j++];
	    }
	    *p = sv;
	    d[i] = -h;
	}
	if (mm == 1)
	    d[i] = *p;
	p1 = p + 1;
	sv = h = 0.;
	if (nm > 1) {
	    for (j = 0, q = p1, s = 0.; j < nm; ++j, ++q)
		s += *q * *q;
	    if (s > 0.) {
		h = sqrt(s);
		if (*p1 < 0.)
		    h = -h;
		sv = 1. + fabs(*p1 / h);
		s += *p1 * h;
		s = 1. / s;
		t = 1. / (*p1 += h);
		for (k = n, ms = n * (m - i); k < ms; k += n) {
		    for (j = 0, q = p1, pp = p1 + k, r = 0.; j < nm; ++j)
			r += *q++ * *pp++;
		    r *= s;
		    for (j = 0, q = p1, pp = p1 + k; j < nm; ++j)
			*pp++ -= r * *q++;
		}
		for (j = 1, q = p1 + 1; j < nm; ++j)
		    *q++ *= t;
	    }
	    *p1 = sv;
	    e[i] = -h;
	}
	if (nm == 1)
	    e[i] = *p1;
    }
    ldvmat(a, v, n);
    atou1(a, m, n);
    qrbdu1(d, e, a, m, v, n);
    for (i = 0; i < n; ++i) {
	if (d[i] < 0.) {
	    d[i] = -d[i];
	    for (j = 0, p = v + i; j < n; ++j, p += n)
		*p = -*p;
	}
    }
    free(w);
    return 0;
}
