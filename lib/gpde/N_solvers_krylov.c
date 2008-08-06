
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      linear equation system solvers
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

/*local protos */
static void scalar_product(double *a, double *b, double *scalar, int rows);
static void sub_vectors(double *source_a, double *source_b, double *result,
			int row);
static void sub_vectors_scalar(double *source_a, double *source_b,
			       double *result, double scalar_b, int rows);
static void add_vectors(double *source_a, double *source_b, double *result,
			int rows);
static void add_vectors_scalar(double *source_a, double *source_b,
			       double *result, double scalar_b, int rows);
static void add_vectors_scalar2(double *source_a, double *source_b,
				double *result, double scalar_a,
				double scalar_b, int rows);
static void scalar_vector_product(double *a, double *result, double scalar,
				  int rows);
static void sync_vectors(double *source, double *target, int rows);


/* ******************************************************* *
 * *** preconditioned conjugate gradients **************** *
 * ******************************************************* */
/*!
 * \brief The iterative preconditioned conjugate gradients solver for symmetric positive definite matrices
 *
 * This iterative solver works with symmetric positive definite sparse matrices and 
 * symmetric positive definite regular quadratic matrices.
 *
 * The parameter <i>maxit</i> specifies the maximum number of iterations. If the maximum is reached, the
 * solver will abort the calculation and writes the current result into the vector L->x.
 * The parameter <i>err</i> defines the error break criteria for the solver.
 *
 * \param L N_les *  -- the linear equatuin system
 * \param maxit int -- the maximum number of iterations
 * \param err double -- defines the error break criteria
 * \param prec int -- the preconditioner which shoudl be used N_DIAGONAL_PRECONDITION, N_ROWSUM_PRECONDITION
 * \return int -- 1 - success, 2 - not finisehd but success, 0 - matrix singular, -1 - could not solve the les
 * 
 * */
int N_solver_pcg(N_les * L, int maxit, double err, int prec)
{
    double *r, *z;
    double *p;
    double *v;
    double *x, *b;
    double s = 0.0;
    double a0 = 0, a1 = 0, mygamma, tmp = 0;
    int m, rows, i;
    int finished = 2;
    int error_break;
    N_les *M;

    if (L->quad != 1) {
	G_warning(_("The linear equation system is not quadratic"));
	return -1;
    }

    /* check for symmetry */
    if (check_symmetry(L) != 1) {
	G_warning(_("Matrix is not symmetric!"));
    }

    x = L->x;
    b = L->b;
    rows = L->rows;

    r = vectmem(rows);
    p = vectmem(rows);
    v = vectmem(rows);
    z = vectmem(rows);

    error_break = 0;

    /*compute the preconditioning matrix */
    M = N_create_diag_precond_matrix(L, prec);

    /*
     * residual calculation 
     */
#pragma omp parallel
    {
	/* matrix vector multiplication */
	if (L->type == N_SPARSE_LES)
	    N_sparse_matrix_vector_product(L, x, v);
	else
	    N_matrix_vector_product(L, x, v);

	sub_vectors(b, v, r, rows);
	/*performe the preconditioning */
	N_sparse_matrix_vector_product(M, r, p);

	/* scalar product */
#pragma omp for schedule (static) private(i) reduction(+:s)
	for (i = 0; i < rows; i++) {
	    s += p[i] * r[i];
	}
    }

    a0 = s;
    s = 0.0;

    /* ******************* */
    /* start the iteration */
    /* ******************* */
    for (m = 0; m < maxit; m++) {
#pragma omp parallel default(shared)
	{
	    /* matrix vector multiplication */
	    if (L->type == N_SPARSE_LES)
		N_sparse_matrix_vector_product(L, p, v);
	    else
		N_matrix_vector_product(L, p, v);


	    /* scalar product */
#pragma omp for schedule (static) private(i) reduction(+:s)
	    for (i = 0; i < rows; i++) {
		s += v[i] * p[i];
	    }

	    /* barrier */
#pragma omp single
	    {
		tmp = s;
		mygamma = a0 / tmp;
		s = 0.0;
	    }

	    add_vectors_scalar(x, p, x, mygamma, rows);

	    if (m % 50 == 1) {
		/* matrix vector multiplication */
		if (L->type == N_SPARSE_LES)
		    N_sparse_matrix_vector_product(L, x, v);
		else
		    N_matrix_vector_product(L, x, v);

		sub_vectors(b, v, r, rows);

	    }
	    else {
		sub_vectors_scalar(r, v, r, mygamma, rows);
	    }

	    /*performe the preconditioning */
	    N_sparse_matrix_vector_product(M, r, z);

	    /* scalar product */
#pragma omp for schedule (static) private(i) reduction(+:s)
	    for (i = 0; i < rows; i++) {
		s += z[i] * r[i];
	    }

	    /* barrier */
#pragma omp single
	    {
		a1 = s;
		tmp = a1 / a0;
		a0 = a1;
		s = 0.0;

		if (a1 < 0 || a1 == 0 || a1 > 0) {
		    ;
		}
		else {
		    G_warning(_("Unable to solve the linear equation system"));
		    error_break = 1;
		}
	    }
	    add_vectors_scalar(z, p, p, tmp, rows);
	}

	if (L->type == N_SPARSE_LES)
	    G_message(_("Sparse PCG -- iteration %i error  %g\n"), m, a0);
	else
	    G_message(_("PCG -- iteration %i error  %g\n"), m, a0);

	if (error_break == 1) {
	    finished = -1;
	    break;
	}


	if (a0 < err) {
	    finished = 1;
	    break;
	}
    }

    G_free(r);
    G_free(p);
    G_free(v);
    G_free(z);

    return finished;
}


