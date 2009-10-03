/*  sv2uv.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include <stdlib.h>
#include "ccmath.h"
int sv2uv(double *d, double *a, double *u, int m, double *v, int n)
{
    double *p, *p1, *q, *pp, *w, *e;

    double s, t, h, r, sv;

    int i, j, k, mm, nm, ms;

    if (m < n)
	return -1;
    w = (double *)calloc(m + n, sizeof(double));
    e = w + m;
    for (i = 0, mm = m, p = a; i < n; ++i, --mm, p += n + 1) {
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
		    r = r * s;
		    for (j = 0, q = p + k; j < mm; q += n)
			*q -= r * w[j++];
		}
		for (j = 1, q = p; j < mm;)
		    *(q += n) = w[j++] * t;
	    }
	    *p = sv;
	    d[i] = -h;
	}
	if (mm == 1)
	    d[i] = *p;
    }
    ldumat(a, u, m, n);
    for (i = 0, q = a; i < n; ++i) {
	for (j = 0; j < n; ++j, ++q) {
	    if (j < i)
		*q = 0.;
	    else if (j == i)
		*q = d[i];
	}
    }
    for (i = 0, mm = n, nm = n - 1, p = a; i < n; ++i, --mm, --nm, p += n + 1) {
	if (i && mm > 1) {
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
		    for (j = 0, q = p + k, r *= s; j < mm; q += n)
			*q -= r * w[j++];
		}
		for (k = 0, p1 = u + i; k < m; ++k, p1 += m) {
		    for (j = 0, q = p1, r = 0.; j < mm;)
			r += w[j++] * *q++;
		    for (j = 0, q = p1, r *= s; j < mm;)
			*q++ -= r * w[j++];
		}
	    }
	    *p = sv;
	    d[i] = -h;
	}
	if (mm == 1)
	    d[i] = *p;
	p1 = p + 1;
	if (nm > 1) {
	    sv = h = 0.;
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
		for (k = n, ms = n * (n - i); k < ms; k += n) {
		    for (j = 0, q = p1, pp = p1 + k, r = 0.; j < nm; ++j)
			r += *q++ * *pp++;
		    for (j = 0, q = p1, pp = p1 + k, r *= s; j < nm; ++j)
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
    qrbdv(d, e, u, m, v, n);
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
