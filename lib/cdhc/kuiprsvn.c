#include <stdio.h>
#include <math.h>
#include <grass/cdhc.h>
#include "local_proto.h"


double *Cdhc_kuipers_v(double *x, int n)
{
    static double y[2];
    double *d, sqrtn;

    sqrtn = sqrt((double)n);
    d = Cdhc_dmax(x, n);

    y[1] = d[0] + d[1];
    y[0] = y[1] * (sqrtn + 0.05 + 0.82 / sqrtn);

#ifdef NOISY
    fprintf(stdout, "  TEST5  KV(N)  =%10.4f\n", y[0]);
#endif /* NOISY */

    return y;
}
