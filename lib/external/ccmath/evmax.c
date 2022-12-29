/*  evmax.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include <stdlib.h>
#include "ccmath.h"
double evmax(double *a, double *u, int n)
{
    double *p, *q, *qm, *r, *s, *t;

    double ev, evm, c, h;

    int kc;

    q = (double *)calloc(n, sizeof(double));
    qm = q + n;
    *(qm - 1) = 1.;
    ev = 0.;
    for (kc = 0; kc < 200; ++kc) {
	h = c = 0.;
	evm = ev;
	for (p = u, r = a, s = q; s < qm;) {
	    *p = 0.;
	    for (t = q; t < qm;)
		*p += *r++ * *t++;
	    c += *p * *p;
	    h += *p++ * *s++;
	}
	ev = c / h;
	c = sqrt(c);
	for (p = u, s = q; s < qm;) {
	    *p /= c;
	    *s++ = *p++;
	}
	if (((c = ev - evm) < 0. ? -c : c) < 1.e-16 * (ev < 0. ? -ev : ev)) {
	    free(q);
	    return ev;
	}
    }
    free(q);
    for (kc = 0; kc < n;)
	u[kc++] = 0.;
    return 0.;
}
