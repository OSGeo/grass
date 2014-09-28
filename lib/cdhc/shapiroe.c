#include <stdio.h>
#include <math.h>


double *Cdhc_shapiro_wilk_exp(double *x, int n)
{
    static double y[2];
    double mean, b, s1, xs, sum1 = 0.0, sum2 = 0.0;
    int i;

    for (i = 0; i < n; ++i)
	if (i == 0 || xs > x[i])
	    xs = x[i];

    for (i = 0; i < n; ++i) {
	sum1 += x[i];
	sum2 += x[i] * x[i];
    }

    s1 = sum2 - sum1 * sum1 / n;
    mean = sum1 / n;
    b = (mean - xs) * sqrt((double)n / (n - 1.0));
    y[0] = b * b / s1;

#ifdef NOISY
    fprintf(stdout, "  TEST15 SW(E)  =%10.4f\n", y[0]);
#endif /* NOISY */

    return y;
}
