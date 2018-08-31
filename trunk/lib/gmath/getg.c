/*      Name:   getg.c
 *
 * Created:        Thu May 29 00:37:44 1986
 * Last modified:  Sat May 31 20:34:30 1986
 *
 * Purpose:        Get the laplacian of a Gaussian (not normalized).
 *
 * Author:         Bill Hoff,2-114C,8645,3563478 (hoff) at uicsl
 */

#include <stdio.h>
#include <math.h>
#include <grass/gmath.h>


int getg(double w, double *g[2], int size)
{
    long i, j, totsize, n, g_row;
    float rsq, sigma, two_ssq, val, sum = 0.0;

    totsize = size * size;
    n = size / 2;
    for (i = 0; i < totsize; i++) {
	*(g[0] + i) = 0.0;
	*(g[1] + i) = 0.0;
    }

    sigma = w / (2.0 * sqrt((double)2.0));
    two_ssq = 2.0 * sigma * sigma;
    for (i = 0; i < n; i++) {
	g_row = i * size;	/* start of row */
	for (j = 0; j < n; j++) {
	    rsq = i * i + j * j;
	    val = (rsq / two_ssq - 1) * exp(-rsq / two_ssq);
	    *(g[0] + g_row + j) = val;
	    sum += val;
	    /* reflect into other quadrants */
	    if (j > 0) {
		*(g[0] + g_row + (size - j)) = val;
		sum += val;
	    }
	    if (i > 0) {
		*(g[0] + (size - i) * size + j) = val;
		sum += val;
	    }
	    if (i > 0 && j > 0) {
		*(g[0] + (size - i) * size + (size - j)) = val;
		sum += val;
	    }
	}
    }

    *(g[0] + 0) -= sum;		/* make sure sum of all values is zero */

    return 0;
}
