#include <grass/gis.h>
#include "local_proto.h"

double *vector(int nl, int nh)
{
    double *v;

    v = (double *)G_malloc((unsigned)(nh - nl + 1) * sizeof(double));
    return v - nl;
}

double **matrix(int nrl, int nrh, int ncl, int nch)
{
    int i;
    double **m;

    m = (double **)G_malloc((unsigned)(nrh - nrl + 1) * sizeof(double *));
    m -= nrl;

    for (i = nrl; i <= nrh; i++) {
	m[i] = (double *)G_malloc((unsigned)(nch - ncl + 1) * sizeof(double));
	m[i] -= ncl;
    }
    return m;
}

int free_vector(double *v, int nl, int nh)
{
    G_free((char *)(v + nl));

    return 0;
}

int free_matrix(double **m, int nrl, int nrh, int ncl, int nch)
{
    int i;

    for (i = nrh; i >= nrl; i--)
	G_free((char *)(m[i] + ncl));
    G_free((char *)(m + nrl));

    return 0;
}
