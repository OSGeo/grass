/**
 * \file la.h
 *
 * \brief Wrapper headers for BLAS/LAPACK.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author David D. Gray, ddgray@armacde.demon co uk
 * \author GRASS Development Team
 *
 * \date 2000-2007
 */

#ifndef HAVE_LIBBLAS
#error GRASS is not configured with BLAS (la.h cannot be included)
#endif

#ifndef HAVE_LIBLAPACK
#error GRASS is not configured with LAPACK (la.h cannot be included)
#endif

#ifndef GRASS_LA_H
#define GRASS_LA_H

#include <grass/config.h>
#include <stdio.h>

/* Useful defines */

#define MAX_POS    1  /* Indicates maximum value         */
#define MAX_NEG    -1 /* Indicates minimum value         */
#define MAX_ABS    0  /* Indicates absolute value        */

#define DO_COMPACT 0 /* Eliminate unnecessary rows (cols) in matrix  */
#define NO_COMPACT 1 /* ... or not                                   */

/* Operations should know type of coefficient matrix, so that
   they can call the right driver
 */

typedef enum { NONSYM, SYM, HERMITIAN } mat_type;
typedef enum { MATRIX_, ROWVEC_, COLVEC_ } mat_spec;
typedef enum { RVEC, CVEC } vtype;

/************************************************************
 *                                                          *
 * A general matrix wrapper for use with BLAS / LAPACK      *
 *  routines, and perhaps elsewhere                         *
 *                                                          *
 ************************************************************/

typedef struct matrix_ {

    mat_spec type;  /* matrix, row vector or column vector? */
    int v_indx;     /* If a vector, which row(column) is active?
                     * If a matrix this is ignored. If value is < 0,
                     * the first row(column) is assumed, ie. index 0.  */
    int rows, cols; /* Rows and columns of matrix */
    int ldim;       /* Lead dimension of matrix. How many `rows' are
                     * alloc'ed? May exceed real number of rows `rows' */
    double *vals;   /* The values (should be dimensioned to lda * cols */
    int is_init;    /* Is  matrix initialised: values array
                     * is allocated and parameters set ?               */
} mat_struct;

typedef mat_struct vec_struct;

#include <grass/defs/la.h>

#endif /* GRASS_LA_H */
