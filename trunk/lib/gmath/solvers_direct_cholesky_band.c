#include <stdlib.h>		
#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/gmath.h>
#include <grass/glocale.h>

/**
 * \brief Cholesky decomposition of a symmetric band matrix
 *
 * \param A (double**) the input symmetric band matrix
 * \param T (double**) the resulting lower tringular symmetric band matrix
 * \param rows (int) number of rows
 * \param bandwidth (int) the bandwidth of the symmetric band matrix
 *
 * */

void G_math_cholesky_sband_decomposition(double **A, double **T, int rows, int bandwidth)
{
    int i, j, k, end;
    double sum;

    G_debug(2, "G_math_cholesky_sband_decomposition(): n=%d  bandwidth=%d", rows, bandwidth);

    for (i = 0; i < rows; i++) {
	G_percent(i, rows, 9);
        /* For j = 0 */
	sum = A[i][0];
	end = ((bandwidth - 0) < (i + 1) ? (bandwidth - 0) : (i + 1));
	for (k = 1; k < end; k++)
	    sum -= T[i - k][k] * T[i - k][0 + k];
	if (sum <= 0.0)
	    G_fatal_error(_("Decomposition failed at row %i and col %i"), i, 0);
	T[i][0] = sqrt(sum);

        #pragma omp parallel for schedule (static) private(j, k, end, sum) shared(A, T, i, bandwidth)
	for (j = 1; j < bandwidth; j++) {
	    sum = A[i][j];
	    end = ((bandwidth - j) < (i + 1) ? (bandwidth - j) : (i + 1));
	    for (k = 1; k < end; k++)
		sum -= T[i - k][k] * T[i - k][j + k];
       	    T[i][j] = sum / T[i][0];
	}
    }

    G_percent(i, rows, 2);
    return;
}

/**
 * \brief Cholesky symmetric band matrix solver for linear equation systems of type Ax = b 
 *
 * \param A (double**) the input symmetric band matrix
 * \param x (double*) the resulting vector, result is written in here
 * \param b (double*) the right hand side of Ax = b
 * \param rows (int) number of rows
 * \param bandwidth (int) the bandwidth of the symmetric band matrix
 *
 * */

void G_math_solver_cholesky_sband(double **A, double *x, double *b, int rows, int bandwidth)
{

    double **T;

    T = G_alloc_matrix(rows, bandwidth);

    G_math_cholesky_sband_decomposition(A, T, rows, bandwidth);	/* T computation                */
    G_math_cholesky_sband_substitution(T, x, b, rows, bandwidth);

    G_free_matrix(T);

    return;
}

/**
 * \brief Forward and backward substitution of a lower tringular symmetric band matrix of A from system Ax = b
 *
 * \param T (double**) the lower triangle symmetric band matrix
 * \param x (double*) the resulting vector
 * \param b (double*) the right hand side of Ax = b
 * \param rows (int) number of rows
 * \param bandwidth (int) the bandwidth of the symmetric band matrix
 *
 * */

void G_math_cholesky_sband_substitution(double **T, double *x, double *b, int rows, int bandwidth)
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

void G_math_cholesky_sband_invert(double **A, double *invAdiag, int rows, int bandwidth)
{
    double **T = NULL;
    double *vect = NULL;
    int i, j, k, start;
    double sum;

    T = G_alloc_matrix(rows, bandwidth);
    vect = G_alloc_vector(rows);

    /* T computation                */
    G_math_cholesky_sband_decomposition(A, T, rows, bandwidth);

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

void G_math_solver_cholesky_sband_invert(double **A, double *x, double *b, double *invAdiag,
		   int rows, int bandwidth)
{

    double **T = NULL;
    double *vect = NULL;
    int i, j, k, start;
    double sum;

    T = G_alloc_matrix(rows, bandwidth);
    vect = G_alloc_vector(rows);

    /* T computation                */
    G_math_cholesky_sband_decomposition(A, T, rows, bandwidth);
    G_math_cholesky_sband_substitution(T, x, b, rows, bandwidth);

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

