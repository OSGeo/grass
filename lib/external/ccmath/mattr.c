/*  mattr.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
void mattr(double *a, double *b, int m, int n)
{
    double *p;

    int i, j;

    for (i = 0; i < n; ++i, ++b)
	for (j = 0, p = b; j < m; ++j, p += n)
	    *a++ = *p;
}
