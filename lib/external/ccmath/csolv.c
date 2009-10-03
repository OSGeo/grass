/*  csolv.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include <stdlib.h>
#include "ccmath.h"
int csolv(Cpx * a, Cpx * b, int n)
{
    int i, j, k, lc;

    Cpx *ps, *p, *q, *pa, *pd;

    Cpx z, h, *q0;

    double s, t, tq = 0., zr = 1.e-15;

    q0 = (Cpx *) calloc(n, sizeof(Cpx));
    pa = a;
    pd = a;
    for (j = 0; j < n; ++j, ++pa, pd += n + 1) {
	if (j > 0) {
	    for (i = 0, p = pa, q = q0; i < n; ++i, p += n)
		*q++ = *p;
	    for (i = 1; i < n; ++i) {
		lc = i < j ? i : j;
		z.re = z.im = 0.;
		for (k = 0, p = pa + i * n - j, q = q0; k < lc; ++k, ++q, ++p) {
		    z.re += p->re * q->re - p->im * q->im;
		    z.im += p->im * q->re + p->re * q->im;
		}
		q0[i].re -= z.re;
		q0[i].im -= z.im;
	    }
	    for (i = 0, p = pa, q = q0; i < n; ++i, p += n)
		*p = *q++;
	}
	s = fabs(pd->re) + fabs(pd->im);
	lc = j;
	for (k = j + 1, ps = pd; k < n; ++k) {
	    ps += n;
	    if ((t = fabs(ps->re) + fabs(ps->im)) > s) {
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
	    h = b[j];
	    b[j] = b[lc];
	    b[lc] = h;
	    p = a + n * j;
	    q = a + n * lc;
	    for (k = 0; k < n; ++k) {
		h = *p;
		*p++ = *q;
		*q++ = h;
	    }
	}
	t = pd->re * pd->re + pd->im * pd->im;
	h.re = pd->re / t;
	h.im = -(pd->im) / t;
	for (k = j + 1, ps = pd + n; k < n; ++k, ps += n) {
	    z.re = ps->re * h.re - ps->im * h.im;
	    z.im = ps->im * h.re + ps->re * h.im;
	    *ps = z;
	}
    }
    for (j = 1, ps = b + 1; j < n; ++j, ++ps) {
	for (k = 0, p = a + n * j, q = b, z.re = z.im = 0.; k < j; ++k) {
	    z.re += p->re * q->re - p->im * q->im;
	    z.im += p->im * q->re + p->re * q->im;
	    ++p;
	    ++q;
	}
	ps->re -= z.re;
	ps->im -= z.im;
    }
    for (j = n - 1, --ps, pd = a + n * n - 1; j >= 0; --j, pd -= n + 1) {
	for (k = j + 1, p = pd + 1, q = b + j + 1, z.re = z.im = 0.; k < n;
	     ++k) {
	    z.re += p->re * q->re - p->im * q->im;
	    z.im += p->im * q->re + p->re * q->im;
	    ++p;
	    ++q;
	}
	h.re = ps->re - z.re;
	h.im = ps->im - z.im;
	t = pd->re * pd->re + pd->im * pd->im;
	ps->re = (h.re * pd->re + h.im * pd->im) / t;
	ps->im = (h.im * pd->re - h.re * pd->im) / t;
	--ps;
    }
    free(q0);
    return 0;
}
