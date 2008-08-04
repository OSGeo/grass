#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/gmath.h>


/***************************************************************************/

/* this does not use the Jacobi method, but it should give the same result */

int jacobi(double a[MX][MX], long n, double d[MX], double v[MX][MX])
{
    double *aa[MX], *vv[MX], *dd;
    int i;

    for (i = 0; i < n; i++) {
	aa[i] = &a[i + 1][1];
	vv[i] = &v[i + 1][1];
    }
    dd = &d[1];
    eigen(aa, vv, dd, n);

    return 0;
}

/***************************************************************************/

static int egcmp(const void *pa, const void *pb)
{
    const double *a = *(const double *const *)pa;
    const double *b = *(const double *const *)pb;

    if (*a > *b)
	return -1;
    if (*a < *b)
	return 1;

    return 0;
}

int egvorder(double d[MX], double z[MX][MX], long bands)
{
    double *buff;
    double **tmp;
    int i, j;

    /* allocate temporary matrix */

    buff = (double *)G_malloc(bands * (bands + 1) * sizeof(double));
    tmp = (double **)G_malloc(bands * sizeof(double *));
    for (i = 0; i < bands; i++)
	tmp[i] = &buff[i * (bands + 1)];

    /* concatenate (vertically) z and d into tmp */

    for (i = 0; i < bands; i++) {
	for (j = 0; j < bands; j++)
	    tmp[i][j + 1] = z[j + 1][i + 1];
	tmp[i][0] = d[i + 1];
    }

    /* sort the combined matrix */

    qsort(tmp, bands, sizeof(double *), egcmp);

    /* split tmp into z and d */

    for (i = 0; i < bands; i++) {
	for (j = 0; j < bands; j++)
	    z[j + 1][i + 1] = tmp[i][j + 1];
	d[i + 1] = tmp[i][0];
    }

    /* free temporary matrix */

    G_free(tmp);
    G_free(buff);

    return 0;
}

/***************************************************************************/

int transpose(double eigmat[MX][MX], long bands)
{
    int i, j;

    for (i = 1; i <= bands; i++)
	for (j = 1; j < i; j++) {
	    double tmp = eigmat[i][j];

	    eigmat[i][j] = eigmat[j][i];
	    eigmat[j][i] = tmp;
	}

    return 0;
}

/***************************************************************************/
