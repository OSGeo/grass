#include <grass/gis.h>

int eigen(
	     /* Computes eigenvalues (and eigen vectors if desired) for      *
	      *  symmetric matices.                                          */
	     double **M,	/* Input matrix */
	     double *lambda,	/* Output eigenvalues */
	     int n		/* Input matrix dimension */
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
    /*      for(i=0; i<n; i++) 
       for(j=0; j<n; j++) 
       M[i][j] = a[i][j]; */

    G_free_matrix(a);
    G_free_vector(e);

    return 0;
}
