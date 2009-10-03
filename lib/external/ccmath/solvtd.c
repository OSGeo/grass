/*  solvtd.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
void solvtd(double *a, double *b, double *c, double *x, int m)
{
    double s;

    int j;

    for (j = 0; j < m; ++j) {
	s = b[j] / a[j];
	a[j + 1] -= s * c[j];
	x[j + 1] -= s * x[j];
    }
    for (j = m, s = 0.; j >= 0; --j) {
	x[j] -= s * c[j];
	s = (x[j] /= a[j]);
    }
}
