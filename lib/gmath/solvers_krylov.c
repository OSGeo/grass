
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

static G_math_spvector **create_diag_precond_matrix(double **A,
						    G_math_spvector ** Asp,
						    int rows, int prec);
static int solver_pcg(double **A, G_math_spvector ** Asp, double *x,
		      double *b, int rows, int maxit, double err, int prec, int has_band, int bandwidth);
static int solver_cg(double **A, G_math_spvector ** Asp, double *x, double *b,
		     int rows, int maxit, double err, int has_band, int bandwidth);
static int solver_bicgstab(double **A, G_math_spvector ** Asp, double *x,
			   double *b, int rows, int maxit, double err);


/*!
 * \brief The iterative preconditioned conjugate gradients solver for symmetric positive definite matrices
 *
 * This iterative solver works with symmetric positive definite  regular quadratic matrices.
 *
 * This solver solves the linear equation system:
 *  A x = b
 *
 * The parameter <i>maxit</i> specifies the maximum number of iterations. If the maximum is reached, the
 * solver will abort the calculation and writes the current result into the vector x.
 * The parameter <i>err</i> defines the error break criteria for the solver.
 *
 * \param A (double **) -- the matrix
 * \param x (double *) -- the value vector
 * \param b (double *) -- the right hand side
 * \param rows (int)
 * \param maxit (int) -- the maximum number of iterations
 * \param err (double) -- defines the error break criteria
 * \param prec (int) -- the preconditioner which should be used 1,2 or 3
 * \return (int) -- 1 - success, 2 - not finished but success, 0 - matrix singular, -1 - could not solve the les
 * 
 * */
int G_math_solver_pcg(double **A, double *x, double *b, int rows, int maxit,
		      double err, int prec)
{

    return solver_pcg(A, NULL, x, b, rows, maxit, err, prec, 0, 0);
}

/*!
 * \brief The iterative preconditioned conjugate gradients solver for symmetric positive definite band matrices
 *
 * WARNING: The preconditioning of symmetric band matrices is not implemented yet
 *
 * This iterative solver works with symmetric positive definite band matrices.
 *
 * This solver solves the linear equation system:
 *  A x = b
 *
 * The parameter <i>maxit</i> specifies the maximum number of iterations. If the maximum is reached, the
 * solver will abort the calculation and writes the current result into the vector x.
 * The parameter <i>err</i> defines the error break criteria for the solver.
 *
 * \param A (double **) -- the positive definite band matrix
 * \param x (double *) -- the value vector
 * \param b (double *) -- the right hand side
 * \param rows (int)
 * \param bandwidth (int) -- bandwidth of matrix A
 * \param maxit (int) -- the maximum number of iterations
 * \param err (double) -- defines the error break criteria
 * \param prec (int) -- the preconditioner which should be used 1,2 or 3
 * \return (int) -- 1 - success, 2 - not finished but success, 0 - matrix singular, -1 - could not solve the les
 * 
 * */
int G_math_solver_pcg_sband(double **A, double *x, double *b, int rows, int bandwidth, int maxit,
		      double err, int prec)
{
    G_fatal_error("Preconditioning of band matrics is not implemented yet");
    return solver_pcg(A, NULL, x, b, rows, maxit, err, prec, 1, bandwidth);
}


/*!
 * \brief The iterative preconditioned conjugate gradients solver for sparse symmetric positive definite matrices
 *
 * This iterative solver works with symmetric positive definite sparse matrices.
 *
 * This solver solves the linear equation system:
 *  A x = b
 *
 * The parameter <i>maxit</i> specifies the maximum number of iterations. If the maximum is reached, the
 * solver will abort the calculation and writes the current result into the vector x.
 * The parameter <i>err</i> defines the error break criteria for the solver.
 *
 * \param Asp (G_math_spvector **) -- the sparse matrix
 * \param x (double *) -- the value vector
 * \param b (double *) -- the right hand side
 * \param rows (int)
 * \param maxit (int) -- the maximum number of iterations
 * \param err (double) -- defines the error break criteria
 * \param prec (int) -- the preconditioner which should be used 1,2 or 3
 * \return (int) -- 1 - success, 2 - not finished but success, 0 - matrix singular, -1 - could not solve the les
 * 
 * */
