/*  hmgen.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include <stdlib.h>
#include "ccmath.h"
void hmgen(Cpx * h, double *ev, Cpx * u, int n)
{
    Cpx *v, *p;

    int i, j;

    double e;

    v = (Cpx *) calloc(n * n, sizeof(Cpx));
    cmcpy(v, u, n * n);
    hconj(v, n);
    for (i = 0, p = v; i < n; ++i) {
	for (j = 0, e = ev[i]; j < n; ++j, ++p) {
	    p->re *= e;
	    p->im *= e;
	}
    }
    cmmul(h, u, v, n);
    free(v);
}
