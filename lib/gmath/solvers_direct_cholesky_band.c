#include <stdlib.h>		
#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/gmath.h>
#include <grass/glocale.h>

/**
 * \brief Cholesky decomposition of a band matrix
 *
 * \param A (double**) the input band matrix
 * \param T (double**) the resulting lower tringular band matrix
 * \param rows (int) number of rows
 * \param bandwidth (int) the bandwidth of the band matrix
 *
 * */

void G_math_cholesky_band_decomposition(double **A, double **T, int rows, int bandwidth)
{
    int i, j, k, end;
    double sum;

    G_debug(2, "G_math_cholesky_band_decomposition(): n=%d  bandwidth=%d", rows, bandwidth);

    for (i = 0; i < rows; i++) {
	G_percent(i, rows, 2);
	for (j = 0; j < bandwidth; j++) {
	    sum = A[i][j];
	    /* start = 1 */
	    /* end = bandwidth - j or i + 1 */
	    end = ((bandwidth - j) < (i + 1) ? (bandwidth - j) : (i + 1));
	    for (k = 1; k < end; k++)
		sum -= T[i - k][k] * T[i - k][j + k];
	    if (j == 0) {
		if (sum <= 0.0)
		    G_fatal_error(_("Decomposition failed at row %i and col %i"), i, j);
		T[i][0] = sqrt(sum);
	    }
	    else
		T[i][j] = sum / T[i][0];
	}
    }

    G_percent(i, rows, 2);
    return;
}

/**
 * \brief Cholesky band matrix solver for linear equation systems of type Ax = b 
 *
 * \param A (double**) the input band matrix
 * \param x (double*) the resulting vector, result is written in here
 * \param b (double*) the right hand side of Ax = b
 * \param rows (int) number of rows
 * \param bandwidth (int) the bandwidth of the band matrix
 *
 * */

void G_math_solver_cholesky_band(double **A, double *x, double *b, int rows, int bandwidth)
{

    double **T;

    T = G_alloc_matrix(rows, bandwidth);

    G_math_cholesky_band_decomposition(A, T, rows, bandwidth);	/* T computation                */
    G_math_cholesky_band_substitution(T, x, b, rows, bandwidth);

    G_free_matrix(T);

    return;
}

/**
 * \brief Forward and backward substitution of a lower tringular band matrix of A from system Ax = b
 *
 * \param T (double**) the lower band triangle band matrix
 * \param x (double*) the resulting vector
 * \param b (double*) the right hand side of Ax = b
 * \param rows (int) number of rows
 * \param bandwidth (int) the bandwidth of the band matrix
 *
 * */

void G_math_cholesky_band_substitution(double **T, double *x, double *b, int rows, int bandwidth)
{

    int i, j, start, end;

    /* Forward substitution */
    x[0] = b[0] / T[0][0];
    for (i = 1; i < rows; i++) {
	x[i] = b[i];
	/* start = 0 or i - bandwidth + 1 */
	start = ((i - bandwidth + 1) < 0 ? 0 : (i - bandwidth + 1));
	/* end = i */
	for (j = start; j < i; j++)
	    x[i] -= T[j][i - j] * x[j];
	x[i] = x[i] / T[i][0];
    }

    /* Backward substitution */
    x[rows - 1] = x[rows - 1] / T[rows - 1][0];
    for (i = rows - 2; i >= 0; i--) {
	/* start = i + 1 */
	/* end = rows or bandwidth + i */
	end = (rows < (bandwidth + i) ? rows : (bandwidth + i));
	for (j = i + 1; j < end; j++)
	    x[i] -= T[i][j - i] * x[j];
	x[i] = x[i] / T[i][0];
    }

    return;
}

/*--------------------------------------------------------------------------------------*/
/* Tcholetsky matrix invertion */