int G_math_solver_sparse_pcg(G_math_spvector ** Asp, double *x, double *b,
			     int rows, int maxit, double err, int prec)
{

    return solver_pcg(NULL, Asp, x, b, rows, maxit, err, prec, 0, 0);
}

int solver_pcg(double **A, G_math_spvector ** Asp, double *x, double *b,
	       int rows, int maxit, double err, int prec, int has_band, int bandwidth)
{
    double *r, *z;

    double *p;

    double *v;

    double s = 0.0;

    double a0 = 0, a1 = 0, mygamma, tmp = 0;

    int m, i;

    int finished = 2;

    int error_break;

    G_math_spvector **M;

    r = G_alloc_vector(rows);
    p = G_alloc_vector(rows);
    v = G_alloc_vector(rows);
    z = G_alloc_vector(rows);

    error_break = 0;

    /*compute the preconditioning matrix, this is a sparse matrix */
    M = create_diag_precond_matrix(A, Asp, rows, prec);

    /*
     * residual calculation 
     */
#pragma omp parallel
    {
	if (Asp)
	    G_math_Ax_sparse(Asp, x, v, rows);
	else if(has_band)
	    G_math_Ax_sband(A, x, v, rows, bandwidth);
	else
	    G_math_d_Ax(A, x, v, rows, rows);

	G_math_d_ax_by(b, v, r, 1.0, -1.0, rows);
	/*performe the preconditioning */
	G_math_Ax_sparse(M, r, p, rows);

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
	    if (Asp)
		G_math_Ax_sparse(Asp, p, v, rows);
	    else if(has_band)
		G_math_Ax_sband(A, p, v, rows, bandwidth);
	    else
		G_math_d_Ax(A, p, v, rows, rows);



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

	    G_math_d_ax_by(p, x, x, mygamma, 1.0, rows);

	    if (m % 50 == 1) {
		if (Asp)
		    G_math_Ax_sparse(Asp, x, v, rows);
		else if(has_band)
		    G_math_Ax_sband(A, x, v, rows, bandwidth);
		else
		    G_math_d_Ax(A, x, v, rows, rows);

		G_math_d_ax_by(b, v, r, 1.0, -1.0, rows);
	    }
	    else {
		G_math_d_ax_by(r, v, r, 1.0, -1.0 * mygamma, rows);
	    }

	    /*performe the preconditioning */
	    G_math_Ax_sparse(M, r, z, rows);


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
		    G_warning(_
			      ("Unable to solve the linear equation system"));
		    error_break = 1;
		}
	    }
	    G_math_d_ax_by(p, z, p, tmp, 1.0, rows);
	}

	if (Asp != NULL)
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
    G_math_free_spmatrix(M, rows);

    return finished;
}


/*!
 * \brief The iterative conjugate gradients solver for symmetric positive definite matrices
 *
 * This iterative solver works with symmetric positive definite  regular quadratic matrices.
 *
 * This solver solves the linear equation system:
 *  A x = b
 *
 * The parameter <i>maxit</i> specifies the maximum number of iterations. If the maximum is reached, the
 * solver will abort the calculation and writes the current result into the vector x.
 * The parameter <i>err</i> defines the error break criteria for the solver.
 *
 * \param A (double **) -- the matrix
 * \param x (double *) -- the value vector
 * \param b (double *) -- the right hand side
 * \param rows (int)
 * \param maxit (int) -- the maximum number of iterations
 * \param err (double) -- defines the error break criteria
 * \return (int) -- 1 - success, 2 - not finished but success, 0 - matrix singular, -1 - could not solve the les
 * 
 * */
int G_math_solver_cg(double **A, double *x, double *b, int rows, int maxit,
		     double err)
{
    return solver_cg(A, NULL, x, b, rows, maxit, err, 0, 0);
}

