#include <math.h>
#include <grass/gis.h>
#include <grass/gmath.h>
#include "local_proto.h"


int product(double vector[MX], double factor, double matrix1[MX][MX],
	    int bands)
{
    int i, j;

    for (i = 1; i <= bands; i++)
	for (j = 1; j <= bands; j++) {
	    matrix1[i][j] = (double)factor *(vector[i] * vector[j]);
	}
    return 0;
}


int setdiag(double eigval[MX], int bands, double l[MX][MX])
{
    int i, j;

    for (i = 1; i <= bands; i++)
	for (j = 1; j <= bands; j++)
	    if (i == j)
		l[i][j] = eigval[i];
	    else
		l[i][j] = 0.0;
    return 0;
}


int
getsqrt(double w[MX][MX], int bands, double l[MX][MX], double eigmat[MX][MX])
{
    int i;
    double tmp[MX][MX];

    for (i = 1; i <= bands; i++)
	l[i][i] = 1.0 / sqrt(l[i][i]);
    matmul(tmp, eigmat, l, bands);
    transpose(eigmat, bands);
    matmul(w, tmp, eigmat, bands);
    return 0;
}


int solveq(double q[MX][MX], int bands, double w[MX][MX], double p[MX][MX])
{
    double tmp[MX][MX];

    matmul(tmp, w, p, bands);
    matmul(q, tmp, w, bands);
    return 0;
}


int matmul(double res[MX][MX], double m1[MX][MX], double m2[MX][MX], int dim)
{
    int i, j, k;
    double sum;

    for (i = 1; i <= dim; i++) {
	for (j = 1; j <= dim; j++) {
	    sum = 0.0;
	    for (k = 1; k <= dim; k++)
		sum += m1[i][k] * m2[k][j];
	    res[i][j] = sum;
	}
    }

    return 0;
}