void G_math_cholesky_band_invert(double **A, double *invAdiag, int rows, int bandwidth)
{
    double **T = NULL;
    double *vect = NULL;
    int i, j, k, start;
    double sum;

    T = G_alloc_matrix(rows, bandwidth);
    vect = G_alloc_vector(rows);

    /* T computation                */
    G_math_cholesky_band_decomposition(A, T, rows, bandwidth);

    /* T Diagonal invertion */
    for (i = 0; i < rows; i++) {
	T[i][0] = 1.0 / T[i][0];
    }

    /* A Diagonal invertion */
    for (i = 0; i < rows; i++) {
	vect[0] = T[i][0];
	invAdiag[i] = vect[0] * vect[0];
	for (j = i + 1; j < rows; j++) {
	    sum = 0.0;
	    /* start = i or j - bandwidth + 1 */
	    start = ((j - bandwidth + 1) < i ? i : (j - bandwidth + 1));
	    /* end = j */
	    for (k = start; k < j; k++) {
		sum -= vect[k - i] * T[k][j - k];
	    }
	    vect[j - i] = sum * T[j][0];
	    invAdiag[i] += vect[j - i] * vect[j - i];
	}
    }

    G_free_matrix(T);
    G_free_vector(vect);

    return;
}

/*--------------------------------------------------------------------------------------*/
/* Tcholetsky matrix solution and invertion */

void G_math_solver_cholesky_band_invert(double **A, double *x, double *b, double *invAdiag,
		   int rows, int bandwidth)
{

    double **T = NULL;
    double *vect = NULL;
    int i, j, k, start, end;
    double sum;

    T = G_alloc_matrix(rows, bandwidth);
    vect = G_alloc_vector(rows);

    /* T computation                */
    G_math_cholesky_band_decomposition(A, T, rows, bandwidth);
    G_math_cholesky_band_substitution(T, x, b, rows, bandwidth);

    /* T Diagonal invertion */
    for (i = 0; i < rows; i++) {
	T[i][0] = 1.0 / T[i][0];
    }

    /* A Diagonal invertion */
    for (i = 0; i < rows; i++) {
	vect[0] = T[i][0];
	invAdiag[i] = vect[0] * vect[0];
	for (j = i + 1; j < rows; j++) {
	    sum = 0.0;
	    /* start = i or j - bandwidth + 1 */
	    start = ((j - bandwidth + 1) < i ? i : (j - bandwidth + 1));
	    /* end = j */
	    for (k = start; k < j; k++) {
		sum -= vect[k - i] * T[k][j - k];
	    }
	    vect[j - i] = sum * T[j][0];
	    invAdiag[i] += vect[j - i] * vect[j - i];
	}
    }

    G_free_matrix(T);
    G_free_vector(vect);

    return;
}

/**
 * \brief Convert a symmetrix matrix into a band matrix
 *
 * \verbatim
 Symmetric matrix with bandwidth of 3

 5 2 1 0
 2 5 2 1
 1 2 5 2
 0 1 2 5

 will be converted into the band matrix
 
 5 2 1
 5 2 1
 5 2 0
 5 0 0

  \endverbatim

 * \param A (double**) the symmetric matrix
 * \param rows (int)
 * \param bandwidth (int)
 * \return B (double**) new created band matrix 
 * */

double **G_math_matrix_to_band_matrix(double **A, int rows, int bandwidth)
{
    int i, j;
    double **B = G_alloc_matrix(rows, bandwidth);
    double tmp;

    for(i = 0; i < rows; i++) {
       for(j = 0; j < bandwidth; j++) {
          if(i + j < rows) {
            tmp = A[i][i + j]; 
            B[i][j] = tmp;
          } else {
            B[i][j] = 0.0;
          }
       }
    }

    return B;
}


/**
 * \brief Convert a band matrix into a symmetric matrix
 *
 * \verbatim
 Such a band matrix with banwidth 3
 
 5 2 1
 5 2 1
 5 2 0
 5 0 0

 Will be converted into this symmetric matrix

 5 2 1 0
 2 5 2 1
 1 2 5 2
 0 1 2 5

  \endverbatim
 * \param A (double**) the band matrix
 * \param rows (int)
 * \param bandwidth (int)
 * \return B (double**) new created symmetric matrix 
 * */

double **G_math_band_matrix_to_matrix(double **A, int rows, int bandwidth)
{
    int i, j;
    double **B = G_alloc_matrix(rows, rows);
    double tmp;

    for(i = 0; i < rows; i++) {
       for(j = 0; j < bandwidth; j++) {
          tmp = A[i][j];
          if(i + j < rows) {
            B[i][i + j] = tmp; 
          } 
       }
    }

    /*Symmetry*/
    for(i = 0; i < rows; i++) {
       for(j = i; j < rows; j++) {
          B[j][i] = B[i][j]; 
       }
    }

    return B;
}


