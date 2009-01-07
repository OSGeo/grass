/* taken from i.pca */

#include <stdlib.h>
#include <grass/gmath.h>
#include <grass/gis.h>


static int egcmp(const void *pa, const void *pb);


/*!
 * \fn int eigen (double **M, double **Vectors, double *lambda, int n)
 *
 * \brief Computes eigenvalues (and eigen vectors if desired) for
 * symmetric matices.
 *
 * Computes eigenvalues (and eigen vectors if desired) for symmetric matices.
 *
 * \param M Input matrix
 * \param Vectors eigen output vector matrix
 * \param lambda Output eigenvalues
 * \param n Input matrix dimension
 * \return int
 */

int eigen(double **M,		/* Input matrix */
	  double **Vectors,	/* eigen vector matrix -output */
	  double *lambda,	/* Output eigenvalues */
	  int n			/* Input matrix dimension */
    )
{
    int i, j;
    double **a, *e;

    a = G_alloc_matrix(n, n);
    e = G_alloc_vector(n);

    for (i = 0; i < n; i++)
	for (j = 0; j < n; j++)
	    a[i][j] = M[i][j];

    G_tred2(a, n, lambda, e);
    G_tqli(lambda, e, n, a);

    /* Returns eigenvectors */
    if (Vectors)
	for (i = 0; i < n; i++)
	    for (j = 0; j < n; j++)
		Vectors[i][j] = a[i][j];

    G_free_matrix(a);
    G_free_vector(e);

    return 0;
}


/*!
 * \fn int egvorder2 (double *d, double **z, long bands)
 *
 * \brief
 *
 * Returns 0.
 *
 * \param d
 * \param z
 * \param bands
 * \return int
 */

int egvorder2(double *d, double **z, long bands)
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
	    tmp[i][j + 1] = z[j][i];
	tmp[i][0] = d[i];
    }

    /* sort the combined matrix */
    qsort(tmp, bands, sizeof(double *), egcmp);

    /* split tmp into z and d */
    for (i = 0; i < bands; i++) {
	for (j = 0; j < bands; j++)
	    z[j][i] = tmp[i][j + 1];
	d[i] = tmp[i][0];
    }

    /* free temporary matrix */
    G_free(tmp);
    G_free(buff);

    return 0;
}


/*!
 * \fn int transpose2 (double **eigmat, long bands)
 *
 * \brief
 *
 * Returns 0.
 *
 * \param eigmat
 * \param bands
 * \return int
 */

int transpose2(double **eigmat, long bands)
{
    int i, j;

    for (i = 0; i < bands; i++)
	for (j = 0; j < i; j++) {
	    double tmp = eigmat[i][j];

	    eigmat[i][j] = eigmat[j][i];
	    eigmat[j][i] = tmp;
	}

    return 0;
}


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
