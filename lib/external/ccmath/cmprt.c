/*  cmprt.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include "ccmath.h"
void cmprt(Cpx * a, int m, int n, char *f)
{
    int i, j;

    Cpx *p;

    for (i = 0, p = a; i < m; ++i) {
	for (j = 0; j < n; ++j, ++p)
	    printf(f, p->re, p->im);
	printf("\n");
    }
}
