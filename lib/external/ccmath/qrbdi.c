/*  qrbdi.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include "ccmath.h"
int qrbdi(double *dm, double *em, int m)
{
    int i, j, k, n;

    double u, x, y, a, b, c, s, t;

    for (j = 1, t = fabs(dm[0]); j < m; ++j)
	if ((s = fabs(dm[j]) + fabs(em[j - 1])) > t)
	    t = s;
    t *= 1.e-15;
    n = 100 * m;
    for (j = 0; m > 1 && j < n; ++j) {
	for (k = m - 1; k > 0; --k) {
	    if (fabs(em[k - 1]) < t)
		break;
	    if (fabs(dm[k - 1]) < t) {
		for (i = k, s = 1., c = 0.; i < m; ++i) {
		    a = s * em[i - 1];
		    b = dm[i];
		    em[i - 1] *= c;
		    dm[i] = u = sqrt(a * a + b * b);
		    s = -a / u;
		    c = b / u;
		}
		break;
	    }
	}
	y = dm[k];
	x = dm[m - 1];
	u = em[m - 2];
	a = (y + x) * (y - x) - u * u;
	s = y * em[k];
	b = s + s;
	u = sqrt(a * a + b * b);
	if (u > 0.) {
	    c = sqrt((u + a) / (u + u));
	    if (c != 0.)
		s /= (c * u);
	    else
		s = 1.;
	    for (i = k; i < m - 1; ++i) {
		b = em[i];
		if (i > k) {
		    a = s * em[i];
		    b *= c;
		    em[i - 1] = u = sqrt(x * x + a * a);
		    c = x / u;
		    s = a / u;
		}
		a = c * y + s * b;
		b = c * b - s * y;
		s *= dm[i + 1];
		dm[i] = u = sqrt(a * a + s * s);
		y = c * dm[i + 1];
		c = a / u;
		s /= u;
		x = c * b + s * y;
		y = c * y - s * b;
	    }
	}
	em[m - 2] = x;
	dm[m - 1] = y;
	if (fabs(x) < t)
	    --m;
	if (m == k + 1)
	    --m;
    }
    return j;
}
