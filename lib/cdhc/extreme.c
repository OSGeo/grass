#include <stdio.h>
#include <math.h>


double *Cdhc_extreme(double *x, int n)
{
    int i;
    static double y[2];
    double min, max, sum1 = 0.;

    min = max = x[0];
    for (i = 0; i < n; ++i) {
	sum1 += x[i];
	if (min > x[i])
	    min = x[i];

	if (max < x[i])
	    max = x[i];
    }
    sum1 /= n;

    y[0] = max - sum1;
    y[1] = min - sum1;

#ifdef NOISY
    fprintf(stdout, "  TEST3  U(N)   =%10.4f   U(1)   =%10.4f\n", y[0], y[1]);
#endif /* NOISY */

    return y;
}
