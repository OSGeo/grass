#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "local_proto.h"


double *Cdhc_dmax_exp(double *x, int n)
{
    static double y[2];
    double mean = 0.0, zmax, tmax, *xcopy, t, z, fx;
    int i;

    if ((xcopy = (double *)malloc(n * sizeof(double))) == NULL) {
	fprintf(stderr, "Memory error in Cdhc_dmax_exp\n");
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
	z = (double)(i + 1) / (double)n - fx;
	t = fx - (double)i / (double)n;
	if (i == 0 || z > zmax)
	    zmax = z;

	if (i == 0 || t > tmax)
	    tmax = t;
    }

    y[0] = zmax;
    y[1] = tmax;

    free(xcopy);

    return y;
}