/*!
 * \brief The iterative conjugate gradients solver for symmetric positive definite band matrices
 *
 * This iterative solver works with symmetric positive definite band matrices.
 *
 * This solver solves the linear equation system:
 *  A x = b
 *
 * The parameter <i>maxit</i> specifies the maximum number of iterations. If the maximum is reached, the
 * solver will abort the calculation and writes the current result into the vector x.
 * The parameter <i>err</i> defines the error break criteria for the solver.
 *
 * \param A (double **) -- the symmetric positive definite band matrix
 * \param x (double *) -- the value vector
 * \param b (double *) -- the right hand side
 * \param rows (int)
 * \param bandwidth (int) -- the bandwidth of matrix A
 * \param maxit (int) -- the maximum number of iterations
 * \param err (double) -- defines the error break criteria
 * \return (int) -- 1 - success, 2 - not finished but success, 0 - matrix singular, -1 - could not solve the les
 * 
 * */
int G_math_solver_cg_sband(double **A, double *x, double *b, int rows, int bandwidth, int maxit, double err)
{
    return solver_cg(A, NULL, x, b, rows, maxit, err, 1, bandwidth);
}


/*!
 * \brief The iterative conjugate gradients solver for sparse symmetric positive definite matrices
 *
 * This iterative solver works with symmetric positive definite sparse matrices.
 *
 * This solver solves the linear equation system:
 *  A x = b
 *
 * The parameter <i>maxit</i> specifies the maximum number of iterations. If the maximum is reached, the
 * solver will abort the calculation and writes the current result into the vector x.
 * The parameter <i>err</i> defines the error break criteria for the solver.
 *
 * \param Asp (G_math_spvector **) -- the sparse matrix
 * \param x (double *) -- the value vector
 * \param b (double *) -- the right hand side
 * \param rows (int)
 * \param maxit (int) -- the maximum number of iterations
 * \param err (double) -- defines the error break criteria
 * \return (int) -- 1 - success, 2 - not finished but success, 0 - matrix singular, -1 - could not solve the les
 * 
 * */
int G_math_solver_sparse_cg(G_math_spvector ** Asp, double *x, double *b,
			    int rows, int maxit, double err)
{
    return solver_cg(NULL, Asp, x, b, rows, maxit, err, 0, 0);
}


int solver_cg(double **A, G_math_spvector ** Asp, double *x, double *b,
	      int rows, int maxit, double err, int has_band, int bandwidth)
{
    double *r;

    double *p;

    double *v;

    double s = 0.0;

    double a0 = 0, a1 = 0, mygamma, tmp = 0;

    int m, i;

    int finished = 2;

    int error_break;

    r = G_alloc_vector(rows);
    p = G_alloc_vector(rows);
    v = G_alloc_vector(rows);

    error_break = 0;
    /*
     * residual calculation 
     */
#pragma omp parallel
    {
	if (Asp)
	    G_math_Ax_sparse(Asp, x, v, rows);
	else if(has_band)
	    G_math_Ax_sband(A, x, v, rows, bandwidth);
	else
	    G_math_d_Ax(A, x, v, rows, rows);

	G_math_d_ax_by(b, v, r, 1.0, -1.0, rows);
	G_math_d_copy(r, p, rows);

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
	    if (Asp)
		G_math_Ax_sparse(Asp, p, v, rows);
	    else if(has_band)
		G_math_Ax_sband(A, p, v, rows, bandwidth);
	    else
		G_math_d_Ax(A, p, v, rows, rows);

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

	    G_math_d_ax_by(p, x, x, mygamma, 1.0, rows);

	    if (m % 50 == 1) {
		if (Asp)
		    G_math_Ax_sparse(Asp, x, v, rows);
		else if(has_band)
		    G_math_Ax_sband(A, x, v, rows, bandwidth);
		else
		    G_math_d_Ax(A, x, v, rows, rows);

		G_math_d_ax_by(b, v, r, 1.0, -1.0, rows);
	    }
	    else {
		G_math_d_ax_by(r, v, r, 1.0, -1.0 * mygamma, rows);
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
		    G_warning(_
			      ("Unable to solve the linear equation system"));
		    error_break = 1;
		}
	    }
	    G_math_d_ax_by(p, r, p, tmp, 1.0, rows);
	}

	if (Asp != NULL)
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



