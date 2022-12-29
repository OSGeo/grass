/*  matprt.c    CCMATH mathematics library source code.
 *
 *  Copyright (C)  2000   Daniel A. Atkinson    All rights reserved.
 *  This code may be redistributed under the terms of the GNU library
 *  public license (LGPL). ( See the lgpl.license file for details.)
 * ------------------------------------------------------------------------
 */
#include <stdio.h>
void matprt(double *a, int n, int m, char *fmt)
{
    int i, j;

    double *p;

    for (i = 0, p = a; i < n; ++i) {
	for (j = 0; j < m; ++j)
	    printf(fmt, *p++);
	printf("\n");
    }
}

void fmatprt(FILE * fp, double *a, int n, int m, char *fmt)
{
    int i, j;

    double *p;

    for (i = 0, p = a; i < n; ++i) {
	for (j = 0; j < m; ++j)
	    fprintf(fp, fmt, *p++);
	fprintf(fp, "\n");
    }
}
