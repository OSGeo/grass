
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

static int sparse_jacobi_gauss(N_les * L, int maxit, double sor, double error,
			       const char *type);
static int jacobi(double **M, double *b, double *x, int rows, int maxit,
		  double sor, double error);
static int gauss_seidel(double **M, double *b, double *x, int rows, int maxit,
			double sor, double error);

/* ******************************************************* *
 * ******** overrelaxed jacobian ************************* *
 * ******************************************************* */
/*!
 * \brief The iterative jacobian solver for regular matrices
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
 * \param sor double -- defines the successive overrelaxion parameter [0:1]
 * \param error double -- defines the error break criteria
 * \return int -- 1=success, -1=could not solve the les
 * 
 * */
int N_solver_jacobi(N_les * L, int maxit, double sor, double error)
{

    if (L->quad != 1) {
	G_warning(_("The linear equation system is not quadratic"));
	return -1;
    }

    if (L->type == N_NORMAL_LES)
	return jacobi(L->A, L->b, L->x, L->rows, maxit, sor, error);
    else
	return sparse_jacobi_gauss(L, maxit, sor, error,
				   N_SOLVER_ITERATIVE_JACOBI);
}


/* ******************************************************* *
 * ********* overrelaxed gauss seidel ******************** *
 * ******************************************************* */
/*!
 * \brief The iterative overrelaxed gauss seidel solver for regular matrices
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
 * \param sor double -- defines the successive overrelaxion parameter [0:1]
 * \param error double -- defines the error break criteria
 * \return int -- 1=success, -1=could not solve the les
 * 
 * */

int N_solver_SOR(N_les * L, int maxit, double sor, double error)
{

    if (L->quad != 1) {
	G_warning(_("The linear equation system is not quadratic"));
	return -1;
    }

    if (L->type == N_NORMAL_LES)
	return gauss_seidel(L->A, L->b, L->x, L->rows, maxit, sor, error);
    else
	return sparse_jacobi_gauss(L, maxit, sor, error,
				   N_SOLVER_ITERATIVE_SOR);
}

/* ******************************************************* *
 * ****** sparse jacobi and SOR algorithm **************** *
 * ******************************************************* */
int
sparse_jacobi_gauss(N_les * L, int maxit, double sor, double error,
		    const char *type)
{
    int i, j, k, rows, finished = 0;
    double *Enew, *x, *b;
    double E, err = 0;

    x = L->x;
    b = L->b;
    rows = L->rows;

    Enew = vectmem(rows);

    for (k = 0; k < maxit; k++) {
	err = 0;
	{
	    if (k == 0) {
		for (j = 0; j < rows; j++) {
		    Enew[j] = x[j];
		}
	    }
	    for (i = 0; i < rows; i++) {
		E = 0;
		if (strcmp(type, N_SOLVER_ITERATIVE_JACOBI) == 0) {
		    for (j = 0; j < L->Asp[i]->cols; j++) {
			E += L->Asp[i]->values[j] * x[L->Asp[i]->index[j]];
		    }
		}
		else {
		    for (j = 0; j < L->Asp[i]->cols; j++) {
			E += L->Asp[i]->values[j] * Enew[L->Asp[i]->index[j]];
		    }
		}
		Enew[i] = x[i] - sor * (E - b[i]) / L->Asp[i]->values[0];
	    }
	    for (j = 0; j < rows; j++) {
		err += (x[j] - Enew[j]) * (x[j] - Enew[j]);

		x[j] = Enew[j];
	    }
	}

	if (strcmp(type, N_SOLVER_ITERATIVE_JACOBI) == 0)
	    G_message(_("sparse Jacobi -- iteration %5i error %g\n"), k, err);
	else if (strcmp(type, N_SOLVER_ITERATIVE_SOR) == 0)
	    G_message(_("sparse SOR -- iteration %5i error %g\n"), k, err);

	if (err < error) {
	    finished = 1;
	    break;
	}
    }

    G_free(Enew);

    return finished;
}

/* ******************************************************* *
 * ******* direct jacobi ********************************* *
 * ******************************************************* */
int jacobi(double **M, double *b, double *x, int rows, int maxit, double sor,
	   double error)
{
    int i, j, k;
    double *Enew;
    double E, err = 0;

    Enew = vectmem(rows);

    for (j = 0; j < rows; j++) {
	Enew[j] = x[j];
    }

    for (k = 0; k < maxit; k++) {
	for (i = 0; i < rows; i++) {
	    E = 0;
	    for (j = 0; j < rows; j++) {
		E += M[i][j] * x[j];
	    }
	    Enew[i] = x[i] - sor * (E - b[i]) / M[i][i];
	}
	err = 0;
	for (j = 0; j < rows; j++) {
	    err += (x[j] - Enew[j]) * (x[j] - Enew[j]);

	    x[j] = Enew[j];
	}
	G_message(_("Jacobi -- iteration %5i error %g\n"), k, err);
	if (err < error)
	    break;
    }

    return 1;
}

/* ******************************************************* *
 * ******* direct gauss seidel *************************** *
 * ******************************************************* */
int gauss_seidel(double **M, double *b, double *x, int rows, int maxit,
		 double sor, double error)
{
    int i, j, k;
    double *Enew;
    double E, err = 0;

    Enew = vectmem(rows);

    for (j = 0; j < rows; j++) {
	Enew[j] = x[j];
    }

    for (k = 0; k < maxit; k++) {
	for (i = 0; i < rows; i++) {
	    E = 0;
	    for (j = 0; j < rows; j++) {
		E += M[i][j] * Enew[j];
	    }
	    Enew[i] = x[i] - sor * (E - b[i]) / M[i][i];
	}
	err = 0;
	for (j = 0; j < rows; j++) {
	    err += (x[j] - Enew[j]) * (x[j] - Enew[j]);

	    x[j] = Enew[j];
	}
	G_message(_("SOR -- iteration %5i error %g\n"), k, err);
	if (err < error)
	    break;
    }

    return 1;
}