/*!
 * \brief The iterative biconjugate gradients solver with stabilization for unsymmetric non-definite matrices
 *
 * This iterative solver works with regular quadratic matrices.
 *
 * This solver solves the linear equation system:
 *  A x = b
 *
 * The parameter <i>maxit</i> specifies the maximum number of iterations. If the maximum is reached, the
 * solver will abort the calculation and writes the current result into the vector x.
 * The parameter <i>err</i> defines the error break criteria for the solver.
 *
 * \param A (double **) -- the matrix
 * \param x (double *) -- the value vector
 * \param b (double *) -- the right hand side
 * \param rows (int)
 * \param maxit (int) -- the maximum number of iterations
 * \param err (double) -- defines the error break criteria
 * \return (int) -- 1 - success, 2 - not finished but success, 0 - matrix singular, -1 - could not solve the les
 * 
 * */
int G_math_solver_bicgstab(double **A, double *x, double *b, int rows,
			   int maxit, double err)
{
    return solver_bicgstab(A, NULL, x, b, rows, maxit, err);
}

/*!
 * \brief The iterative biconjugate gradients solver with stabilization for unsymmetric non-definite matrices
 *
 * This iterative solver works with sparse matrices.
 *
 * This solver solves the linear equation system:
 *  A x = b
 *
 * The parameter <i>maxit</i> specifies the maximum number of iterations. If the maximum is reached, the
 * solver will abort the calculation and writes the current result into the vector x.
 * The parameter <i>err</i> defines the error break criteria for the solver.
 *
 * \param Asp (G_math_spvector **) -- the sparse matrix
 * \param x (double *) -- the value vector
 * \param b (double *) -- the right hand side
 * \param rows (int)
 * \param maxit (int) -- the maximum number of iterations
 * \param err (double) -- defines the error break criteria
 * \return (int) -- 1 - success, 2 - not finished but success, 0 - matrix singular, -1 - could not solve the les
 * 
 * */
int G_math_solver_sparse_bicgstab(G_math_spvector ** Asp, double *x,
				  double *b, int rows, int maxit, double err)
{
    return solver_bicgstab(NULL, Asp, x, b, rows, maxit, err);
}


int solver_bicgstab(double **A, G_math_spvector ** Asp, double *x, double *b,
		    int rows, int maxit, double err)
{
    double *r;

    double *r0;

    double *p;

    double *v;

    double *s;

    double *t;

    double s1 = 0.0, s2 = 0.0, s3 = 0.0;

    double alpha = 0, beta = 0, omega, rr0 = 0, error;

    int m, i;

    int finished = 2;

    int error_break;

    r = G_alloc_vector(rows);
    r0 = G_alloc_vector(rows);
    p = G_alloc_vector(rows);
    v = G_alloc_vector(rows);
    s = G_alloc_vector(rows);
    t = G_alloc_vector(rows);

    error_break = 0;

#pragma omp parallel
    {
	if (Asp)
	    G_math_Ax_sparse(Asp, x, v, rows);
	else
	    G_math_d_Ax(A, x, v, rows, rows);

	G_math_d_ax_by(b, v, r, 1.0, -1.0, rows);
	G_math_d_copy(r, r0, rows);
	G_math_d_copy(r, p, rows);
    }

    s1 = s2 = s3 = 0.0;

    /* ******************* */
    /* start the iteration */
    /* ******************* */
    for (m = 0; m < maxit; m++) {

#pragma omp parallel default(shared)
	{
	    if (Asp)
		G_math_Ax_sparse(Asp, p, v, rows);
	    else
		G_math_d_Ax(A, p, v, rows, rows);

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
		    G_warning(_
			      ("Unable to solve the linear equation system"));
		    error_break = 1;
		}

		rr0 = s2;
		alpha = rr0 / s3;
		s1 = s2 = s3 = 0.0;
	    }

	    G_math_d_ax_by(r, v, s, 1.0, -1.0 * alpha, rows);
	    if (Asp)
		G_math_Ax_sparse(Asp, s, t, rows);
	    else
		G_math_d_Ax(A, s, t, rows, rows);

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

	    G_math_d_ax_by(p, s, r, alpha, omega, rows);
	    G_math_d_ax_by(x, r, x, 1.0, 1.0, rows);
	    G_math_d_ax_by(s, t, r, 1.0, -1.0 * omega, rows);

#pragma omp for schedule (static) private(i) reduction(+:s1)
	    for (i = 0; i < rows; i++) {
		s1 += r[i] * r0[i];
	    }

#pragma omp single
	    {
		beta = alpha / omega * s1 / rr0;
		s1 = s2 = s3 = 0.0;
	    }

	    G_math_d_ax_by(p, v, p, 1.0, -1.0 * omega, rows);
	    G_math_d_ax_by(p, r, p, beta, 1.0, rows);
	}


	if (Asp != NULL)
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
 * \brief Compute a diagonal preconditioning matrix for krylov space solver
 *
 * \param A (double **) -- the matrix for which the precondition should be computed (if the sparse matrix is used, set it to NULL)
 * \param Asp (G_math_spvector **) -- the matrix for which the precondition should be computed 
 * \param rows (int)
 * \param prec (int) -- which preconditioner should be used 1, 2 or 3
 *
 * */
