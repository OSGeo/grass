
/*****************************************************************************
*
* MODULE:       Grass numerical math interface
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> googlemail <dot> com
*               
* PURPOSE:      linear equation system solvers
* 		part of the gmath library
*               
* COPYRIGHT:    (C) 2010 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/gmath.h>
#include <grass/glocale.h>

#define TINY 1.0e-20
#define COMP_PIVOT 100

/*!
 * \brief The gauss elimination solver for quardatic matrices
 *
 * This solver does not support sparse matrices
 * The matrix A will be overwritten.
 * The result is written to the vector x 
 *
 * \param A double **
 * \param x double *
 * \param b double *
 * \param rows int
 * \return int -- 1 success
 * */
int G_math_solver_gauss(double **A, double *x, double *b, int rows)
{
    G_message(_("Starting direct gauss elimination solver"));

    G_math_gauss_elimination(A, b, rows);
    G_math_backward_substitution(A, x, b, rows);

    return 1;
}

/*!
 * \brief The LU solver for quardatic matrices
 *
 * This solver does not support sparse matrices
 * The matrix A will be overwritten.
 * The result is written to the vector x in the G_math_les structure
 *
 *
 * \param A double **
 * \param x double *
 * \param b double *
 * \param rows int
 * \return int -- 1 success
 * */
int G_math_solver_lu(double **A, double *x, double *b, int rows)
{
    int i;

    double *c, *tmpv;

    G_message(_("Starting direct lu decomposition solver"));

    tmpv = G_alloc_vector(rows);
    c = G_alloc_vector(rows);

    G_math_lu_decomposition(A, b, rows);

#pragma omp parallel
    {

#pragma omp for  schedule (static) private(i)
	for (i = 0; i < rows; i++) {
	    tmpv[i] = A[i][i];
	    A[i][i] = 1;
	}

#pragma omp single
	{
	    G_math_forward_substitution(A, b, b, rows);
	}

#pragma omp for  schedule (static) private(i)
	for (i = 0; i < rows; i++) {
	    A[i][i] = tmpv[i];
	}

#pragma omp single
	{
	    G_math_backward_substitution(A, x, b, rows);
	}
    }

    G_free(c);
    G_free(tmpv);


    return 1;
}

/*!
 * \brief The choleksy decomposition solver for quardatic, symmetric
 * positiv definite matrices
 *
 * This solver does not support sparse matrices
 * The matrix A will be overwritten.
 * The result is written to the vector x 
 *
 * \param A double **
 * \param x double *
 * \param b double *
 * \param bandwidth int -- the bandwidth of the band matrix, if unsure set to rows
 * \param rows int
 * \return int -- 1 success
 * */
int G_math_solver_cholesky(double **A, double *x, double *b, int bandwidth,
			   int rows)
{

    G_message(_("Starting cholesky decomposition solver"));

    if (G_math_cholesky_decomposition(A, rows, bandwidth) != 1) {
	G_warning(_("Unable to solve the linear equation system"));
	return -2;
    }

    G_math_forward_substitution(A, b, b, rows);
    G_math_backward_substitution(A, x, b, rows);

    return 1;
}

/*!
 * \brief Gauss elimination
 *
 * To run this solver efficiently,
 * no pivoting is supported.
 * The matrix will be overwritten with the decomposite form
 * \param A double **
 * \param b double * 
 * \param rows int
 * \return void
 *
 * */
void G_math_gauss_elimination(double **A, double *b, int rows)
{
    int i, j, k;

    double tmpval = 0.0;

    for (k = 0; k < rows - 1; k++) {
#pragma omp parallel for schedule (static) private(i, j, tmpval) shared(k, A, b, rows)
	for (i = k + 1; i < rows; i++) {
	    tmpval = A[i][k] / A[k][k];
	    b[i] = b[i] - tmpval * b[k];
	    for (j = k + 1; j < rows; j++) {
		A[i][j] = A[i][j] - tmpval * A[k][j];
	    }
	}
    }

    return;
}

