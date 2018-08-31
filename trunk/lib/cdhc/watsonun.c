#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "local_proto.h"


double *Cdhc_watson_u2(double *x, int n)
{
    double *xcopy, mean = 0.0, sdx = 0.0, sqrt2, zbar = 0.0;
    double fn2, fx, sum4 = 0.0;
    static double y[2];
    int i;

    sqrt2 = sqrt((double)2.0);

    if ((xcopy = (double *)malloc(n * sizeof(double))) == NULL) {
	fprintf(stderr, "Memory error in Cdhc_anderson_darling\n");
	exit(EXIT_FAILURE);
    }

    for (i = 0; i < n; ++i) {
	xcopy[i] = x[i];
	mean += x[i];
	sdx += x[i] * x[i];
    }
    sdx = sqrt((n * sdx - mean * mean) / (n * (n - 1)));
    mean /= n;

    qsort(xcopy, n, sizeof(double), Cdhc_dcmp);

    for (i = 0; i < n; ++i) {
	xcopy[i] = (xcopy[i] - mean) / sdx;
	fn2 = (2.0 * (i + 1) - 1.0) / (2.0 * n);
	fx = 0.5 + Cdhc_normp(xcopy[i] / sqrt2) / 2.0;

	if (fx <= 0.0)
	    fx = 1e-5;

	if (fx >= 1.0)
	    fx = 0.99999;

	zbar += fx;
	sum4 += (fx - fn2) * (fx - fn2);
    }

    zbar /= n;
    y[0] = (1.0 / (n * 12) + sum4) - n * (zbar - .5) * (zbar - .5);
    y[0] *= 0.5 / n + 1.0;

#ifdef NOISY
    fprintf(stdout, "  TEST6  WU2(N) =%10.4f\n", y[0]);
#endif /* NOISY */

    free(xcopy);

    return y;
}
