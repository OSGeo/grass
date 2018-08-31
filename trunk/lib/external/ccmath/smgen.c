/*  smgen.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
void smgen(double *a, double *eval, double *evec, int n)
{
    double *p, *q, *ps, *r, *s, *t, *v = evec + n * n;

    for (ps = a, p = evec; p < v; p += n) {
	for (q = evec; q < v; q += n, ++ps) {
	    *ps = 0.;
	    for (r = eval, s = p, t = q; r < eval + n;)
		*ps += *r++ * *s++ * *t++;
	}
    }
}
