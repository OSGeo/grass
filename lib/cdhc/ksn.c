#include <stdio.h>
#include <math.h>
#include <grass/cdhc.h>
#include "local_proto.h"


double *Cdhc_kolmogorov_smirnov(double *x, int n)
{
    static double y[2];
    double *d, sqrtn;

    sqrtn = sqrt((double)n);
    d = Cdhc_dmax(x, n);

    y[1] = (d[0] > d[1]) ? d[0] : d[1];
    y[0] = y[1] * (sqrtn + 0.85 / sqrtn - 0.01);

#ifdef NOISY
    fprintf(stdout, "  TEST10 KSD(N) =%10.4f\n", y[0]);
    fprintf(stdout, "  TEST11 KSD    =%10.4f\n", y[1]);
#endif /* NOISY */

    return y;
}