/* ******************************************************* *
 * ****************** conjugate gradients **************** *
 * ******************************************************* */
/*!
 * \brief The iterative conjugate gradients solver for symmetric positive definite matrices
 *
 * This iterative solver works with symmetric positive definite sparse matrices and 
 * symmetric positive definite regular quadratic matrices.
 *
 * The parameter <i>maxit</i> specifies the maximum number of iterations. If the maximum is reached, the
 * solver will abort the calculation and writes the current result into the vector L->x.
 * The parameter <i>err</i> defines the error break criteria for the solver.
 *
 * \param L N_les *  -- the linear equatuin system
 * \param maxit int -- the maximum number of iterations
 * \param err double -- defines the error break criteria
 * \return int -- 1 - success, 2 - not finisehd but success, 0 - matrix singular, -1 - could not solve the les
 * 
 * */
int N_solver_cg(N_les * L, int maxit, double err)
{
    double *r;
    double *p;
    double *v;
    double *x, *b;
    double s = 0.0;
    double a0 = 0, a1 = 0, mygamma, tmp = 0;
    int m, rows, i;
    int finished = 2;
    int error_break;

    if (L->quad != 1) {
	G_warning(_("The linear equation system is not quadratic"));
	return -1;
    }

    /* check for symmetry */
    if (check_symmetry(L) != 1) {
	G_warning(_("Matrix is not symmetric!"));
    }

    x = L->x;
    b = L->b;
    rows = L->rows;

    r = vectmem(rows);
    p = vectmem(rows);
    v = vectmem(rows);

    error_break = 0;
    /*
     * residual calculation 
     */
#pragma omp parallel
    {
	/* matrix vector multiplication */
	if (L->type == N_SPARSE_LES)
	    N_sparse_matrix_vector_product(L, x, v);
	else
	    N_matrix_vector_product(L, x, v);

	sub_vectors(b, v, r, rows);
	sync_vectors(r, p, rows);

	/* scalar product */
#pragma omp for schedule (static) private(i) reduction(+:s)
	for (i = 0; i < rows; i++) {
	    s += r[i] * r[i];
	}
    }

    a0 = s;
    s = 0.0;

    /* ******************* */
    /* start the iteration */
    /* ******************* */
    for (m = 0; m < maxit; m++) {
#pragma omp parallel default(shared)
	{
	    /* matrix vector multiplication */
	    if (L->type == N_SPARSE_LES)
		N_sparse_matrix_vector_product(L, p, v);
	    else
		N_matrix_vector_product(L, p, v);


	    /* scalar product */
#pragma omp for schedule (static) private(i) reduction(+:s)
	    for (i = 0; i < rows; i++) {
		s += v[i] * p[i];
	    }

	    /* barrier */
#pragma omp single
	    {
		tmp = s;
		mygamma = a0 / tmp;
		s = 0.0;
	    }

	    add_vectors_scalar(x, p, x, mygamma, rows);

	    if (m % 50 == 1) {
		/* matrix vector multiplication */
		if (L->type == N_SPARSE_LES)
		    N_sparse_matrix_vector_product(L, x, v);
		else
		    N_matrix_vector_product(L, x, v);

		sub_vectors(b, v, r, rows);
	    }
	    else {
		sub_vectors_scalar(r, v, r, mygamma, rows);
	    }

	    /* scalar product */
#pragma omp for schedule (static) private(i) reduction(+:s)
	    for (i = 0; i < rows; i++) {
		s += r[i] * r[i];
	    }

	    /* barrier */
#pragma omp single
	    {
		a1 = s;
		tmp = a1 / a0;
		a0 = a1;
		s = 0.0;

		if (a1 < 0 || a1 == 0 || a1 > 0) {
		    ;
		}
		else {
		    G_warning(_("Unable to solve the linear equation system"));
		    error_break = 1;
		}
	    }
	    add_vectors_scalar(r, p, p, tmp, rows);
	}

	if (L->type == N_SPARSE_LES)
	    G_message(_("Sparse CG -- iteration %i error  %g\n"), m, a0);
	else
	    G_message(_("CG -- iteration %i error  %g\n"), m, a0);

	if (error_break == 1) {
	    finished = -1;
	    break;
	}

	if (a0 < err) {
	    finished = 1;
	    break;
	}
    }

    G_free(r);
    G_free(p);
    G_free(v);

    return finished;
}

