#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "local_proto.h"


double *Cdhc_anderson_darling_exp(double *x, int n)
{
    static double y[2];
    double sqrt2, mean = 0.0, *xcopy, fx, sum3 = 0.0;
    int i;

    if ((xcopy = (double *)malloc(n * sizeof(double))) == NULL) {
	fprintf(stderr, "Memory error in Cdhc_anderson_darling\n");
	exit(EXIT_FAILURE);
    }

    sqrt2 = sqrt((double)2.0);

    for (i = 0; i < n; ++i) {
	xcopy[i] = x[i];
	mean += x[i];
    }
    mean /= n;
    qsort(xcopy, n, sizeof(double), Cdhc_dcmp);

    for (i = 0; i < n; ++i) {
	fx = 1 - exp(-xcopy[i] / mean);
	sum3 += (2.0 * i + 1) * (log(fx) - xcopy[n - i - 1] / mean);
    }

    y[0] = (1.0 + 0.3 / n) * (-n - sum3 / n);
#ifdef NOISY
    fprintf(stdout, "  TEST20 AD(E)  =%10.4f\n", y[0]);
#endif /* NOISY */

    free(xcopy);

    return y;
}
