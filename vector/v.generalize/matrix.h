
/****************************************************************
 *
 * MODULE:     v.generalize
 *
 * AUTHOR(S):  Daniel Bundala
 *
 * PURPOSE:    definition of a matrix and basic operations with
 *             with matrices
 *
 * COPYRIGHT:  (C) 2002-2005 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 ****************************************************************/

#ifndef MATRIX_H
#define MATRIX_H

#include <grass/vector.h>

typedef struct
{
    int rows, cols;
    double **a;
} MATRIX;

/* return 1 on success, 0 on error (out of memory) */
extern int matrix_init(int rows, int cols, MATRIX *res);

/* free the memory occupied by the values of m */
extern void matrix_free(MATRIX *m);

/* multiply two matrices, Return 1 on success, 0 on failure.
 * return value 0 means - bad dimensions  */
extern int matrix_mult(MATRIX *a, MATRIX *b, MATRIX *res);

/* adds a multiple of the identity matrix to the given matrix
 * M = M + s * Id. Returns 1 on success, 0 otherwise */
extern int matrix_add_identity(double s, MATRIX *m);

/* calculate the inverse of given (square) matrix. Returns 0 if
 * the matrix is not invertible or if an error occurs.
 * percents indicates whether we want to show the progress of
 * computation
 * Otherwise it returns 1 */
extern int matrix_inverse(MATRIX *a, MATRIX *res, int percents);

/* multiplies matrix by a scalar */
extern void matrix_mult_scalar(double s, MATRIX *m);

/* res = a + b. Does not cheack the dimensions */
extern void matrix_add(MATRIX *a, MATRIX *b, MATRIX *res);

/* debug function */
extern void matrix_print(MATRIX *a);
#endif