/* ******************************************************* *
 * ************ biconjugate gradients ******************** *
 * ******************************************************* */
/*!
 * \brief The iterative biconjugate gradients solver with stabilization for unsymmetric non-definite matrices
 *
 * The result is written to the vector L->x of the les.
 * This iterative solver works with sparse matrices and regular quadratic matrices.
 *
 * The parameter <i>maxit</i> specifies the maximum number of iterations. If the maximum is reached, the
 * solver will abort the calculation and writes the current result into the vector L->x.
 * The parameter <i>err</i> defines the error break criteria for the solver.
 *
 * \param L N_les *  -- the linear equatuin system
 * \param maxit int -- the maximum number of iterations
 * \param err double -- defines the error break criteria
 * \return int -- 1 - success, 2 - not finisehd but success, 0 - matrix singular, -1 - could not solve the les
 * 
 * 
 * */
int N_solver_bicgstab(N_les * L, int maxit, double err)
{
    double *r;
    double *r0;
    double *p;
    double *v;
    double *s;
    double *t;
    double *x, *b;
    double s1 = 0.0, s2 = 0.0, s3 = 0.0;
    double alpha = 0, beta = 0, omega, rr0 = 0, error;
    int m, rows, i;
    int finished = 2;
    int error_break;

    if (L->quad != 1) {
	G_warning(_("The linear equation system is not quadratic"));
	return -1;
    }


    x = L->x;
    b = L->b;
    rows = L->rows;
    r = vectmem(rows);
    r0 = vectmem(rows);
    p = vectmem(rows);
    v = vectmem(rows);
    s = vectmem(rows);
    t = vectmem(rows);

    error_break = 0;

#pragma omp parallel
    {
	if (L->type == N_SPARSE_LES)
	    N_sparse_matrix_vector_product(L, x, v);
	else
	    N_matrix_vector_product(L, x, v);
	sub_vectors(b, v, r, rows);
	sync_vectors(r, r0, rows);
	sync_vectors(r, p, rows);
    }

    s1 = s2 = s3 = 0.0;

    /* ******************* */
    /* start the iteration */
    /* ******************* */
    for (m = 0; m < maxit; m++) {

#pragma omp parallel default(shared)
	{
	    if (L->type == N_SPARSE_LES)
		N_sparse_matrix_vector_product(L, p, v);
	    else
		N_matrix_vector_product(L, p, v);

	    /* scalar product */
#pragma omp for schedule (static) private(i) reduction(+:s1, s2, s3)
	    for (i = 0; i < rows; i++) {
		s1 += r[i] * r[i];
		s2 += r[i] * r0[i];
		s3 += v[i] * r0[i];
	    }

#pragma omp single
	    {
		error = s1;

		if (error < 0 || error == 0 || error > 0) {
		    ;
		}
		else {
		    G_warning(_("Unable to solve the linear equation system"));
		    error_break = 1;
		}

		rr0 = s2;
		alpha = rr0 / s3;
		s1 = s2 = s3 = 0.0;
	    }

	    sub_vectors_scalar(r, v, s, alpha, rows);
	    if (L->type == N_SPARSE_LES)
		N_sparse_matrix_vector_product(L, s, t);
	    else
		N_matrix_vector_product(L, s, t);

	    /* scalar product */
#pragma omp for schedule (static) private(i) reduction(+:s1, s2)
	    for (i = 0; i < rows; i++) {
		s1 += t[i] * s[i];
		s2 += t[i] * t[i];
	    }

#pragma omp single
	    {
		omega = s1 / s2;
		s1 = s2 = 0.0;
	    }

	    add_vectors_scalar2(p, s, r, alpha, omega, rows);
	    add_vectors(x, r, x, rows);
	    sub_vectors_scalar(s, t, r, omega, rows);

#pragma omp for schedule (static) private(i) reduction(+:s1)
	    for (i = 0; i < rows; i++) {
		s1 += r[i] * r0[i];
	    }

#pragma omp single
	    {
		beta = alpha / omega * s1 / rr0;
		s1 = s2 = s3 = 0.0;
	    }

	    sub_vectors_scalar(p, v, p, omega, rows);
	    add_vectors_scalar(r, p, p, beta, rows);
	}


	if (L->type == N_SPARSE_LES)
	    G_message(_("Sparse BiCGStab -- iteration %i error  %g\n"), m,
		      error);
	else
	    G_message(_("BiCGStab -- iteration %i error  %g\n"), m, error);

	if (error_break == 1) {
	    finished = -1;
	    break;
	}

	if (error < err) {
	    finished = 1;
	    break;
	}
    }

    G_free(r);
    G_free(r0);
    G_free(p);
    G_free(v);
    G_free(s);
    G_free(t);

    return finished;
}

