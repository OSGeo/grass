
/**
 * \file la.h
 *
 * \brief Wrapper headers for BLAS/LAPACK.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author David D. Gray <ddgray AT armacde demon co uk>
 * \author GRASS GIS Development Team
 *
 * \date 2000-2007
 */

#ifndef LA_H_
#define LA_H_

/* QUESTION: On some systems there appears to be no default link
   to this. Do we create a symlink to
   /usr/lib/gcc-lib/<platform>/<vers_num>/include/g2c.h

   or link to any old f2c.h that happens to hanging around?

   A job for autoconf

   [Also a consideration for -lg2c]
 */

#include <grass/config.h>
#include <stdio.h>

#ifdef HAVE_G2C_H
#include <g2c.h>
#else /* for gcc4+ */
typedef int integer;
typedef unsigned int uinteger;
typedef char *address;
typedef short shortint;
typedef float real;
typedef double doublereal;
typedef struct
{
    real r, i;
} complex;
typedef struct
{
    doublereal r, i;
} doublecomplex;
typedef int logical;
typedef short shortlogical;
typedef char logical1;
typedef char integer1;
typedef long longint;
typedef unsigned long ulongint;

/* IO stuff */
typedef int ftnlen;

/* procedure parameter types for -A */
typedef int (*U_fp) ();
typedef shortint(*J_fp) ();
typedef integer(*I_fp) ();
typedef real(*R_fp) ();
typedef doublereal(*D_fp) (), (*E_fp) ();
typedef void (*C_fp) ();
typedef void (*Z_fp) ();
typedef logical(*L_fp) ();
typedef shortlogical(*K_fp) ();
typedef void (*H_fp) ();
typedef int (*S_fp) ();

/* E_fp is for real functions when -R is not specified */
typedef void C_f;		/* complex function                    */
typedef void H_f;		/* character function                  */
typedef void Z_f;		/* double complex function             */
typedef doublereal E_f;		/* real function with -R not specified */
#endif /* HAVE_G2C_H */

/* The following may have to be selectively installed according
   to platform, at least partly
 */

#if defined(HAVE_LIBBLAS) && defined(HAVE_LIBLAPACK)
#include <grass/blas.h>
#include <grass/lapack.h>
#endif


/* Useful defines */

#define MAX_POS          1	/* Indicates maximum value         */
#define MAX_NEG         -1	/* Indicates minimum value         */
#define MAX_ABS          0	/* Indicates absolute value        */

#define DO_COMPACT       0	/* Elliminate unnecessary rows (cols) in matrix  */
#define NO_COMPACT       1	/* ... or not                                    */


/* define macros for fortran symbols (called directly). Needed because 
   of platform invariance on fortran->C symbol translations
 */

#if defined(HAVE_LIBBLAS) && defined(HAVE_LIBLAPACK)
#define f77_dgesv                   dgesv_
#define f77_dgemm                   dgemm_
#define f77_dnrm2                   dnrm2_
#endif

/* Operations should know type of coefficient matrix, so that
   they can call the right driver
 */

typedef enum
{ NONSYM, SYM, HERMITIAN } mat_type;
typedef enum
{ MATRIX_, ROWVEC_, COLVEC_ } mat_spec;
typedef enum
{ RVEC, CVEC } vtype;


/************************************************************
 *                                                          *
 * A general matrix wrapper for use with BLAS / LAPACK      *
 *  routines, and perhaps elsewhere                         *
 *                                                          *
 ************************************************************/

typedef struct matrix_
{

    mat_spec type;		/* matrix, row vector or column vector? */
    int v_indx;			/* If a vector, which row(column) is active?
				 * If a matrix this is ignored. If value is < 0,
				 * the first row(column) is assumed, ie. index 0.  */
    int rows, cols;		/* Rows and columns of matrix */
    int ldim;			/* Lead dimension of matrix. How many `rows' are
				 * alloc'ed? May exceed real number of rows `rows' */
    doublereal *vals;		/* The values (should be dimensioned to lda * cols */
    int is_init;		/* Is  matrix initialised: values array
				 * is allocated and parameters set ?               */
} mat_struct;

typedef mat_struct vec_struct;


/* Prototypes */

/* Matrix routines corresponding to BLAS Level III */

mat_struct *G_matrix_init(int, int, int);
int G_matrix_zero(mat_struct *);
int G_matrix_set(mat_struct *, int, int, int);
mat_struct *G_matrix_copy(const mat_struct *);
mat_struct *G_matrix_add(mat_struct *, mat_struct *);
mat_struct *G_matrix_subtract(mat_struct *, mat_struct *);
mat_struct *G_matrix_scale(mat_struct *, const double);
mat_struct *G__matrix_add(mat_struct *, mat_struct *, const double,
			  const double);
mat_struct *G_matrix_product(mat_struct *, mat_struct *);
mat_struct *G_matrix_transpose(mat_struct *);
int G_matrix_LU_solve(const mat_struct *, mat_struct **, const mat_struct *,
		      mat_type);
mat_struct *G_matrix_inverse(mat_struct *);
void G_matrix_free(mat_struct *);
void G_matrix_print(mat_struct *);
int G_matrix_set_element(mat_struct *, int, int, double);
double G_matrix_get_element(mat_struct *, int, int);


/* Matrix-vector routines corresponding to BLAS Level II */

vec_struct *G_matvect_get_column(mat_struct *, int);
vec_struct *G_matvect_get_row(mat_struct *, int);
int G_matvect_extract_vector(mat_struct *, vtype, int);
int G_matvect_retrieve_matrix(vec_struct *);


/* Vector routines corresponding to BLAS Level I */

vec_struct *G_vector_init(int, int, vtype);
int G_vector_set(vec_struct *, int, int, vtype, int);
double G_vector_norm_euclid(vec_struct *);
double G_vector_norm_maxval(vec_struct *, int);
vec_struct *G_vector_copy(const vec_struct *, int);

/* Matrix and vector routines corresponding to ?? */

void G_vector_free(vec_struct *);
vec_struct *G_vector_sub(vec_struct *, vec_struct *, vec_struct *);
double G_vector_norm1(vec_struct *);
int G_matrix_read(FILE *, mat_struct *);
int G_matrix_stdin(mat_struct *);
int G_matrix_eigen_sort(vec_struct *, mat_struct *);

#endif /* LA_H_ */
