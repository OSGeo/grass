
/*****************************************************************************
 *
 * MODULE:       Grass PDE Numerical Library
 * AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
 * 		soerengebbert <at> gmx <dot> de
 *               
 * PURPOSE:      direkt linear equation system solvers
 * 		part of the gpde library
 *               
 * COPYRIGHT:    (C) 2007 by the GRASS Development Team
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
#include "grass/gis.h"
#include "grass/glocale.h"
#include "grass/gmath.h"

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
 * \int rows int
 * \return int -- 1 success
 * */
int G_math_solver_gauss(double **A, double *x, double *b, int rows)
{
    G_message(_("Starting direct gauss elimination solver"));

    G_math_gauss_elimination(A, b, rows);
    G_math_backward_solving(A, x, b, rows);

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
 * \int rows int
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
	    G_math_forward_solving(A, b, b, rows);
	}

#pragma omp for  schedule (static) private(i)
	for (i = 0; i < rows; i++) {
	    A[i][i] = tmpv[i];
	}

#pragma omp single
	{
	    G_math_backward_solving(A, x, b, rows);
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
 * \int rows int
 * \return int -- 1 success
 * */
int G_math_solver_cholesky(double **A, double *x, double *b, int bandwith,
			   int rows)
{

    G_message(_("Starting cholesky decomposition solver"));

    if (G_math_cholesky_decomposition(A, rows, bandwith) != 1) {
	G_warning(_("Unable to solve the linear equation system"));
	return -2;
    }

    G_math_forward_solving(A, b, b, rows);
    G_math_backward_solving(A, x, b, rows);

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

    /*compute the pivot -- commented out, because its meaningless
       to compute it only nth times. */
    /*G_math_pivot_create(A, b, rows, 0); */

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

    /*compute the pivot -- commented out, because its meaningless
       to compute it only nth times. */
    /*G_math_pivot_create(A, b, rows, 0); */

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
 *        with bandwith optimization
 *
 * The provided matrix will be overwritten with the lower and 
 * upper triangle matrix A = LL^T 
 *
 * \param A double **
 * \param rows int
 * \param bandwith int -- the bandwith of the matrix (0 > bandwith <= cols)
 * \return void
 *
 * */
int G_math_cholesky_decomposition(double **A, int rows, int bandwith)
{

    int i = 0, j = 0, k = 0;

    double sum_1 = 0.0;

    double sum_2 = 0.0;

    int colsize;

    if (bandwith <= 0)
	bandwith = rows;

    colsize = bandwith;

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

	if ((k + bandwith) > rows) {
	    colsize = rows;
	}
	else {
	    colsize = k + bandwith;
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
    /*we need to copy the lower triangle matrix to the upper trianle */
#pragma omp parallel for schedule (static) private(i, k) shared(A, rows)
    for (k = 0; k < rows; k++) {
	for (i = k + 1; i < rows; i++) {
	    A[k][i] = A[i][k];
	}
    }


    return 1;
}

/*!
 * \brief backward solving
 *
 * \param A double **
 * \param x double *
 * \param b double *
 * \param rows int
 * \return void
 *
 * */
void G_math_backward_solving(double **A, double *x, double *b, int rows)
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
 * \brief forward solving
 *
 * \param A double **
 * \param x double *
 * \param b double *
 * \param rows int
 * \return void
 *
 * */
void G_math_forward_solving(double **A, double *x, double *b, int rows)
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


/*!
 * \brief Optimize the structure of the linear equation system with a common pivoting strategy
 *
 * Create a optimized linear equation system for
 * direct solvers: gauss and lu decomposition.
 *
 * The rows are permuted based on the pivot elements.
 *
 * This algorithm will modify the provided linear equation system
 * and should only be used with the gauss elimination and lu decomposition solver.
 *
 * \param A double ** - a quadratic matrix
 * \param b double *  - the right hand  vector, if not available set it to NULL
 * \param rows int 
 * \param start int -- the row
 * \return int - the number of swapped rows
 *
 *
 * */
int G_math_pivot_create(double **A, double *b, int rows, int start)
{
    int num = 0;		/*number of changed rows */

    int i, j, k;

    double max;

    int number = 0;

    double tmpval = 0.0, s = 0.0;

    double *link = NULL;

    link = G_alloc_vector(rows);

    G_debug(2, "G_math_pivot_create: swap rows if needed");
    for (i = start; i < rows; i++) {
	s = 0.0;
	for (k = i + 1; k < rows; k++) {
	    s += fabs(A[i][k]);
	}
	max = fabs(A[i][i]) / s;
	number = i;
	for (j = i + 1; j < rows; j++) {
	    s = 0.0;
	    for (k = j; k < rows; k++) {
		s += fabs(A[j][k]);
	    }
	    /*search for the pivot element */
	    if (max < fabs(A[j][i]) / s) {
		max = fabs(A[j][i] / s);
		number = j;
	    }
	}
	if (max == 0) {
	    max = TINY;
	    G_warning("Matrix is singular");
	}
	/*if an pivot element was found, swap the les entries */
	if (number != i) {

	    G_debug(4, "swap row %i with row %i", i, number);

	    if (b != NULL) {
		tmpval = b[number];
		b[number] = b[i];
		b[i] = tmpval;
	    }
	    G_math_d_copy(A[number], link, rows);
	    G_math_d_copy(A[i], A[number], rows);
	    G_math_d_copy(link, A[i], rows);
	    num++;
	}
    }

    G_free_vector(link);

    return num;
}
