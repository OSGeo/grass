/*  cminv.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include <stdlib.h>
#include "ccmath.h"
int cminv(Cpx * a, int n)
{
    int i, j, k, m, lc, *le;

    Cpx *ps, *p, *q, *pa, *pd;

    Cpx z, h, *q0;

    double s, t, tq = 0., zr = 1.e-15;

    le = (int *)calloc(n, sizeof(int));
    q0 = (Cpx *) calloc(n, sizeof(Cpx));
    pa = pd = a;
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
	    free(le - j);
	    free(q0);
	    return -1;
	}
	*le++ = lc;
	if (lc != j) {
	    p = a + n * j;
	    q = a + n * lc;
	    for (k = 0; k < n; ++k, ++p, ++q) {
		h = *p;
		*p = *q;
		*q = h;
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
	*pd = h;
    }
    for (j = 1, pd = ps = a; j < n; ++j) {
	for (k = 0, pd += n + 1, q = ++ps; k < j; ++k, q += n) {
	    z.re = q->re * pd->re - q->im * pd->im;
	    z.im = q->im * pd->re + q->re * pd->im;
	    *q = z;
	}
    }
    for (j = 1, pa = a; j < n; ++j) {
	++pa;
	for (i = 0, q = q0, p = pa; i < j; ++i, p += n)
	    *q++ = *p;
	for (k = 0; k < j; ++k) {
	    h.re = h.im = 0.;
	    for (i = k, p = pa + k * n + k - j, q = q0 + k; i < j; ++i) {
		h.re -= p->re * q->re - p->im * q->im;
		h.im -= p->im * q->re + p->re * q->im;
		++p;
		++q;
	    }
	    q0[k] = h;
	}
	for (i = 0, q = q0, p = pa; i < j; ++i, p += n)
	    *p = *q++;
    }
    for (j = n - 2, pd = pa = a + n * n - 1; j >= 0; --j) {
	--pa;
	pd -= n + 1;
	for (i = 0, m = n - j - 1, q = q0, p = pd + n; i < m; ++i, p += n)
	    *q++ = *p;
	for (k = n - 1, ps = pa; k > j; --k, ps -= n) {
	    z.re = -ps->re;
	    z.im = -ps->im;
	    for (i = j + 1, p = ps + 1, q = q0; i < k; ++i, ++p, ++q) {
		z.re -= p->re * q->re - p->im * q->im;
		z.im -= p->im * q->re + p->re * q->im;
	    }
	    q0[--m] = z;
	}
	for (i = 0, m = n - j - 1, q = q0, p = pd + n; i < m; ++i, p += n)
	    *p = *q++;
    }
    for (k = 0, pa = a; k < n - 1; ++k, ++pa) {
	for (i = 0, q = q0, p = pa; i < n; ++i, p += n)
	    *q++ = *p;
	for (j = 0, ps = a; j < n; ++j, ps += n) {
	    if (j > k) {
		h.re = h.im = 0.;
		p = ps + j;
		i = j;
	    }
	    else {
		h = q0[j];
		p = ps + k + 1;
		i = k + 1;
	    }
	    for (; i < n; ++i, ++p) {
		h.re += p->re * q0[i].re - p->im * q0[i].im;
		h.im += p->im * q0[i].re + p->re * q0[i].im;
	    }
	    q0[j] = h;
	}
	for (i = 0, q = q0, p = pa; i < n; ++i, p += n)
	    *p = *q++;
    }
    for (j = n - 2, le--; j >= 0; --j) {
	for (k = 0, p = a + j, q = a + *(--le); k < n; ++k, p += n, q += n) {
	    h = *p;
	    *p = *q;
	    *q = h;
	}
    }
    free(le);
    free(q0);
    return 0;
}
