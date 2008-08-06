
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      direkt linear equation system solvers
* 		part of the gpde library
*               
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
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
#include "grass/N_pde.h"
#include "solvers_local_proto.h"

/*prototypes */
static void gauss_elimination(double **A, double *b, int rows);
static void lu_decomposition(double **A, int rows);
static int cholesky_decomposition(double **A, int rows);
static void backward_solving(double **A, double *x, double *b, int rows);
static void forward_solving(double **A, double *x, double *b, int rows);

/***********************************************************
 * GAUSS elimination solver for Ax = b *********************
 * ********************************************************/
/*!
 * \brief The gauss elimination solver for quardatic matrices
 *
 * This solver does not support sparse matrices
 * The matrix A will be overwritten.
 * The result is written to the vector x in the N_les structure
 *
 * \param les N_les *
 * \return int -- 1 success, 0 solver does not work with sprase matrices, -1 matrix is not quadratic, -2 unable to solve the les
 * */
int N_solver_gauss(N_les * les)
{

    if (les->type != N_NORMAL_LES) {
	G_warning(_("The gauss elimination solver does not work with sparse matrices"));
	return 0;
    }

    if (les->quad != 1) {
	G_fatal_error(_("The linear equation system is not quadratic"));
	return 0;
    }


    G_message(_("Starting direct gauss elimination solver"));

    N_les_pivot_create(les);
    gauss_elimination(les->A, les->b, les->rows);
    backward_solving(les->A, les->x, les->b, les->rows);

    return 1;
}

/***********************************************************
 * LU solver for Ax = b ************************************
 * ********************************************************/
/*!
 * \brief The LU solver for quardatic matrices
 *
 * This solver does not support sparse matrices
 * The matrix A will be overwritten.
 * The result is written to the vector x in the N_les structure
 *
 * \param les N_les *
 * \return int -- 1 success, 0 solver does not work with sprase matrices, -1 matrix is not quadratic, -2 unable to solve the les
 * */
int N_solver_lu(N_les * les)
{
    int i;
    double *c, *tmpv;

    if (les->type != N_NORMAL_LES) {
	G_warning(_("The lu solver does not work with sparse matrices"));
	return 0;
    }

    if (les->quad != 1) {
	G_warning(_("The linear equation system is not quadratic"));
	return -1;
    }


    G_message(_("Starting direct lu decomposition solver"));

    tmpv = vectmem(les->rows);
    c = vectmem(les->rows);

    N_les_pivot_create(les);
    lu_decomposition(les->A, les->rows);

#pragma omp parallel
    {

#pragma omp for  schedule (static) private(i)
	for (i = 0; i < les->rows; i++) {
	    tmpv[i] = les->A[i][i];
	    les->A[i][i] = 1;
	}

#pragma omp single
	{
	    forward_solving(les->A, les->b, les->b, les->rows);
	}

#pragma omp for  schedule (static) private(i)
	for (i = 0; i < les->rows; i++) {
	    les->A[i][i] = tmpv[i];
	}

#pragma omp single
	{
	    backward_solving(les->A, les->x, les->b, les->rows);
	}
    }

    G_free(c);
    G_free(tmpv);


    return 1;
}

/***********************************************************
 * cholesky solver for Ax = b ******************************
 * ********************************************************/
/*!
 * \brief The choleksy decomposition solver for quardatic, symmetric 
 * positiv definite matrices
 *
 * This solver does not support sparse matrices
 * The matrix A will be overwritten.
 * The result is written to the vector x in the N_les structure
 *
 * \param les N_les *
 * \return int -- 1 success, 0 solver does not work with sprase matrices, -1 matrix is not quadratic, -2 unable to solve the les, -3 matrix is not symmetric
 * */
int N_solver_cholesky(N_les * les)
{
    if (les->type != N_NORMAL_LES) {
	G_warning(_("The cholesky solver does not work with sparse matrices"));
	return 0;
    }

    if (les->quad != 1) {
	G_warning(_("The linear equation system is not quadratic"));
	return -1;
    }

    /* check for symmetry */
    if (check_symmetry(les) != 1) {
	G_warning(_("Matrix is not symmetric!"));
	return -3;
    }

    G_message(_("Starting cholesky decomposition solver"));

    if (cholesky_decomposition(les->A, les->rows) != 1) {
	G_warning(_("Unable to solve the linear equation system"));
	return -2;
    }

    forward_solving(les->A, les->b, les->b, les->rows);
    backward_solving(les->A, les->x, les->b, les->rows);

    return 1;
}


