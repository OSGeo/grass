/*****************************************************************************
 *
 * MODULE:       Grass numerical math interface
 * AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
 *                 soerengebbert <at> googlemail <dot> com
 *
 * PURPOSE:      linear equation system solvers
 *                 part of the gmath library
 *
 * COPYRIGHT:    (C) 2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/gmath.h>

/*!
 * \brief The iterative jacobi solver for sparse matrices
 *
 * The Jacobi solver solves the linear equation system Ax = b
 * The result is written to the vector x.
 *
 * The parameter <i>maxit</i> specifies the maximum number of iterations. If the
 * maximum is reached, the solver will abort the calculation and writes the
 * current result into the vector x. The parameter <i>err</i> defines the error
 * break criteria for the solver.
 *
 * \param Asp G_math_spvector ** -- the sparse matrix
 * \param x double * -- the vector of unknowns
 * \param b double * -- the right side vector
 * \param rows int -- number of rows
 * \param maxit int -- the maximum number of iterations
 * \param sor double -- defines the successive overrelaxion parameter [0:1]
 * \param error double -- defines the error break criteria
 * \return int -- 1=success, -1=could not solve the les
 *
 * */
int G_math_solver_sparse_jacobi(G_math_spvector **Asp, double *x, double *b,
                                int rows, int maxit, double sor, double error)
{
    unsigned int i, j, center, finished = 0;

    int k;

    double *Enew;

    double E, err = 0;

    assert(rows >= 0);

    Enew = G_alloc_vector(rows);

    for (k = 0; k < maxit; k++) {
        err = 0;
        {
            if (k == 0) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                for (j = 0; j < (unsigned int)rows; j++) {
                    Enew[j] = x[j];
                }
            }
            for (i = 0; i < (unsigned int)rows; i++) {
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
                for (j = 0; j < rows; j++) {
                    Enew[j] = x[j];
                }
            }
            for (i = 0; i < rows; i++) {
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
                for (j = 0; j < (unsigned int)rows; j++) {
                    Enew[j] = x[j];
                }
            }
            for (i = 0; i < (unsigned int)rows; i++) {
<<<<<<< HEAD
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
                E = 0;
                center = 0;
                for (j = 0; j < Asp[i]->cols; j++) {
                    E += Asp[i]->values[j] * x[Asp[i]->index[j]];
                    if (Asp[i]->index[j] == i)
                        center = j;
                }
                Enew[i] = x[i] - sor * (E - b[i]) / Asp[i]->values[center];
            }
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (j = 0; j < (unsigned int)rows; j++) {
=======
            for (j = 0; j < rows; j++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            for (j = 0; j < rows; j++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            for (j = 0; j < (unsigned int)rows; j++) {
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
            for (j = 0; j < rows; j++) {
=======
            for (j = 0; j < (unsigned int)rows; j++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
                err += (x[j] - Enew[j]) * (x[j] - Enew[j]);

                x[j] = Enew[j];
            }
        }

        G_message(_("sparse Jacobi -- iteration %5i error %g\n"), k, err);

        if (err < error) {
            finished = 1;
            break;
        }
    }

    G_free(Enew);

    return finished;
}

/*!
 * \brief The iterative gauss seidel solver for sparse matrices
 *
 * The Jacobi solver solves the linear equation system Ax = b
 * The result is written to the vector x.
 *
 * The parameter <i>maxit</i> specifies the maximum number of iterations. If the
 * maximum is reached, the solver will abort the calculation and writes the
 * current result into the vector x. The parameter <i>err</i> defines the error
 * break criteria for the solver.
 *
 * \param Asp G_math_spvector ** -- the sparse matrix
 * \param x double * -- the vector of unknowns
 * \param b double * -- the right side vector
 * \param rows int -- number of rows
 * \param maxit int -- the maximum number of iterations
 * \param sor double -- defines the successive overrelaxion parameter [0:2]
 * \param error double -- defines the error break criteria
 * \return int -- 1=success, -1=could not solve the les
 *
 * */
int G_math_solver_sparse_gs(G_math_spvector **Asp, double *x, double *b,
                            int rows, int maxit, double sor, double error)
{
    unsigned int i, j, finished = 0;

    int k;

    double *Enew;

    double E, err = 0;

    int center;

    assert(rows >= 0);

    Enew = G_alloc_vector(rows);

    for (k = 0; k < maxit; k++) {
        err = 0;
        {
            if (k == 0) {
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                for (j = 0; j < (unsigned int)rows; j++) {
                    Enew[j] = x[j];
                }
            }
            for (i = 0; i < (unsigned int)rows; i++) {
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
                for (j = 0; j < rows; j++) {
                    Enew[j] = x[j];
                }
            }
            for (i = 0; i < rows; i++) {
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
                for (j = 0; j < (unsigned int)rows; j++) {
                    Enew[j] = x[j];
                }
            }
            for (i = 0; i < (unsigned int)rows; i++) {
<<<<<<< HEAD
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
                E = 0;
                center = 0;
                for (j = 0; j < Asp[i]->cols; j++) {
                    E += Asp[i]->values[j] * Enew[Asp[i]->index[j]];
                    if (Asp[i]->index[j] == i)
                        center = j;
                }
                Enew[i] = x[i] - sor * (E - b[i]) / Asp[i]->values[center];
            }
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
            for (j = 0; j < (unsigned int)rows; j++) {
=======
            for (j = 0; j < rows; j++) {
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            for (j = 0; j < rows; j++) {
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
            for (j = 0; j < (unsigned int)rows; j++) {
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
=======
            for (j = 0; j < rows; j++) {
=======
            for (j = 0; j < (unsigned int)rows; j++) {
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
                err += (x[j] - Enew[j]) * (x[j] - Enew[j]);

                x[j] = Enew[j];
            }
        }

        G_message(_("sparse SOR -- iteration %5i error %g\n"), k, err);

        if (err < error) {
            finished = 1;
            break;
        }
    }

    G_free(Enew);

    return finished;
}

/*!
 * \brief The iterative jacobi solver for quadratic matrices
 *
 * The Jacobi solver solves the linear equation system Ax = b
 * The result is written to the vector x.
 *
 * The parameter <i>maxit</i> specifies the maximum number of iterations. If the
 * maximum is reached, the solver will abort the calculation and writes the
 * current result into the vector x. The parameter <i>err</i> defines the error
 * break criteria for the solver.
 *
 * \param A double ** -- the dense matrix
 * \param x double * -- the vector of unknowns
 * \param b double * -- the right side vector
 * \param rows int -- number of rows
 * \param maxit int -- the maximum number of iterations
 * \param sor double -- defines the successive overrelaxion parameter [0:1]
 * \param error double -- defines the error break criteria
 * \return int -- 1=success, -1=could not solve the les
 *
 * */
int G_math_solver_jacobi(double **A, double *x, double *b, int rows, int maxit,
                         double sor, double error)
{
    int i, j, k;

    double *Enew;

    double E, err = 0;

    Enew = G_alloc_vector(rows);

    for (j = 0; j < rows; j++) {
        Enew[j] = x[j];
    }

    for (k = 0; k < maxit; k++) {
        for (i = 0; i < rows; i++) {
            E = 0;
            for (j = 0; j < rows; j++) {
                E += A[i][j] * x[j];
            }
            Enew[i] = x[i] - sor * (E - b[i]) / A[i][i];
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

/*!
 * \brief The iterative gauss seidel solver for quadratic matrices
 *
 * The Jacobi solver solves the linear equation system Ax = b
 * The result is written to the vector x.
 *
 * The parameter <i>maxit</i> specifies the maximum number of iterations. If the
 * maximum is reached, the solver will abort the calculation and writes the
 * current result into the vector x. The parameter <i>err</i> defines the error
 * break criteria for the solver.
 *
 * \param A double ** -- the dense matrix
 * \param x double * -- the vector of unknowns
 * \param b double * -- the right side vector
 * \param rows int -- number of rows
 * \param maxit int -- the maximum number of iterations
 * \param sor double -- defines the successive overrelaxion parameter [0:2]
 * \param error double -- defines the error break criteria
 * \return int -- 1=success, -1=could not solve the les
 *
 * */
int G_math_solver_gs(double **A, double *x, double *b, int rows, int maxit,
                     double sor, double error)
{
    int i, j, k;

    double *Enew;

    double E, err = 0;

    Enew = G_alloc_vector(rows);

    for (j = 0; j < rows; j++) {
        Enew[j] = x[j];
    }

    for (k = 0; k < maxit; k++) {
        for (i = 0; i < rows; i++) {
            E = 0;
            for (j = 0; j < rows; j++) {
                E += A[i][j] * Enew[j];
            }
            Enew[i] = x[i] - sor * (E - b[i]) / A[i][i];
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