/*!
 * \brief lu decomposition
 *
 * To run this solver efficiently,
 * no pivoting is supported.
 * The matrix will be overwritten with the decomposite form
 *
 * \param A double **
 * \param b double * -- this vector is needed if its part of the linear equation system, otherwise set it to NULL
 * \param rows int
 * \return void
 *
 * */
void G_math_lu_decomposition(double **A, double *b, int rows)
{

    int i, j, k;

    for (k = 0; k < rows - 1; k++) {
#pragma omp parallel for schedule (static) private(i, j) shared(k, A, rows)
	for (i = k + 1; i < rows; i++) {
	    A[i][k] = A[i][k] / A[k][k];
	    for (j = k + 1; j < rows; j++) {
		A[i][j] = A[i][j] - A[i][k] * A[k][j];
	    }
	}
    }

    return;
}

/*!
 * \brief cholesky decomposition for symmetric, positiv definite matrices
 *        with bandwidth optimization
 *
 * The provided matrix will be overwritten with the lower and 
 * upper triangle matrix A = LL^T 
 *
 * \param A double **
 * \param rows int
 * \param bandwidth int -- the bandwidth of the matrix (0 > bandwidth <= cols)
 * \return void
 *
 * */
int G_math_cholesky_decomposition(double **A, int rows, int bandwidth)
{

    int i = 0, j = 0, k = 0;

    double sum_1 = 0.0;

    double sum_2 = 0.0;

    int colsize;

    if (bandwidth <= 0)
	bandwidth = rows;

    colsize = bandwidth;

    for (k = 0; k < rows; k++) {
#pragma omp parallel for schedule (static) private(i, j, sum_2) shared(A, k) reduction(+:sum_1)
	for (j = 0; j < k; j++) {
	    sum_1 += A[k][j] * A[k][j];
	}

	if (0 > (A[k][k] - sum_1)) {
	    G_warning("Matrix is not positive definite. break.");
	    return -1;
	}
	A[k][k] = sqrt(A[k][k] - sum_1);
	sum_1 = 0.0;

	if ((k + bandwidth) > rows) {
	    colsize = rows;
	}
	else {
	    colsize = k + bandwidth;
	}

#pragma omp parallel for schedule (static) private(i, j, sum_2) shared(A, k, sum_1, colsize)

	for (i = k + 1; i < colsize; i++) {
	    sum_2 = 0.0;
	    for (j = 0; j < k; j++) {
		sum_2 += A[i][j] * A[k][j];
	    }
	    A[i][k] = (A[i][k] - sum_2) / A[k][k];
	}

    }
    /* we need to copy the lower triangle matrix to the upper triangle */
#pragma omp parallel for schedule (static) private(i, k) shared(A, rows)
    for (k = 0; k < rows; k++) {
	for (i = k + 1; i < rows; i++) {
	    A[k][i] = A[i][k];
	}
    }


    return 1;
}

/*!
 * \brief backward substitution
 *
 * \param A double **
 * \param x double *
 * \param b double *
 * \param rows int
 * \return void
 *
 * */
void G_math_backward_substitution(double **A, double *x, double *b, int rows)
{
    int i, j;

    for (i = rows - 1; i >= 0; i--) {
	for (j = i + 1; j < rows; j++) {
	    b[i] = b[i] - A[i][j] * x[j];
	}
	x[i] = (b[i]) / A[i][i];
    }

    return;
}

/*!
 * \brief forward substitution
 *
 * \param A double **
 * \param x double *
 * \param b double *
 * \param rows int
 * \return void
 *
 * */
void G_math_forward_substitution(double **A, double *x, double *b, int rows)
{
    int i, j;

    double tmpval = 0.0;

    for (i = 0; i < rows; i++) {
	tmpval = 0;
	for (j = 0; j < i; j++) {
	    tmpval += A[i][j] * x[j];
	}
	x[i] = (b[i] - tmpval) / A[i][i];
    }

    return;
}
