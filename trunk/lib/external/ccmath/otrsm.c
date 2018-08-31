/*  otrsm.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include <stdlib.h>
void otrsm(double *sm, double *a, double *b, int n)
{
    double z, *q0, *p, *s, *t;

    int i, j, k;

    q0 = (double *)calloc(n, sizeof(double));
    for (i = 0; i < n; ++i) {
	for (j = 0, t = b; j < n; ++j) {
	    for (k = 0, s = a + i * n, z = 0.; k < n; ++k)
		z += *t++ * *s++;
	    q0[j] = z;
	}
	for (j = 0, p = sm + i, t = a; j <= i; ++j, p += n) {
	    for (k = 0, s = q0, z = 0.; k < n; ++k)
		z += *t++ * *s++;
	    *p = z;
	    if (j < i)
		sm[i * n + j] = z;
	}
    }
    free(q0);
}
