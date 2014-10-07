#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "local_proto.h"


double *Cdhc_watson_u2_exp(double *x, int n)
{
    double *xcopy, mean = 0.0, zbar = 0.0, fn2, fx, sum4 = 0.0;
    static double y[2];
    int i;

    if ((xcopy = (double *)malloc(n * sizeof(double))) == NULL) {
	fprintf(stderr, "Memory error in Cdhc_watson_u2_exp\n");
	exit(EXIT_FAILURE);
    }

    for (i = 0; i < n; ++i) {
	xcopy[i] = x[i];
	mean += x[i];
    }
    mean /= n;

    qsort(xcopy, n, sizeof(double), Cdhc_dcmp);

    for (i = 0; i < n; ++i) {
	fx = 1 - exp(-xcopy[i] / mean);
	if (fx <= 1e-5)
	    fx = 1e-5;

	if (fx >= 0.99999)
	    fx = 0.99999;

	/* sum3 += 2 * (i + 1) * (log (fx) + log (1.0 - fx[n - i - 1])); */
	fn2 = (2.0 * i + 1.0) / (2.0 * n);
	sum4 += (fx - fn2) * (fx - fn2);
	fn2 = (2.0 * (i + 1) - 1.0) / (2.0 * n);
	zbar += fx;
    }

    zbar /= n;
    y[0] = (1.0 / (n * 12) + sum4) - n * (zbar - .5) * (zbar - .5);
    y[0] *= 1.0 + 0.16 / n;

#ifdef NOISY
    fprintf(stdout, "  TEST19 WU2(E) =%10.4f\n", y[0]);
#endif /* NOISY */

    free(xcopy);

    return y;
}