/*!
 * \brief Calculates the scalar product of vector a and b 
 *
 * The result is written to variable scalar
 *
 * \param a       double *
 * \param b       double *
 * \param scalar  double *
 * \param rows int
 * \return void
 *
 * */
void scalar_product(double *a, double *b, double *scalar, int rows)
{
    int i;
    double s = 0.0;

#pragma omp parallel for schedule (static) reduction(+:s)
    for (i = 0; i < rows; i++) {
	s += a[i] * b[i];
    }

    *scalar = s;
    return;
}

/*!
 * \brief Calculates the matrix - vector product of matrix L->A and vector x 
 *
 * The result is written to vector named result. This function only works with
 * regular quadratic matrices.
 *
 * \param L N_les *
 * \param x double *
 * \param result double *
 * \return void
 *
 * */
void N_matrix_vector_product(N_les * L, double *x, double *result)
{
    int i, j;
    double tmp;

#pragma omp for schedule (static) private(i, j, tmp)
    for (i = 0; i < L->rows; i++) {
	tmp = 0;
	for (j = 0; j < L->cols; j++) {
	    tmp += L->A[i][j] * x[j];
	}
	result[i] = tmp;
    }
    return;
}

/*!
 * \brief Calculates the matrix - vector product of sparse matrix L->Asp and vector x 
 *
 * The result is written to vector named result. This function only works with
 * sparse matrices matrices.
 *
 * \param L N_les * 
 * \param x double * 
 * \param result double *
 * \return void
 *
 * */
void N_sparse_matrix_vector_product(N_les * L, double *x, double *result)
{
    int i, j;
    double tmp;

#pragma omp for schedule (static) private(i, j, tmp)
    for (i = 0; i < L->rows; i++) {
	tmp = 0;
	for (j = 0; j < L->Asp[i]->cols; j++) {
	    tmp += L->Asp[i]->values[j] * x[L->Asp[i]->index[j]];
	}
	result[i] = tmp;
    }
    return;
}

/*!
 * \brief Multipiles the vector a and b with the scalars scalar_a and scalar_b and adds them
 *
 *
 * The result is written to the vector named result.
 *
 * \param a      double *
 * \param b      double *
 * \param result double *
 * \param scalar_a double
 * \param scalar_b double
 * \param rows int
 * 
 * */
void
add_vectors_scalar2(double *a, double *b, double *result, double scalar_a,
		    double scalar_b, int rows)
{
    int i;

#pragma omp for schedule (static)
    for (i = 0; i < rows; i++) {
	result[i] = scalar_a * a[i] + scalar_b * b[i];
    }

    return;
}

