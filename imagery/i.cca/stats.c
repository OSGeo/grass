#include <grass/gis.h>
#include <grass/gmath.h>

#include "local_proto.h"

int
within(int samptot, int nclass, double *nsamp, double ***cov,
       double **w, int bands)
{
    int i, j, k;

    /* Initialize within class covariance matrix */
    for (i = 0; i < bands; i++)
	for (j = 0; j < bands; j++)
	    w[i][j] = 0.0;

    for (i = 0; i < nclass; i++)
	for (j = 0; j < bands; j++)
	    for (k = 0; k < bands; k++)
		w[j][k] += (nsamp[i] - 1) * cov[i][j][k];

    for (i = 0; i < bands; i++)
	for (j = 0; j < bands; j++)
	    w[i][j] = (1.0 / ((double)(samptot - nclass))) * w[i][j];

    return 0;
}


int
between(int samptot, int nclass, double *nsamp, double **mu,
	double **p, int bands)
{
    int i, j, k;
    double **tmp0, **tmp1, **tmp2;
    double *newvec;

    tmp0 = G_alloc_matrix(bands, bands);
    tmp1 = G_alloc_matrix(bands, bands);
    tmp2 = G_alloc_matrix(bands, bands);
    newvec = G_alloc_vector(bands);

    for (i = 0; i < nclass; i++)
	for (j = 0; j < bands; j++)
	    newvec[j] += nsamp[i] * mu[i][j];
    for (i = 0; i < bands; i++)
	for (j = 0; j < bands; j++)
	    tmp1[i][j] = (newvec[i] * newvec[j]) / samptot;

    for (k = 0; k < nclass; k++) {
	product(mu[k], nsamp[k], tmp0, bands);
	for (i = 0; i < bands; i++)
	    for (j = 0; j < bands; j++)
		tmp2[i][j] += tmp0[i][j] - tmp1[i][j];
    }

    for (i = 0; i < bands; i++)
	for (j = 0; j < bands; j++)
	    p[i][j] = tmp2[i][j] / (nclass - 1);

    G_free_matrix(tmp0);
    G_free_matrix(tmp1);
    G_free_matrix(tmp2);
    G_free_vector(newvec);

    return 0;
}

