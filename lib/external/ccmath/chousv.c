/*  chousv.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include <stdlib.h>
#include "ccmath.h"
void chousv(Cpx * a, double *d, double *dp, int n)
{
    double sc, x, y;

    Cpx cc, u, *qs;

    int i, j, k, m, e;

    Cpx *qw, *pc, *p, *q;

    qs = (Cpx *) calloc(2 * n, sizeof(Cpx));
    q = qs + n;
    for (j = 0, pc = a; j < n - 2; ++j, pc += n + 1, ++q) {
	m = n - j - 1;
	for (i = 1, sc = 0.; i <= m; ++i)
	    sc += pc[i].re * pc[i].re + pc[i].im * pc[i].im;
	if (sc > 0.) {
	    sc = sqrt(sc);
	    p = pc + 1;
	    y = sc + (x = sqrt(p->re * p->re + p->im * p->im));
	    if (x > 0.) {
		cc.re = p->re / x;
		cc.im = p->im / x;
	    }
	    else {
		cc.re = 1.;
		cc.im = 0.;
	    }
	    q->re = -cc.re;
	    q->im = -cc.im;
	    x = 1. / sqrt(2. * sc * y);
	    y *= x;
	    for (i = 0, qw = pc + 1; i < m; ++i) {
		qs[i].re = qs[i].im = 0.;
		if (i) {
		    qw[i].re *= x;
		    qw[i].im *= -x;
		}
		else {
		    qw[0].re = y * cc.re;
		    qw[0].im = -y * cc.im;
		}
	    }
	    for (i = 0, e = j + 2, p = pc + n + 1, y = 0.; i < m;
		 ++i, p += e++) {
		qs[i].re += (u.re = qw[i].re) * p->re - (u.im =
							 qw[i].im) * p->im;
		qs[i].im += u.re * p->im + u.im * p->re;
		++p;
		for (k = i + 1; k < m; ++k, ++p) {
		    qs[i].re += qw[k].re * p->re - qw[k].im * p->im;
		    qs[i].im += qw[k].im * p->re + qw[k].re * p->im;
		    qs[k].re += u.re * p->re + u.im * p->im;
		    qs[k].im += u.im * p->re - u.re * p->im;
		}
		y += u.re * qs[i].re + u.im * qs[i].im;
	    }
	    for (i = 0; i < m; ++i) {
		qs[i].re -= y * qw[i].re;
		qs[i].re += qs[i].re;
		qs[i].im -= y * qw[i].im;
		qs[i].im += qs[i].im;
	    }
	    for (i = 0, e = j + 2, p = pc + n + 1; i < m; ++i, p += e++) {
		for (k = i; k < m; ++k, ++p) {
		    p->re -= qw[i].re * qs[k].re + qw[i].im * qs[k].im
			+ qs[i].re * qw[k].re + qs[i].im * qw[k].im;
		    p->im -= qw[i].im * qs[k].re - qw[i].re * qs[k].im
			+ qs[i].im * qw[k].re - qs[i].re * qw[k].im;
		}
	    }
	}
	d[j] = pc->re;
	dp[j] = sc;
    }
    d[j] = pc->re;
    cc = *(pc + 1);
    d[j + 1] = (pc += n + 1)->re;
    dp[j] = sc = sqrt(cc.re * cc.re + cc.im * cc.im);
    q->re = cc.re /= sc;
    q->im = cc.im /= sc;
    for (i = 0, m = n + n, p = pc; i < m; ++i, --p)
	p->re = p->im = 0.;
    pc->re = 1.;
    (pc -= n + 1)->re = 1.;
    qw = pc - n;
    for (m = 2; m < n; ++m, qw -= n + 1) {
	for (j = 0, p = pc, pc->re = 1.; j < m; ++j, p += n) {
	    for (i = 0, q = p, u.re = u.im = 0.; i < m; ++i, ++q) {
		u.re += qw[i].re * q->re - qw[i].im * q->im;
		u.im += qw[i].re * q->im + qw[i].im * q->re;
	    }
	    for (i = 0, q = p, u.re += u.re, u.im += u.im; i < m; ++i, ++q) {
		q->re -= u.re * qw[i].re + u.im * qw[i].im;
		q->im -= u.im * qw[i].re - u.re * qw[i].im;
	    }
	}
	for (i = 0, p = qw + m - 1; i < n; ++i, --p)
	    p->re = p->im = 0.;
	(pc -= n + 1)->re = 1.;
    }
    for (j = 1, p = a + n + 1, q = qs + n, u.re = 1., u.im = 0.; j < n;
	 ++j, ++p, ++q) {
	sc = u.re * q->re - u.im * q->im;
	u.im = u.im * q->re + u.re * q->im;
	u.re = sc;
	for (i = 1; i < n; ++i, ++p) {
	    sc = u.re * p->re - u.im * p->im;
	    p->im = u.re * p->im + u.im * p->re;
	    p->re = sc;
	}
    }
    free(qs);
}
