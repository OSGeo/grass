/*  ruinv.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
int ruinv(double *a, int n)
{
    int j;

    double fabs();

    double tt, z, *p, *q, *r, *s, *t;

    for (j = 0, tt = 0., p = a; j < n; ++j, p += n + 1)
	if ((z = fabs(*p)) > tt)
	    tt = z;
    tt *= 1.e-16;
    for (j = 0, p = a; j < n; ++j, p += n + 1) {
	if (fabs(*p) < tt)
	    return -1;
	*p = 1. / *p;
	for (q = a + j, t = a; q < p; t += n + 1, q += n) {
	    for (s = q, r = t, z = 0.; s < p; s += n)
		z -= *s * *r++;
	    *q = z * *p;
	}
    }
    return 0;
}
