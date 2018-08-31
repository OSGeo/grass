#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "local_proto.h"


double *Cdhc_chi_square(double *x, int n)
{
    static double y[2];
    double mean = 0.0, sdx = 0.0, sum3 = 0.0, *v;
    int i, j, k, *f;

    k = rint(4.0 * pow(0.75 * (n - 1.0) * (n - 1.0), 0.2));

    while ((double)(n / k) < 5.0)
	--k;

    if ((f = (int *)calloc(k, sizeof(int))) == NULL) {
	fprintf(stderr, "Memory error in Cdhc_chi_square\n");
	exit(EXIT_FAILURE);
    }
    if ((v = (double *)malloc((k + 1) * sizeof(double))) == NULL) {
	fprintf(stderr, "Memory error in Cdhc_chi_square\n");
	exit(EXIT_FAILURE);
    }

    for (i = 0; i < n; ++i) {
	mean += x[i];
	sdx += x[i] * x[i];
    }
    sdx = sqrt((n * sdx - mean * mean) / (n * (n - 1.0)));
    mean /= n;

    v[0] = -1e9;
    for (i = 1; i < k; ++i)
	v[i] = mean + Cdhc_xinormal((double)i / k) * sdx;

    v[k] = 1e9;

    for (i = 0; i < n; ++i) {
	j = 0;
	while (j < k) {
	    if (x[i] > v[j] && x[i] <= v[j + 1]) {
		f[j]++;
		j = k;
	    }
	    j++;
	}
    }

    for (i = 0; i < k; ++i)
	sum3 += f[i] * f[i];

    y[0] = sum3 * k / n - n;
    y[1] = (double)k - 3.0;

#ifdef NOISY
    fprintf(stdout, "  TEST12 CS(N)  =%10.4f   DOF    =%10.4f\n", y[0], y[1]);
#endif /* NOISY */

    free(f);
    free(v);

    return y;
}
