/*  cmmult.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include <stdlib.h>
#include "ccmath.h"
void cmmult(Cpx * cm, Cpx * a, Cpx * b, int n, int m, int l)
{
    Cpx z, *q0, *p, *q;

    int i, j, k;

    q0 = (Cpx *) calloc(m, sizeof(Cpx));
    for (i = 0; i < l; ++i, ++cm) {
	for (k = 0, p = b + i; k < m; p += l)
	    q0[k++] = *p;
	for (j = 0, p = a, q = cm; j < n; ++j, q += l) {
	    for (k = 0, z.re = z.im = 0.; k < m; ++k, ++p) {
		z.re += p->re * q0[k].re - p->im * q0[k].im;
		z.im += p->im * q0[k].re + p->re * q0[k].im;
	    }
	    *q = z;
	}
    }
    free(q0);
}