G_math_spvector **create_diag_precond_matrix(double **A,
					     G_math_spvector ** Asp, int rows,
					     int prec)
{
    G_math_spvector **Msp;

    int i, j, cols = rows;

    double sum;

    Msp = G_math_alloc_spmatrix(rows);

    if (A != NULL) {
#pragma omp parallel for schedule (static) private(i, j, sum) shared(A, Msp, rows, cols, prec)
	for (i = 0; i < rows; i++) {
	    G_math_spvector *spvect = G_math_alloc_spvector(1);

	    switch (prec) {
	    case G_MATH_ROWSCALE_EUKLIDNORM_PRECONDITION:
		sum = 0;
		for (j = 0; j < cols; j++)
		    sum += A[i][j] * A[i][j];
		spvect->values[0] = 1.0 / sqrt(sum);
		break;
	    case G_MATH_ROWSCALE_ABSSUMNORM_PRECONDITION:
		sum = 0;
		for (j = 0; j < cols; j++)
		    sum += fabs(A[i][j]);
		spvect->values[0] = 1.0 / (sum);
		break;
	    case G_MATH_DIAGONAL_PRECONDITION:
	    default:
		spvect->values[0] = 1.0 / A[i][i];
		break;
	    }


	    spvect->index[0] = i;
	    spvect->cols = 1;;
	    G_math_add_spvector(Msp, spvect, i);

	}
    }
    else {
#pragma omp parallel for schedule (static) private(i, j, sum) shared(Asp, Msp, rows, cols, prec)
	for (i = 0; i < rows; i++) {
	    G_math_spvector *spvect = G_math_alloc_spvector(1);

	    switch (prec) {
	    case G_MATH_ROWSCALE_EUKLIDNORM_PRECONDITION:
		sum = 0;
		for (j = 0; j < Asp[i]->cols; j++)
		    sum += Asp[i]->values[j] * Asp[i]->values[j];
		spvect->values[0] = 1.0 / sqrt(sum);
		break;
	    case G_MATH_ROWSCALE_ABSSUMNORM_PRECONDITION:
		sum = 0;
		for (j = 0; j < Asp[i]->cols; j++)
		    sum += fabs(Asp[i]->values[j]);
		spvect->values[0] = 1.0 / (sum);
		break;
	    case G_MATH_DIAGONAL_PRECONDITION:
	    default:
		for (j = 0; j < Asp[i]->cols; j++)
		    if (i == Asp[i]->index[j])
			spvect->values[0] = 1.0 / Asp[i]->values[j];
		break;
	    }

	    spvect->index[0] = i;
	    spvect->cols = 1;;
	    G_math_add_spvector(Msp, spvect, i);
	}
    }
    return Msp;
}