/*!
 * \brief Multipiles the vector b with the scalar scalar_b and adds  a to b
 *
 *
 * The result is written to the vector named result.
 *
 * \param a      double *
 * \param b      double *
 * \param result double *
 * \param scalar_b double
 * \param rows int
 * 
 * */
void
add_vectors_scalar(double *a, double *b, double *result, double scalar_b,
		   int rows)
{
    int i;

#pragma omp for schedule (static)
    for (i = 0; i < rows; i++) {
	result[i] = a[i] + scalar_b * b[i];
    }

    return;
}

/*!
 * \brief Multipiles the vector b with the scalar scalar_b and substracts b from a
 *
 *
 * The result is written to the vector named result.
 *
 * \param a       double *
 * \param b       double *
 * \param result  double *
 * \param scalar_b double
 * \param rows int
 * 
 * */
void
sub_vectors_scalar(double *a, double *b, double *result, double scalar_b,
		   int rows)
{
    int i;

#pragma omp for schedule (static)
    for (i = 0; i < rows; i++) {
	result[i] = a[i] - scalar_b * b[i];
    }

    return;
}

/*!
 * \brief Adds a to b
 *
 *
 * The result is written to the vector named result.
 *
 * \param a       double *
 * \param b       double *
 * \param result  double *
 * \param rows int
 * 
 * */
void add_vectors(double *a, double *b, double *result, int rows)
{
    int i;

#pragma omp for schedule (static)
    for (i = 0; i < rows; i++) {
	result[i] = a[i] + b[i];
    }

    return;
}

/*!
 * \brief Substracts b from a 
 *
 *
 * The result is written to the vector named result.
 *
 * \param a      double *
 * \param b      double *
 * \param result double *
 * \param rows int
 * 
 * */
void sub_vectors(double *a, double *b, double *result, int rows)
{
    int i;

#pragma omp for schedule (static)
    for (i = 0; i < rows; i++) {
	result[i] = a[i] - b[i];
    }

    return;
}

/*!
 * \brief Multipiles the vector a with the scalar named scalar
 *
 *
 * The result is written to the vector named result.
 *
 * \param a      double *
 * \param result double *
 * \param scalar double *
 * \param rows int
 * 
 * */
void scalar_vector_product(double *a, double *result, double scalar, int rows)
{
    int i;

#pragma omp for schedule (static)
    for (i = 0; i < rows; i++) {
	result[i] = scalar * a[i];
    }

    return;
}

/*!
 * \brief Copies the source vector to the target vector
 *
 * \param source double *
 * \param target double *
 * \param rows int
 * 
 * */
void sync_vectors(double *source, double *target, int rows)
{
    int i;

#pragma omp for schedule (static)
    for (i = 0; i < rows; i++) {
	target[i] = source[i];
    }

    return;
}

/* ******************************************************* *
 * ***** Check if matrix is symmetric ******************** *
 * ******************************************************* */
/*!
 * \brief Check if the matrix in les is symmetric
 *
 * \param L N_les* -- the linear equation system
 * \return int -- 1 = symmetric, 0 = unsymmetric
 * 
 * */

int check_symmetry(N_les * L)
{
    int i, j, k;
    double value1 = 0;
    double value2 = 0;
    int index;
    int symm = 0;

    if (L->quad != 1) {
	G_warning(_("The linear equation system is not quadratic"));
	return 0;
    }

    G_debug(2, "check_symmetry: Check if matrix is symmetric");

    if (L->type == N_SPARSE_LES) {
#pragma omp parallel for schedule (static) private(i, j, k, value1, value2, index) reduction(+:symm) shared(L)
	for (j = 0; j < L->rows; j++) {
	    for (i = 1; i < L->Asp[j]->cols; i++) {
		value1 = 0;
		value2 = 0;
		index = 0;

		index = L->Asp[j]->index[i];
		value1 = L->Asp[j]->values[i];

		for (k = 1; k < L->Asp[index]->cols; k++) {
		    if (L->Asp[index]->index[k] == j) {
			value2 = L->Asp[index]->values[k];
			if ((value1 != value2)) {
			    if ((fabs((fabs(value1) - fabs(value2))) <
				 SYMM_TOLERANCE)) {
				G_debug(5,
					"check_symmetry: sparse matrix is unsymmetric, but within tolerance");
			    }
			    else {
				G_warning
				    ("Matrix unsymmetric: Position [%i][%i] : [%i][%i] \nError: %12.18lf != %12.18lf \ndifference = %12.18lf\nStop symmetry calculation.\n",
				     j, index, index, L->Asp[index]->index[k],
				     value1, value2,
				     fabs(fabs(value1) - fabs(value2)));
				symm++;
			    }
			}
		    }
		}
	    }
	}
    }
    else {
#pragma omp parallel for schedule (static) private(i, j, k, value1, value2, index) reduction(+:symm) shared(L)
	for (i = 0; i < L->rows; i++) {
	    for (j = i + 1; j < L->rows; j++) {
		if (L->A[i][j] != L->A[j][i]) {
		    if ((fabs(fabs(L->A[i][j]) - fabs(L->A[j][i])) <
			 SYMM_TOLERANCE)) {
			G_debug(5,
				"check_symmetry: matrix is unsymmetric, but within tolerance");
		    }
		    else {
			G_warning
			    ("Matrix unsymmetric: Position [%i][%i] : [%i][%i] \nError: %12.18lf != %12.18lf\ndifference = %12.18lf\nStop symmetry calculation.\n",
			     i, j, j, i, L->A[i][j], L->A[j][i],
			     fabs(fabs(L->A[i][j]) - fabs(L->A[j][i])));
			symm++;
		    }
		}
	    }
	}
    }

    if (symm > 0)
	return 0;

    return 1;
}


