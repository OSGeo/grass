/*  eigen.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include <stdlib.h>
#include "ccmath.h"
void eigen(double *a, double *ev, int n)
{
    double *dp;

    dp = (double *)calloc(n, sizeof(double));
    housev(a, ev, dp, n);
    qrevec(ev, a, dp, n);
    trnm(a, n);
    free(dp);
}
