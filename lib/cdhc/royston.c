#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "local_proto.h"


/*-
 * driver program for AS 181: Royston's extension of the Shapiro-Wilk
 * W statistic to n=2000
 * needs as181.c as177.c as241.c Cdhc_dcmp.c as66.c 
 */

double *Cdhc_royston(double *x, int n)
{
    static double y[2];
    double *a, eps, w, pw, mean = 0, ssq = 0, *xcopy;
    int i, ifault, n2;

    n2 = (int)floor((double)n / 2);

#ifndef lint
    if ((a = (double *)malloc(n2 * sizeof(double))) == NULL) {
	fprintf(stderr, "Memory error in royston\n");
	exit(EXIT_FAILURE);
    }
    if ((xcopy = (double *)malloc(n * sizeof(double))) == NULL) {
	fprintf(stderr, "Memory error in royston\n");
	exit(EXIT_FAILURE);
    }
#endif /* lint */

    for (i = 0; i < n; ++i) {
	xcopy[i] = x[i];
	mean += x[i];
    }
    mean /= n;

    qsort(xcopy, n, sizeof(double), Cdhc_dcmp);

    for (i = 0; i < n; ++i)
	ssq += (mean - x[i]) * (mean - x[i]);

    wcoef(a, n, n2, &eps, &ifault);

    if (ifault == 0)
	wext(xcopy, n, ssq, a, n2, eps, &w, &pw, &ifault);
    else {
	fprintf(stderr, "Error in wcoef()\n");
	return (double *)NULL;
    }

    if (ifault == 0) {
	y[0] = w;
	y[1] = pw;
    }
    else {
	fprintf(stderr, "Error in wcoef()\n");
	return (double *)NULL;
    }

    free(a);
    free(xcopy);

    return y;
}
