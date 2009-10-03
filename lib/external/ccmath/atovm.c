/*  atovm.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
void atovm(double *v, int n)
{
    double *p0, *q0, *p, *q, *qq;

    double h, s;

    int i, j, k, mm;

    q0 = v + n * n - 1;
    *q0 = 1.;
    q0 -= n + 1;
    p0 = v + n * n - n - n - 1;
    for (i = n - 2, mm = 1; i >= 0; --i, p0 -= n + 1, q0 -= n + 1, ++mm) {
	if (i && *(p0 - 1) != 0.) {
	    for (j = 0, p = p0, h = 1.; j < mm; ++j, ++p)
		h += *p * *p;
	    h = *(p0 - 1);
	    *q0 = 1. - h;
	    for (j = 0, q = q0 + n, p = p0; j < mm; ++j, q += n)
		*q = -h * *p++;
	    for (k = i + 1, q = q0 + 1; k < n; ++k) {
		for (j = 0, qq = q + n, p = p0, s = 0.; j < mm; ++j, qq += n)
		    s += *qq * *p++;
		s *= h;
		for (j = 0, qq = q + n, p = p0; j < mm; ++j, qq += n)
		    *qq -= s * *p++;
		*q++ = -s;
	    }
	}
	else {
	    *q0 = 1.;
	    for (j = 0, p = q0 + 1, q = q0 + n; j < mm; ++j, q += n)
		*q = *p++ = 0.;
	}
    }
}
