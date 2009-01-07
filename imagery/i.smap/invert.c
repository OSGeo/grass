#include <math.h>
#include <grass/gis.h>
#include <grass/gmath.h>

int invert(
	      /* inverts a matrix of arbitrary size input as a 2D array. */
	      double **a,	/* input/output matrix */
	      int n		/* dimension */
    )
{
    int status;
    int i, j, *indx;
    double **y, *col, d;

    indx = G_alloc_ivector(n);
    y = G_alloc_matrix(n, n);
    col = G_alloc_vector(n);

    if ((status = G_ludcmp(a, n, indx, &d))) {
	for (j = 0; j < n; j++) {
	    for (i = 0; i < n; i++)
		col[i] = 0.0;
	    col[j] = 1.0;
	    G_lubksb(a, n, indx, col);
	    for (i = 0; i < n; i++)
		y[i][j] = col[i];
	}

	for (i = 0; i < n; i++)
	    for (j = 0; j < n; j++)
		a[i][j] = y[i][j];
    }

    G_free_ivector(indx);
    G_free_matrix(y);
    G_free_vector(col);

    return (status);
}
