#include <stdio.h>
#include <math.h>
#include <grass/cdhc.h>
#include "local_proto.h"


double *Cdhc_kuipers_v_exp(double *x, int n)
{
    static double y[2];
    double *d, r;

    d = Cdhc_dmax_exp(x, n);
    r = sqrt((double)n);

    y[1] = d[0] + d[1];
    y[0] = (y[1] - 0.2 / n) * (r + 0.35 / r + 0.24);

#ifdef NOISY
    fprintf(stdout, "  TEST18 KV(E)  =%10.4f\n", y[0]);
#endif /* NOISY */

    return y;
}
