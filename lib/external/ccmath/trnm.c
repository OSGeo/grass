/*  trnm.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
void trnm(double *a, int n)
{
    double s, *p, *q;

    int i, j, e;

    for (i = 0, e = n - 1; i < n - 1; ++i, --e, a += n + 1) {
	for (p = a + 1, q = a + n, j = 0; j < e; ++j) {
	    s = *p;
	    *p++ = *q;
	    *q = s;
	    q += n;
	}
    }
}
