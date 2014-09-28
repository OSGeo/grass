#include <stdio.h>
#include <math.h>


double *Cdhc_kotz_families(double *x, int n)
{
    static double y[2];
    int i;
    double a1, b1, a2, b3, c1, c2, c3, c4, c5, c6, lx;
    double sum1 = 0.0, sum2 = 0.0, sum4 = 0.0;

    for (i = 0; i < n; ++i) {
	sum1 += x[i];
	sum2 += log(x[i]);
    }

    b1 = sum1 / n;
    a1 = sum2 / n;

    for (i = 0; i < n; ++i) {
	lx = log(x[i]);
	sum4 += (lx - a1) * (lx - a1);
    }

    a2 = sum4 / n;
    b3 = exp(a1 * 2 + a2) * (exp(a2) - 1);
    c1 = log(a2 / b3);
    c2 = (exp(a2 * 4) + exp(a2 * 3) * 2 - 4) / 4 - a2 + exp(a2) * 0.75;
    c3 = a2 * (exp(a2) * 2 - 1) * (exp(a2) * 2 - 1);
    c4 = (exp(a2) - 1) * 2 * (exp(a2) - 1);
    c5 = c3 / c4;

    if (c2 < c5) {
#ifdef NOISY
	fprintf(stdout, "  WARNING!!! STATISTICS FOR THE NEXT TEST WILL\n");
	fprintf(stdout, "  NOT BE CALCULATED DUE TO SMALL LOGVARIANCE\n");
#endif /* NOISY */
	y[0] = 999999999.;
    }
    else {
	c6 = sqrt(c2 - c5) * 2.0 * sqrt((double)n);
	y[0] = c1 / c6;
    }

#ifdef NOISY
    fprintf(stdout, "  TEST24 KT(LN) =%10.4f\n", y[0]);
#endif /* NOISY */

    return y;
}
