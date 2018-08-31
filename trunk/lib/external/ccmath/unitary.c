/*  unitary.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include <stdlib.h>
#include "ccmath.h"
static double tpi = 6.283185307179586;

static void uortho(double *g, int n);

double unfl();

void unitary(Cpx * u, int n)
{
    int i, j, k, m;

    Cpx h, *v, *e, *p, *r;

    double *g, *q, a;

    m = n * n;
    g = (double *)calloc(n * n, sizeof(double));
    v = (Cpx *) calloc(m + n, sizeof(Cpx));
    e = v + m;
    h.re = 1.;
    h.im = 0.;
    for (i = 0; i < n; ++i) {
	a = tpi * unfl();
	e[i].re = cos(a);
	e[i].im = sin(a);
	a = h.re * e[i].re - h.im * e[i].im;
	h.im = h.im * e[i].re + h.re * e[i].im;
	h.re = a;
    }
    h.im = -h.im;
    for (i = 0; i < n; ++i) {
	a = e[i].re * h.re - e[i].im * h.im;
	e[i].im = e[i].re * h.im + e[i].im * h.re;
	e[i].re = a;
    }
    uortho(g, n);
    for (i = 0, p = v, q = g; i < n; ++i) {
	for (j = 0; j < n; ++j)
	    (p++)->re = *q++;
    }
    for (i = 0, p = v; i < n; ++i) {
	for (j = 0, h = e[i]; j < n; ++j, ++p) {
	    a = h.re * p->re - h.im * p->im;
	    p->im = h.im * p->re + h.re * p->im;
	    p->re = a;
	}
    }
    uortho(g, n);
    for (i = m = 0, p = u; i < n; ++i, m += n) {
	for (j = 0; j < n; ++j, ++p) {
	    p->re = p->im = 0.;
	    for (k = 0, q = g + m, r = v + j; k < n; ++k, r += n) {
		p->re += *q * r->re;
		p->im += *q++ * r->im;
	    }
	}
    }
    free(g);
    free(v);
}

static void uortho(double *g, int n)
{
    int i, j, k, m;

    double *p, *q, c, s, a;

    for (i = 0, p = g; i < n; ++i) {
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
	    p = g + n * i;
	    q = g + n * j;
	    for (k = 0; k < n; ++k) {
		a = *p * c + *q * s;
		*q = *q * c - *p * s;
		*p++ = a;
		++q;
	    }
	}
    }
}
