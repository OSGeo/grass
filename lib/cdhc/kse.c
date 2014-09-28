#include <stdio.h>
#include <math.h>
#include <grass/cdhc.h>
#include "local_proto.h"


double *Cdhc_kolmogorov_smirnov_exp(double *x, int n)
{
    static double y[2];
    double *d, sqrtn;

    d = Cdhc_dmax_exp(x, n);
    sqrtn = sqrt((double)n);

    y[1] = (d[0] > d[1]) ? d[0] : d[1];
    y[0] = (y[1] - 0.2 / n) * (sqrtn + 0.5 / sqrtn + 0.26);

#ifdef NOISY
    fprintf(stdout, "  TEST17 KSD(E) =%10.4f\n", y[0]);
#endif /* NOISY */

    return y;
}
