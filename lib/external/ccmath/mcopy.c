/*  mcopy.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
void mcopy(double *a, double *b, int m)
{
    double *p, *q;

    int k;

    for (p = a, q = b, k = 0; k < m; ++k)
	*p++ = *q++;
}
