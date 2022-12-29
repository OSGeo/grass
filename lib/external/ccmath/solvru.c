/*  solvru.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
int solvru(double *a, double *b, int n)
{
    int j, k;

    double fabs();

    double s, t, *p, *q;

    for (j = 0, s = 0., p = a; j < n; ++j, p += n + 1)
	if ((t = fabs(*p)) > s)
	    s = t;
    s *= 1.e-16;
    for (j = n - 1, p = a + n * n - 1; j >= 0; --j, p -= n + 1) {
	for (k = j + 1, q = p + 1; k < n;)
	    b[j] -= b[k++] * *q++;
	if (fabs(*p) < s)
	    return -1;
	b[j] /= *p;
    }
    return 0;
}
