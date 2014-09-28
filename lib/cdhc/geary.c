#include <stdio.h>
#include <math.h>


double *Cdhc_geary_test(double *x, int n)
{
    int i;
    static double y[2];
    double diff, s = 0.0, mean = 0.0;

    y[0] = 0.0;
    for (i = 0; i < n; ++i)
	mean += x[i];

    mean /= n;

    for (i = 0; i < n; ++i) {
	diff = x[i] - mean;
	y[0] += fabs(diff);
	s += diff * diff;
    }

    s *= n;
    y[0] /= sqrt(s);
    y[1] = (y[0] - 0.7979) * sqrt((double)n) / 0.2123;

#ifdef NOISY
    fprintf(stdout, "  TEST2  GTN    =%10.4f   Z(GTN) =%10.4f\n", y[0], y[1]);
#endif /* NOISY */

    return y;
}