/*!
 * \brief Compute a diagonal preconditioning matrix for krylov space solver
 *
 * \param L N_les* 
 * \pram prec int -- the preconditioner which should be choosen N_DIAGONAL_PRECONDITION, N_ROWSUM_PRECONDITION
 * \return M N_les* -- the preconditioning matrix
 *
 * */
N_les *N_create_diag_precond_matrix(N_les * L, int prec)
{
    N_les *L_new;
    int rows = L->rows;
    int cols = L->cols;
    int i, j;
    double sum;

    L_new = N_alloc_les_A(rows, N_SPARSE_LES);

    if (L->type == N_NORMAL_LES) {
#pragma omp parallel for schedule (static) private(i, sum) shared(L_new, L, rows, prec)
	for (i = 0; i < rows; i++) {
	    N_spvector *spvect = N_alloc_spvector(1);

	    switch (prec) {
	    case N_ROWSCALE_EUKLIDNORM_PRECONDITION:
		sum = 0;
		for (j = 0; j < cols; j++)
		    sum += L->A[i][j] * L->A[i][j];
		spvect->values[0] = 1.0 / sqrt(sum);
		break;
	    case N_ROWSCALE_ABSSUMNORM_PRECONDITION:
		sum = 0;
		for (j = 0; j < cols; j++)
		    sum += fabs(L->A[i][j]);
		spvect->values[0] = 1.0 / (sum);
		break;
	    case N_DIAGONAL_PRECONDITION:
		spvect->values[0] = 1.0 / L->A[i][i];
		break;
	    default:
		spvect->values[0] = 1.0 / L->A[i][i];
	    }


	    spvect->index[0] = i;
	    spvect->cols = 1;;
	    N_add_spvector_to_les(L_new, spvect, i);

	}
    }
    else {
#pragma omp parallel for schedule (static) private(i, sum) shared(L_new, L, rows, prec)
	for (i = 0; i < rows; i++) {
	    N_spvector *spvect = N_alloc_spvector(1);

	    switch (prec) {
	    case N_ROWSCALE_EUKLIDNORM_PRECONDITION:
		sum = 0;
		for (j = 0; j < L->Asp[i]->cols; j++)
		    sum += L->Asp[i]->values[j] * L->Asp[i]->values[j];
		spvect->values[0] = 1.0 / sqrt(sum);
		break;
	    case N_ROWSCALE_ABSSUMNORM_PRECONDITION:
		sum = 0;
		for (j = 0; j < L->Asp[i]->cols; j++)
		    sum += fabs(L->Asp[i]->values[j]);
		spvect->values[0] = 1.0 / (sum);
		break;
	    case N_DIAGONAL_PRECONDITION:
		spvect->values[0] = 1.0 / L->Asp[i]->values[0];
		break;
	    default:
		spvect->values[0] = 1.0 / L->Asp[i]->values[0];
	    }

	    spvect->index[0] = i;
	    spvect->cols = 1;;
	    N_add_spvector_to_les(L_new, spvect, i);
	}
    }
    return L_new;
}
