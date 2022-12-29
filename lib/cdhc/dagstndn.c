#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "local_proto.h"


double *Cdhc_dagostino_d(double *x, int n)
{
    int i;
    static double y[2];
    double d, s, t = 0., *xcopy, m2, s1 = 0., s2, mn = 0.0;

    if ((xcopy = (double *)malloc(n * sizeof(double))) == NULL) {
	fprintf(stderr, "Memory allocation error\n");
	exit(EXIT_FAILURE);
    }

    for (i = 0; i < n; ++i)
	xcopy[i] = x[i];

    qsort(xcopy, n, sizeof(double), Cdhc_dcmp);

    for (i = 0; i < n; ++i) {
	t += xcopy[i] * ((i + 1) - 0.5 * (n + 1));
	mn += xcopy[i];
    }

    m2 = mn / n;
    for (i = 0; i < n; ++i)
	s1 += (xcopy[i] - m2) * (xcopy[i] - m2);

    s2 = s1 / n;
    s = sqrt(s2);
    d = t / (n * n * s);

    /* y[0] = (d - 1. / (2*sqrt (M_PI))) * sqrt ((double)n) / 0.02998598; */
    y[0] = d;
    y[1] = sqrt((double)n) * (y[0] - 0.28209479) / 0.02998598;

#ifdef NOISY
    fprintf(stdout, "  TEST4  DAGN   =%10.4f\n", y[0]);
#endif /* NOISY */

    return y;
}