/***********************************************************
 * gauss elimination ***************************************
 * ********************************************************/
/*!
 * \brief Gauss elimination
 *
 * The matrix will be overwritten with the decomposite form
 *
 * \param A double **
 * \param b double * 
 * \param rows int
 * \return void
 *
 * */
void gauss_elimination(double **A, double *b, int rows)
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

/***********************************************************
 * lu decomposition ****************************************
 * ********************************************************/
/*!
 * \brief lu decomposition
 *
 * The matrix will be overwritten with the decomposite form
 *
 * \param A double **
 * \param rows int
 * \return void
 *
 * */
void lu_decomposition(double **A, int rows)
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

/***********************************************************
 * cholesky decomposition **********************************
 * ********************************************************/
/*!
 * \brief cholesky decomposition for symmetric, positiv definite matrices
 *
 * The provided matrix will be overwritten with the lower and 
 * upper triangle matrix A = LL^T 
 *
 * \param A double **
 * \param rows int
 * \return void
 *
 * */
int cholesky_decomposition(double **A, int rows)
{

    int i, j, k;
    double sum_1 = 0.0;
    double sum_2 = 0.0;
    int error = 0;


    for (k = 0; k < rows; k++) {
#pragma omp parallel private(i, j, sum_2) shared(A, rows, sum_1)
	{
#pragma omp for schedule (static) private(j) reduction(+:sum_1)
	    for (j = 0; j < k; j++) {
		sum_1 += A[k][j] * A[k][j];
	    }
#pragma omp single
	    {
		if ((A[k][k] - sum_1) < 0) {
		    error++;	/*not allowed to exit the OpenMP region */
		}
		A[k][k] = sqrt(A[k][k] - sum_1);
		sum_1 = 0.0;
	    }
#pragma omp for schedule (static) private(i, j, sum_2)
	    for (i = k + 1; i < rows; i++) {
		sum_2 = 0.0;
		for (j = 0; j < k; j++) {
		    sum_2 += A[i][j] * A[k][j];
		}
		A[i][k] = (A[i][k] - sum_2) / A[k][k];
	    }
	}
    }

    /*we need to copy the lower triangle matrix to the upper trianle */
#pragma omp parallel for schedule (static) private(i, k) shared(A, rows)
    for (k = 0; k < rows; k++) {
	for (i = k + 1; i < rows; i++) {
	    A[k][i] = A[i][k];
	}
    }

    if (error > 0) {
	G_warning("Matrix is not positive definite");
	return -1;
    }

    return 1;
}


/***********************************************************
 * backward solving ****************************************
 * ********************************************************/
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
void backward_solving(double **A, double *x, double *b, int rows)
{
    int i, j;
    double tmpval = 0.0;

    for (i = rows - 1; i >= 0; i--) {
	tmpval = 0;
	for (j = i + 1; j < rows; j++) {
	    /*tmpval += A[i][j] * x[j]; */
	    b[i] = b[i] - A[i][j] * x[j];
	}
	/*x[i] = (b[i] - tmpval) / A[i][i]; */
	x[i] = (b[i]) / A[i][i];
    }

    return;
}


/***********************************************************
 * forward solving *****************************************
 * ********************************************************/
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
void forward_solving(double **A, double *x, double *b, int rows)
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


/* ******************************************************* *
 * ***** solving a tridiagonal eq system ***************** *
 * ******************************************************* */
void thomalg(double **M, double *V, int rows)
{
    double *Vtmp;
    double *g;
    double b;
    int i;

    Vtmp = vectmem(rows);
    g = vectmem(rows);

    for (i = 0; i < rows; i++) {
	if (i == 0) {
	    b = M[i][i];
	    Vtmp[i] = V[i] / b;
	}
	else {
	    b = M[i][i] - M[i][i - 1] * g[i - 1];
	    Vtmp[i] = (V[i] - Vtmp[i - 1] * M[i][i - 1]) / b;
	}
	if (i < rows - 1) {
	    g[i] = M[i][i + 1] / b;
	}
    }

    V[rows - 1] = Vtmp[rows - 1];
    for (i = rows - 2; i >= 0; i--) {
	V[i] = Vtmp[i] - g[i] * V[i + 1];
    }

    G_free(Vtmp);
    G_free(g);
}


/***********************************************************
 * vectmem *************************************************
 * ********************************************************/
/*!
 * \brief Allocate vector memory 
 *
 * \param rows int
 * \return double *
 *
 * */
double *vectmem(int rows)
{
    double *vector;

    vector = (double *)(G_calloc(rows, (sizeof(double))));
    return vector;
}
