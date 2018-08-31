#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "local_proto.h"


double *Cdhc_cramer_von_mises_exp(double *x, int n)
{
    static double y[2];
    double *xcopy, mean = 0.0, fx, fn2, sum4 = 0.0;
    int i;

    if ((xcopy = (double *)malloc(n * sizeof(double))) == NULL) {
	fprintf(stderr, "Memory error in Cdhc_cramer_von_mises_exp\n");
	exit(EXIT_FAILURE);
    }

    for (i = 0; i < n; ++i) {
	xcopy[i] = x[i];
	mean += x[i];
    }
    mean /= n;

    qsort(xcopy, n, sizeof(double), Cdhc_dcmp);

    for (i = 0; i < n; ++i) {

    /*-
    a = (2 * i + 1) * log (fx);
    b = (2 * i + 1) * (xcopy[n-i-1] * (-1.0 / mean));
    sum3 += a + b;
    */
	fx = 1 - exp(xcopy[i] * (-1.0 / mean));
	fn2 = (double)(2.0 * i + 1) / (2 * n);
	sum4 += (fx - fn2) * (fx - fn2);
    }

  /*-
  cvm = 1.0 / (n * 12) + sum4;
  cvmod = cvm * (0.16 / n + 1.0);
  */
    y[0] = (1.0 / (n * 12) + sum4) * (0.16 / n + 1.0);

#ifdef NOISY
    fprintf(stdout, "  TEST16 CVM(E) =%10.4f\n", y[0]);
#endif /* NOISY */

    free(xcopy);

    return y;
}
