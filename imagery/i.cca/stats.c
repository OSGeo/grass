#include <grass/gis.h>
#include "local_proto.h"


int
within(int samptot, int nclass, double nsamp[MC], double cov[MC][MX][MX],
       double w[MX][MX], int bands)
{
    int i, j, k;

    /* Initialize within class covariance matrix */
    for (i = 1; i <= bands; i++)
	for (j = 1; j <= bands; j++)
	    w[i][j] = 0.0;

    for (i = 1; i <= nclass; i++)
	for (j = 1; j <= bands; j++)
	    for (k = 1; k <= bands; k++)
		w[j][k] += (nsamp[i] - 1) * cov[i][j][k];

    for (i = 1; i <= bands; i++)
	for (j = 1; j <= bands; j++)
	    w[i][j] = (1.0 / ((double)(samptot - nclass))) * w[i][j];

    return 0;
}


int
between(int samptot, int nclass, double nsamp[MC], double mu[MC][MX],
	double p[MX][MX], int bands)
{
    int i, j, k;
    double tmp0[MX][MX], tmp1[MX][MX], tmp2[MX][MX];
    double newvec[MX];

    for (i = 0; i < MX; i++)
	newvec[i] = 0.0;

    for (i = 1; i <= bands; i++)
	for (j = 1; j <= bands; j++)
	    tmp1[i][j] = tmp2[i][j] = 0.0;

    /*  for (i = 1 ; i <= nclass ; i++)
       product(mu[i],nsamp[i],tmp0,tmp1,bands);
       for (i = 1 ; i <= nclass ; i++)
       for (j = 1 ; j <= bands ; j++)
       newvec[j] += nsamp[i] * mu[i][j];
       for (i = 1 ; i <= bands ; i++)
       for (j = 1 ; i <= bands ; j++)
       tmp2[i][j] = (newvec[i] * newvec[j]) / samptot;
       for (i = 1 ; i <= bands ; i++)
       for (j = 1 ; j <= bands ; j++)
       p[i][j] = (tmp1[i][j] - tmp2[i][j]) / (nclass - 1);
     */

    for (i = 1; i <= nclass; i++)
	for (j = 1; j <= bands; j++)
	    newvec[j] += nsamp[i] * mu[i][j];
    for (i = 1; i <= bands; i++)
	for (j = 1; j <= bands; j++)
	    tmp1[i][j] = (newvec[i] * newvec[j]) / samptot;

    for (k = 1; k <= nclass; k++) {
	product(mu[k], nsamp[k], tmp0, bands);
	for (i = 1; i <= bands; i++)
	    for (j = 1; j <= bands; j++)
		tmp2[i][j] += tmp0[i][j] - tmp1[i][j];
    }

    for (i = 1; i <= bands; i++)
	for (j = 1; j <= bands; j++)
	    p[i][j] = tmp2[i][j] / (nclass - 1);

    return 0;
}
