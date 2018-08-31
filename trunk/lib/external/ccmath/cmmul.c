/*  cmmul.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include "ccmath.h"
void cmmul(Cpx * c, Cpx * a, Cpx * b, int n)
{
    Cpx s, *p, *q;

    int i, j, k;

    trncm(b, n);
    for (i = 0; i < n; ++i, a += n) {
	for (j = 0, q = b; j < n; ++j) {
	    for (k = 0, p = a, s.re = s.im = 0.; k < n; ++k) {
		s.re += p->re * q->re - p->im * q->im;
		s.im += p->im * q->re + p->re * q->im;
		++p;
		++q;
	    }
	    *c++ = s;
	}
    }
    trncm(b, n);
}
