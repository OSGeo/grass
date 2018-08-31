/*  vmul.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
void vmul(double *vp, double *mat, double *v, int n)
{
    double s, *q;

    int k, i;

    for (k = 0; k < n; ++k) {
	for (i = 0, q = v, s = 0.; i < n; ++i)
	    s += *mat++ * *q++;
	*vp++ = s;
    }
}

double vnrm(double *u, double *v, int n)
{
    double s;

    int i;

    for (i = 0, s = 0.; i < n; ++i)
	s += *u++ * *v++;
    return s;
}
