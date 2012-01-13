#include <grass/gis.h>
#include <grass/gmath.h>

int invert(
	      /* inverts a matrix of arbitrary size input as a 2D array. */
	      double **a,	/* input/output matrix */
	      int n,		/* dimension */
	      double *det,	/* determinant */
	      int *indx,	/* indx = G_alloc_ivector(n);  */
	      double **y,	/* y = G_alloc_matrix(n,n); */
	      double *col	/* col = G_alloc_vector(n); */
    )
{
    int i, j;
    double d;

    if (G_ludcmp(a, n, indx, &d)) {
	for (j = 0; j < n; j++) {
	    d *= a[j][j];
	    for (i = 0; i < n; i++)
		col[i] = 0.0;
	    col[j] = 1.0;
	    G_lubksb(a, n, indx, col);
	    for (i = 0; i < n; i++)
		y[i][j] = col[i];
	}
	*det = d;

	for (i = 0; i < n; i++)
	    for (j = 0; j < n; j++)
		a[i][j] = y[i][j];

	return (1);
    }
    else {
	*det = 0.0;
	return (0);
    }
}
