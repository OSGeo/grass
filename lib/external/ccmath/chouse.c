/*  chouse.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include <stdlib.h>
#include "ccmath.h"
void chouse(Cpx * a, double *d, double *dp, int n)
{
    double sc, x, y;

    Cpx cc, u, *q0;

    int i, j, k, m, e;

    Cpx *qw, *pc, *p;

    q0 = (Cpx *) calloc(2 * n, sizeof(Cpx));
    for (i = 0, p = q0 + n, pc = a; i < n; ++i, pc += n + 1)
	*p++ = *pc;
    for (j = 0, pc = a; j < n - 2; ++j, pc += n + 1) {
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
	    x = 1. / sqrt(2. * sc * y);
	    y *= x;
	    for (i = 0, qw = pc + 1; i < m; ++i) {
		q0[i].re = q0[i].im = 0.;
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
		q0[i].re += (u.re = qw[i].re) * p->re - (u.im =
							 qw[i].im) * p->im;
		q0[i].im += u.re * p->im + u.im * p->re;
		++p;
		for (k = i + 1; k < m; ++k, ++p) {
		    q0[i].re += qw[k].re * p->re - qw[k].im * p->im;
		    q0[i].im += qw[k].im * p->re + qw[k].re * p->im;
		    q0[k].re += u.re * p->re + u.im * p->im;
		    q0[k].im += u.im * p->re - u.re * p->im;
		}
		y += u.re * q0[i].re + u.im * q0[i].im;
	    }
	    for (i = 0; i < m; ++i) {
		q0[i].re -= y * qw[i].re;
		q0[i].re += q0[i].re;
		q0[i].im -= y * qw[i].im;
		q0[i].im += q0[i].im;
	    }
	    for (i = 0, e = j + 2, p = pc + n + 1; i < m; ++i, p += e++) {
		for (k = i; k < m; ++k, ++p) {
		    p->re -= qw[i].re * q0[k].re + qw[i].im * q0[k].im
			+ q0[i].re * qw[k].re + q0[i].im * qw[k].im;
		    p->im -= qw[i].im * q0[k].re - qw[i].re * q0[k].im
			+ q0[i].im * qw[k].re - q0[i].re * qw[k].im;
		}
	    }
	}
	d[j] = pc->re;
	dp[j] = sc;
    }
    d[j] = pc->re;
    d[j + 1] = (pc + n + 1)->re;
    u = *(pc + 1);
    dp[j] = sqrt(u.re * u.re + u.im * u.im);
    for (j = 0, pc = a, qw = q0 + n; j < n; ++j, pc += n + 1) {
	*pc = qw[j];
	for (i = 1, p = pc + n; i < n - j; ++i, p += n) {
	    pc[i].re = p->re;
	    pc[i].im = -p->im;
	}
    }
    free(q0);
}
