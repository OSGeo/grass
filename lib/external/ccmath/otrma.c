/*  otrma.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include <stdlib.h>
void otrma(double *c, double *a, double *b, int n)
{
    double z, *q0, *p, *s, *t;

    int i, j, k;

    q0 = (double *)calloc(n, sizeof(double));
    for (i = 0; i < n; ++i, ++c) {
	for (j = 0, t = b; j < n; ++j) {
	    for (k = 0, s = a + i * n, z = 0.; k < n; ++k)
		z += *t++ * *s++;
	    q0[j] = z;
	}
	for (j = 0, p = c, t = a; j < n; ++j, p += n) {
	    for (k = 0, s = q0, z = 0.; k < n; ++k)
		z += *t++ * *s++;
	    *p = z;
	}
    }
    free(q0);
}
