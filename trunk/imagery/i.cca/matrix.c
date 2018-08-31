#include <math.h>
#include <grass/gis.h>
#include <grass/gmath.h>
#include "local_proto.h"


int print_matrix(double **matrix, int bands)
{
    int i, j;

    for (i = 0; i < bands; i++)
    {
	for (j = 0; j < bands; j++) {
	    printf("%g ", matrix[i][j]);
	}
        printf("\n");
    }
    return 0;
}

int product(double *vector, double factor, double **matrix1,
	    int bands)
{
    int i, j;

    for (i = 0; i < bands; i++)
	for (j = 0; j < bands; j++) {
	    matrix1[i][j] = (double)factor *(vector[i] * vector[j]);
	}
    return 0;
}


int setdiag(double *eigval, int bands, double **l)
{
    int i, j;

    for (i = 0; i < bands; i++)
	for (j = 0; j < bands; j++)
	    if (i == j)
		l[i][j] = eigval[i];
	    else
		l[i][j] = 0.0;
    return 0;
}


int
getsqrt(double **w, int bands, double **l, double **eigmat)
{
    int i;
    double **tmp;

    tmp = G_alloc_matrix(bands, bands);

    for (i = 0; i < bands; i++)
	l[i][i] = 1.0 / sqrt(l[i][i]);

    G_math_d_AB(eigmat, l, tmp, bands, bands, bands);
    G_math_d_A_T(eigmat, bands);
    G_math_d_AB(tmp, eigmat, w, bands, bands, bands);

    G_free_matrix(tmp);

    return 0;
}


int solveq(double **q, int bands, double **w, double **p)
{
    double **tmp;

    tmp = G_alloc_matrix(bands, bands);

    G_math_d_AB(w, p, tmp, bands, bands, bands);
    G_math_d_AB(tmp, w, q, bands, bands, bands);

    G_free_matrix(tmp);

    return 0;
}

