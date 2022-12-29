#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "local_proto.h"


double *Cdhc_anderson_darling(double *x, int n)
{
    int i;
    static double y[2];
    double sqrt2, mean = 0.0, sdx = 0.0, *xcopy, fx;

    if ((xcopy = (double *)malloc(n * sizeof(double))) == NULL) {
	fprintf(stderr, "Memory error in Cdhc_anderson_darling\n");
	exit(EXIT_FAILURE);
    }

    sqrt2 = sqrt((double)2.0);
    y[0] = y[1] = 0.0;

    for (i = 0; i < n; ++i) {
	xcopy[i] = x[i];
	mean += x[i];
	sdx += x[i] * x[i];
    }
    sdx = sqrt((n * sdx - mean * mean) / (n * (n - 1.0)));
    mean /= n;

    qsort(xcopy, n, sizeof(double), Cdhc_dcmp);

    for (i = 0; i < n; ++i)
	xcopy[i] = (xcopy[i] - mean) / sdx;

    for (i = 0; i < n; ++i) {
	fx = 0.5 + Cdhc_normp(xcopy[i] / sqrt2) / 2.0;
	if (fx <= 1e-5)
	    fx = 1e-5;

	if (fx >= .99999)
	    fx = 0.99999;

	y[1] +=
	    (2.0 * i + 1.0) * log(fx) + (2.0 * (n - i) - 1.0) * log(1 - fx);
    }
    y[1] = -n - y[1] / n;
    y[0] = y[1] * (0.75 / n + 1.0 + 2.25 / (n * n));

#ifdef NOISY
    fprintf(stdout, "  TEST8  AD(N)  =%10.4f\n", y[0]);
#endif /* NOISY */
    free(xcopy);

    return y;
}
