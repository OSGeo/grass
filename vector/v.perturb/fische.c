/*
 * Copyright (C) 1994. James Darrell McCauley.  (darrell@mccauley-usa.com)
 *                                              http://mccauley-usa.com/
 *
 * This program is free software under the GPL (>=v2)
 * Read the file GPL.TXT coming with GRASS for details.
 */

#include <stdio.h>
#include <math.h>
#include "zufall.h"

/*-Poisson generator for distribution function of p's:
 * q(mu,p) = exp(-mu) mu**p/p!
 */

int fische(int n, double *mu, int *p)
{
    int left, indx[1024], i, k, nsegs, p0, ii, jj, nl0;
    double q[1024], u[1024], q0, pmu;

    --p;

    if (n <= 0)
	return 0;

    pmu = exp(-(*mu));
    p0 = 0;

    nsegs = (n - 1) / 1024;
    left = n - (nsegs << 10);
    ++nsegs;
    nl0 = left;

    for (k = 1; k <= nsegs; ++k) {
	for (i = 1; i <= left; ++i) {
	    indx[i - 1] = i;
	    p[p0 + i] = 0;
	    q[i - 1] = 1.0;
	}

	do {			/* Begin iterative loop on segment of p's */
	    zufall(left, u);	/* Get the needed uniforms */
	    jj = 0;

	    for (i = 1; i <= left; ++i) {
		ii = indx[i - 1];
		q0 = q[ii - 1] * u[i - 1];
		q[ii - 1] = q0;
		if (q0 > pmu) {
		    indx[jj++] = ii;
		    ++p[p0 + ii];
		}
	    }
	    left = jj;		/* any left in this segment? */
	}
	while (left > 0);

	p0 += nl0;
	nl0 = left = 1024;
    }
    return 0;
}				/* fische */
