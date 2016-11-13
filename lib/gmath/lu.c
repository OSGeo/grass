#include <math.h>
#include <grass/gis.h>
#include <grass/gmath.h>


#define TINY 1.0e-20;

/*!
 * \brief LU decomposition
 *
 * \param a double **
 * \param n int
 * \param indx int *
 * \param d double *
 *
 * \return 0 on singular matrix, 1 on success
 */
int G_ludcmp(double **a, int n, int *indx, double *d)
{
    int i, imax = 0, j, k;
    double big, dum, sum, temp;
    double *vv;
    int is_singular = FALSE;

    vv = G_alloc_vector(n);
    *d = 1.0;
/* this pragma works, but doesn't really help speed things up */
/* #pragma omp parallel for private(i, j, big, temp) shared(n, a, vv, is_singular) */
    for (i = 0; i < n; i++) {
	big = 0.0;
	for (j = 0; j < n; j++)
	    if ((temp = fabs(a[i][j])) > big)
		big = temp;

	if (big == 0.0) {
	    is_singular = TRUE;
	    break;
	}

	vv[i] = 1.0 / big;
    }
    if (is_singular) {
	*d = 0.0;
	return 0;	/* Singular matrix  */
    }

    for (j = 0; j < n; j++) {
	for (i = 0; i < j; i++) {
	    sum = a[i][j];
	    for (k = 0; k < i; k++)
		sum -= a[i][k] * a[k][j];
	    a[i][j] = sum;
	}

	big = 0.0;
/* not very efficient, but this pragma helps speed things up a bit */
#pragma omp parallel for private(i, k, sum, dum) shared(j, n, a, vv, big, imax)
	for (i = j; i < n; i++) {
	    sum = a[i][j];
	    for (k = 0; k < j; k++)
		sum -= a[i][k] * a[k][j];
	    a[i][j] = sum;
	    if ((dum = vv[i] * fabs(sum)) >= big) {
		big = dum;
		imax = i;
	    }
	}
	if (j != imax) {
	    for (k = 0; k < n; k++) {
		dum = a[imax][k];
		a[imax][k] = a[j][k];
		a[j][k] = dum;
	    }
	    *d = -(*d);
	    vv[imax] = vv[j];
	}
	indx[j] = imax;
	if (a[j][j] == 0.0)
	    a[j][j] = TINY;
	if (j != n) {
	    dum = 1.0 / (a[j][j]);
	    for (i = j + 1; i < n; i++)
		a[i][j] *= dum;
	}
    }
    G_free_vector(vv);

    return 1;
}

#undef TINY


/*!
 * \brief LU backward substitution
 *
 * \param a double **
 * \param n int
 * \param indx int *
 * \param b double []
 *
 * \return void
 */
void G_lubksb(double **a, int n, int *indx, double b[])
{
    int i, ii, ip, j;
    double sum;

    ii = -1;
    for (i = 0; i < n; i++) {
	ip = indx[i];
	sum = b[ip];
	b[ip] = b[i];
	if (ii >= 0)
	    for (j = ii; j < i; j++)
		sum -= a[i][j] * b[j];
	else if (sum)
	    ii = i;
	b[i] = sum;
    }
    for (i = n - 1; i >= 0; i--) {
	sum = b[i];
	for (j = i + 1; j < n; j++)
	    sum -= a[i][j] * b[j];
	b[i] = sum / a[i][i];
    }
}
