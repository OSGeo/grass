#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/gmath.h>

static int egcmp(const void *pa, const void *pb);


int G_math_egvorder(double *d, double **z, long bands)
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
/***************************************************************************/
