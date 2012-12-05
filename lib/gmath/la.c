
/******************************************************************************
 * la.c
 * wrapper modules for linear algebra problems
 * linking to BLAS / LAPACK (and others?)

 * @Copyright David D.Gray <ddgray@armadce.demon.co.uk>
 * 26th. Sep. 2000
 * Last updated:
 * 2006-11-23

 * This file is part of GRASS GIS. It is free software. You can 
 * redistribute it and/or modify it under the terms of 
 * the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 ******************************************************************************/

#include <stdio.h>		/* needed here for ifdef/else */
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <grass/config.h>

#if defined(HAVE_LIBLAPACK) && defined(HAVE_LIBBLAS)

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/la.h>


static int egcmp(const void *pa, const void *pb);


/*!
 * \fn mat_struct *G_matrix_init(int rows, int cols, int ldim)
 *
 * \brief Initialize a matrix structure
 *
 * Initialize a matrix structure
 *
 * \param rows
 * \param cols
 * \param ldim
 * \return mat_struct
 */

mat_struct *G_matrix_init(int rows, int cols, int ldim)
{
    mat_struct *tmp_arry;

    if (rows < 1 || cols < 1 || ldim < rows) {
	G_warning(_("Matrix dimensions out of range"));
	return NULL;
    }

    tmp_arry = (mat_struct *) G_malloc(sizeof(mat_struct));
    tmp_arry->rows = rows;
    tmp_arry->cols = cols;
    tmp_arry->ldim = ldim;
    tmp_arry->type = MATRIX_;
    tmp_arry->v_indx = -1;

    tmp_arry->vals = (doublereal *) G_calloc(ldim * cols, sizeof(doublereal));
    tmp_arry->is_init = 1;

    return tmp_arry;
}


/*!
 * \fn int G_matrix_zero (mat_struct *A)
 *
 * \brief Clears (or resets) the matrix values to 0
 *
 * \param A
 * \return 0 on error; 1 on success
 */

int G_matrix_zero(mat_struct * A)
{
    if (!A->vals)
	return 0;

    memset(A->vals, 0, sizeof(A->vals));

    return 1;
}


/*!
 * \fn int G_matrix_set(mat_struct *A, int rows, int cols, int ldim)
 *
 * \brief Set paramaters for an initialized matrix
 *
 * Set parameters for matrix <b>A</b> that is allocated,
 * but not yet fully initialized.  Is an alternative to G_matrix_init().
 *
 * \param A
 * \param rows
 * \param cols
 * \param ldim
 * \return int
 */

int G_matrix_set(mat_struct * A, int rows, int cols, int ldim)
{
    if (rows < 1 || cols < 1 || ldim < 0) {
	G_warning(_("Matrix dimensions out of range"));
	return -1;
    }

    A->rows = rows;
    A->cols = cols;
    A->ldim = ldim;
    A->type = MATRIX_;
    A->v_indx = -1;

    A->vals = (doublereal *) G_calloc(ldim * cols, sizeof(doublereal));
    A->is_init = 1;

    return 0;
}


/*!
 * \fn mat_struct *G_matrix_copy (const mat_struct *A)
 *
 * \brief Copy a matrix
 *
 * Copy matrix <b>A</b> by exactly duplicating its contents.
 *
 * \param A
 * \return mat_struct
 */

mat_struct *G_matrix_copy(const mat_struct * A)
{
    mat_struct *B;

    if (!A->is_init) {
	G_warning(_("Matrix is not initialised fully."));
	return NULL;
    }

    if ((B = G_matrix_init(A->rows, A->cols, A->ldim)) == NULL) {
	G_warning(_("Unable to allocate space for matrix copy"));
	return NULL;
    }

    memcpy(&B->vals[0], &A->vals[0], A->cols * A->ldim * sizeof(doublereal));

    return B;
}


/*!
 * \fn mat_struct *G_matrix_add (mat_struct *mt1, mat_struct *mt2)
 *
 * \brief Adds two matricies
 *
 *  Adds two matricies <b>mt1</b> and <b>mt2</b> and returns a
 * resulting matrix. The return structure is automatically initialized.
 *
 * \param mt1
 * \param mt2
 * \return mat_struct
 */

mat_struct *G_matrix_add(mat_struct * mt1, mat_struct * mt2)
{
    return G__matrix_add(mt1, mt2, 1, 1);
}


/*!
 * \fn mat_struct *G_matrix_subtract (mat_struct *mt1, mat_struct *mt2)
 *
 * \brief Subtract two matricies
 *
 * Subtracts two matricies <b>mt1</b> and <b>mt2</b> and returns
 * a resulting matrix. The return matrix is automatically initialized.
 *
 * \param mt1
 * \param mt2
 * \return mat_struct
 */

mat_struct *G_matrix_subtract(mat_struct * mt1, mat_struct * mt2)
{
    return G__matrix_add(mt1, mt2, 1, -1);
}


/*!
 * \fn mat_struct *G_matrix_scale (mat_struct *mt1, const double c)
 *
 * \brief Scale a matrix by a scalar value
 *
 * Scales matrix <b>mt1</b> by scalar value <b>c</b>. The
 * resulting matrix is automatically initialized.
 *
 * \param mt1
 * \param c
 * \return mat_struct
 */

mat_struct *G_matrix_scale(mat_struct * mt1, const double c)
{
    return G__matrix_add(mt1, NULL, c, 0);
}


/*!
 * \fn mat_struct *G__matrix_add (mat_struct *mt1, mat_struct *mt2, const double c1, const double c2)
 *
 * \brief General add/subtract/scalar multiply routine
 *
 * General add/subtract/scalar multiply routine. <b>c2</b> may be
 * zero, but <b>c1</b> must be non-zero.
 *
 * \param mt1
 * \param mt2
 * \param c1
 * \param c2
 * \return mat_struct
 */

mat_struct *G__matrix_add(mat_struct * mt1, mat_struct * mt2, const double c1,
			  const double c2)
{
    mat_struct *mt3;
    int i, j;			/* loop variables */

    if (c1 == 0) {
	G_warning(_("First scalar multiplier must be non-zero"));
	return NULL;
    }

    if (c2 == 0) {
	if (!mt1->is_init) {
	    G_warning(_("One or both input matrices uninitialised"));
	    return NULL;
	}
    }

    else {

	if (!((mt1->is_init) && (mt2->is_init))) {
	    G_warning(_("One or both input matrices uninitialised"));
	    return NULL;
	}

	if (mt1->rows != mt2->rows || mt1->cols != mt2->cols) {
	    G_warning(_("Matrix order does not match"));
	    return NULL;
	}
    }

    if ((mt3 = G_matrix_init(mt1->rows, mt1->cols, mt1->ldim)) == NULL) {
	G_warning(_("Unable to allocate space for matrix sum"));
	return NULL;
    }

    if (c2 == 0) {

	for (i = 0; i < mt3->rows; i++) {
	    for (j = 0; j < mt3->cols; j++) {
		mt3->vals[i + mt3->ldim * j] =
		    c1 * mt1->vals[i + mt1->ldim * j];
	    }
	}
    }

    else {

	for (i = 0; i < mt3->rows; i++) {
	    for (j = 0; j < mt3->cols; j++) {
		mt3->vals[i + mt3->ldim * j] =
		    c1 * mt1->vals[i + mt1->ldim * j] + c2 * mt2->vals[i +
								       mt2->
								       ldim *
								       j];
	    }
	}
    }

    return mt3;
}


#if defined(HAVE_LIBBLAS)

/*!
 * \fn mat_struct *G_matrix_product (mat_struct *mt1, mat_struct *mt2)
 *
 * \brief Returns product of two matricies
 *
 *  Returns a matrix with the product of matrix <b>mt1</b> and 
 * <b>mt2</b>. The return matrix is automatically initialized.
 *
 * \param mt1
 * \param mt2
 * \return mat_struct
 */

mat_struct *G_matrix_product(mat_struct * mt1, mat_struct * mt2)
{
    mat_struct *mt3;
    doublereal unity = 1, zero = 0;
    integer rows, cols, interdim, lda, ldb;
    integer1 no_trans = 'n';

    if (!((mt1->is_init) || (mt2->is_init))) {
	G_warning(_("One or both input matrices uninitialised"));
	return NULL;
    }

    if (mt1->cols != mt2->rows) {
	G_warning(_("Matrix order does not match"));
	return NULL;
    }

    if ((mt3 = G_matrix_init(mt1->rows, mt2->cols, mt1->ldim)) == NULL) {
	G_warning(_("Unable to allocate space for matrix product"));
	return NULL;
    }

    /* Call the driver */

    rows = (integer) mt1->rows;
    interdim = (integer) mt1->cols;
    cols = (integer) mt2->cols;

    lda = (integer) mt1->ldim;
    ldb = (integer) mt2->ldim;

    f77_dgemm(&no_trans, &no_trans, &rows, &cols, &interdim, &unity,
	      mt1->vals, &lda, mt2->vals, &ldb, &zero, mt3->vals, &lda);

    return mt3;
}

#else /* defined(HAVE_LIBBLAS) */
#warning G_matrix_product() not compiled; requires BLAS library
#endif /* defined(HAVE_LIBBLAS) */

/*!
 * \fn mat_struct *G_matrix_transpose (mat_struct *mt)
 *
 * \brief Transpose a matrix
 *
 * Transpose matrix <b>m1</b> by creating a new one and 
 * populating with transposed elements. The return matrix is 
 * automatically initialized.
 *
 * \param mt
 * \return mat_struct
 */

mat_struct *G_matrix_transpose(mat_struct * mt)
{
    mat_struct *mt1;
    int ldim, ldo;
    doublereal *dbo, *dbt, *dbx, *dby;
    int cnt, cnt2;

    /* Word align the workspace blocks */
    if (mt->cols % 2 == 0)
	ldim = mt->cols;
    else
	ldim = mt->cols + 1;

    mt1 = G_matrix_init(mt->cols, mt->rows, ldim);

    /* Set initial values for reading arrays */
    dbo = &mt->vals[0];
    dbt = &mt1->vals[0];
    ldo = mt->ldim;

    for (cnt = 0; cnt < mt->cols; cnt++) {
	dbx = dbo;
	dby = dbt;

	for (cnt2 = 0; cnt2 < ldo - 1; cnt2++) {
	    *dby = *dbx;
	    dby += ldim;
	    dbx++;
	}

	*dby = *dbx;

	if (cnt < mt->cols - 1) {
	    dbo += ldo;
	    dbt++;
	}
    }

    return mt1;
}


#if defined(HAVE_LIBBLAS) && defined(HAVE_LIBLAPACK)

/*!
 * \fn int G_matrix_LU_solve (const mat_struct *mt1, mat_struct **xmat0, 
 *                      const mat_struct *bmat, mat_type mtype)
 *
 * \brief Solve a general system A.X = B
 *
 * Solve a general system A.X = B, where A is a NxN matrix, X and B are 
 * NxC matrices, and we are to solve for C arrays in X given B. Uses LU 
 * decomposition.<br>
 * Links to LAPACK function dgesv_() and similar to perform the core routine.
 * (By default solves for a general non-symmetric matrix.)<br>
 * mtype is a flag to indicate what kind of matrix (real/complex, Hermitian,
 * symmetric, general etc.) is used (NONSYM, SYM, HERMITIAN).<br>
 * <b>Warning:</b> NOT YET COMPLETE: only some solutions' options
 * available. Now, only general real matrix is supported.
 *
 *  \param mt1
 *  \param xmat0
 *  \param bmat
 *  \param mtype
 *  \return int
 */

/*** NOT YET COMPLETE: only some solutions' options available ***/

int
G_matrix_LU_solve(const mat_struct * mt1, mat_struct ** xmat0,
		  const mat_struct * bmat, mat_type mtype)
{
    mat_struct *wmat, *xmat, *mtx;

    if (mt1->is_init == 0 || bmat->is_init == 0) {
	G_warning(_("Input: one or both data matrices uninitialised"));
	return -1;
    }

    if (mt1->rows != mt1->cols || mt1->rows < 1) {
	G_warning(_("Principal matrix is not properly dimensioned"));
	return -1;
    }

    if (bmat->cols < 1) {
	G_warning(_("Input: you must have at least one array to solve"));
	return -1;
    }

    /* Now create solution matrix by copying the original coefficient matrix */
    if ((xmat = G_matrix_copy(bmat)) == NULL) {
	G_warning(_("Could not allocate space for solution matrix"));
	return -1;
    }

    /* Create working matrix for the coefficient array */
    if ((mtx = G_matrix_copy(mt1)) == NULL) {
	G_warning(_("Could not allocate space for working matrix"));
	return -1;
    }

    /* Copy the contents of the data matrix, to preserve the
       original information 
     */
    if ((wmat = G_matrix_copy(bmat)) == NULL) {
	G_warning(_("Could not allocate space for working matrix"));
	return -1;
    }

    /* Now call appropriate LA driver to solve equations */
    switch (mtype) {

    case NONSYM:
	{
	    integer *perm, res_info;
	    integer num_eqns, nrhs, lda, ldb;

	    perm = (integer *) G_malloc(wmat->rows);

	    /* Set fields to pass to fortran routine */
	    num_eqns = (integer) mt1->rows;
	    nrhs = (integer) wmat->cols;
	    lda = (integer) mt1->ldim;
	    ldb = (integer) wmat->ldim;

	    /* Call LA driver */
	    f77_dgesv(&num_eqns, &nrhs, mtx->vals, &lda, perm, wmat->vals,
		      &ldb, &res_info);

	    /* Copy the results from the modified data matrix, taking account 
	       of pivot permutations ???
	     */

	    /*
	       for(indx1 = 0; indx1 < num_eqns; indx1++) {
	       iperm = perm[indx1];
	       ptin = &wmat->vals[0] + indx1;
	       ptout = &xmat->vals[0] + iperm;

	       for(indx2 = 0; indx2 < nrhs - 1; indx2++) {
	       *ptout = *ptin;
	       ptin += wmat->ldim;
	       ptout += xmat->ldim;
	       }

	       *ptout = *ptin;        
	       }
	     */

	    memcpy(xmat->vals, wmat->vals,
		   wmat->cols * wmat->ldim * sizeof(doublereal));

	    /* Free temp arrays */
	    G_free(perm);
	    G_matrix_free(wmat);
	    G_matrix_free(mtx);

	    if (res_info > 0) {
		G_warning(_("Matrix (or submatrix is singular). Solution undetermined"));
		return 1;
	    }
	    else if (res_info < 0) {
		G_warning(_("Problem in LA routine."));
		return -1;
	    }
	    break;
	}
    default:
	{
	    G_warning(_("Procedure not yet available for selected matrix type"));
	    return -1;
	}
    }				/* end switch */

    *xmat0 = xmat;

    return 0;
}

#else /* defined(HAVE_LIBBLAS) && defined(HAVE_LIBLAPACK) */
#warning G_matrix_LU_solve() not compiled; requires BLAS and LAPACK libraries
#endif /* defined(HAVE_LIBBLAS) && defined(HAVE_LIBLAPACK) */

#if defined(HAVE_LIBBLAS) && defined(HAVE_LIBLAPACK)

/*!
 * \fn mat_struct *G_matrix_inverse (mat_struct *mt)
 *
 * \brief Returns the matrix inverse
 *
 * Calls G_matrix_LU_solve() to obtain matrix inverse using LU 
 * decomposition. Returns NULL on failure.
 *
 * \param mt
 * \return mat_struct
 */

mat_struct *G_matrix_inverse(mat_struct * mt)
{
    mat_struct *mt0, *res;
    int i, j, k;		/* loop */

    if (mt->rows != mt->cols) {
	G_warning(_("Matrix is not square. Cannot determine inverse"));
	return NULL;
    }

    if ((mt0 = G_matrix_init(mt->rows, mt->rows, mt->ldim)) == NULL) {
	G_warning(_("Unable to allocate space for matrix"));
	return NULL;
    }

    /* Set `B' matrix to unit matrix */
    for (i = 0; i < mt0->rows - 1; i++) {
	mt0->vals[i + i * mt0->ldim] = 1.0;

	for (j = i + 1; j < mt0->cols; j++) {
	    mt0->vals[i + j * mt0->ldim] = mt0->vals[j + i * mt0->ldim] = 0.0;
	}
    }

    mt0->vals[mt0->rows - 1 + (mt0->rows - 1) * mt0->ldim] = 1.0;

    /* Solve system */
    if ((k = G_matrix_LU_solve(mt, &res, mt0, NONSYM)) == 1) {
	G_warning(_("Matrix is singular"));
	G_matrix_free(mt0);
	return NULL;
    }
    else if (k < 0) {
	G_warning(_("Problem in LA procedure."));
	G_matrix_free(mt0);
	return NULL;
    }
    else {
	G_matrix_free(mt0);
	return res;
    }
}

#else /* defined(HAVE_LIBBLAS) && defined(HAVE_LIBLAPACK) */
#warning G_matrix_inverse() not compiled; requires BLAS and LAPACK libraries
#endif /* defined(HAVE_LIBBLAS) && defined(HAVE_LIBLAPACK) */


/*!
 * \fn void G_matrix_free (mat_struct *mt)
 *
 * \brief Free up allocated matrix
 *
 * Free up allocated matrix.
 *
 * \param mt
 * \return void
 */

void G_matrix_free(mat_struct * mt)
{
    if (mt->is_init)
	G_free(mt->vals);

    G_free(mt);
}


/*!
 * \fn void G_matrix_print (mat_struct *mt)
 *
 * \brief Print out a matrix
 *
 * Print out a representation of the matrix to standard output.
 *
 *  \param mt
 *  \return void
 */

void G_matrix_print(mat_struct * mt)
{
    int i, j;
    char buf[64], numbuf[64];

    for (i = 0; i < mt->rows; i++) {
	strcpy(buf, "");

	for (j = 0; j < mt->cols; j++) {

	    sprintf(numbuf, "%14.6f", G_matrix_get_element(mt, i, j));
	    strcat(buf, numbuf);
	    if (j < mt->cols - 1)
		strcat(buf, ", ");
	}

	G_message("%s", buf);
    }

    fprintf(stderr, "\n");
}


/*!
 * \fn int G_matrix_set_element (mat_struct *mt, int rowval, int colval, double val)
 *
 * \brief Set the value of the (i, j)th element
 *
 * Set the value of the (i, j)th element to a double value. Index values 
 * are C-like ie. zero-based. The row number is given first as is 
 * conventional. Returns -1 if the accessed cell is outside the bounds.
 *
 *  \param mt
 *  \param rowval
 *  \param colval
 *  \param val
 *  \return int
 */

int G_matrix_set_element(mat_struct * mt, int rowval, int colval, double val)
{
    if (!mt->is_init) {
	G_warning(_("Element array has not been allocated"));
	return -1;
    }

    if (rowval >= mt->rows || colval >= mt->cols || rowval < 0 || colval < 0) {
	G_warning(_("Specified element is outside array bounds"));
	return -1;
    }

    mt->vals[rowval + colval * mt->ldim] = (doublereal) val;

    return 0;
}


/*!
 * \fn double G_matrix_get_element (mat_struct *mt, int rowval, int colval)
 *
 * \brief Retrieve value of the (i,j)th element
 *
 * Retrieve the value of the (i, j)th element to a double value. Index 
 * values are C-like ie. zero-based.
 * <b>Note:</b> Does currently not set an error flag for bounds checking.
 *
 *  \param mt
 *  \param rowval
 *  \param colval
 *  \return double
 */

double G_matrix_get_element(mat_struct * mt, int rowval, int colval)
{
    double val;

    /* Should do some checks, but this would require an error control
       system: later? */

    return (val = (double)mt->vals[rowval + colval * mt->ldim]);
}


/*!
 * \fn vec_struct *G_matvect_get_column (mat_struct *mt, int col)
 *
 * \brief Retrieve a column of the matrix to a vector structure
 *
 * Retrieve a column of matrix <b>mt</b> to a returning vector structure
 *
 * \param mt
 * \param col
 * \return vec_struct
 */

vec_struct *G_matvect_get_column(mat_struct * mt, int col)
{
    int i;			/* loop */
    vec_struct *vc1;

    if (col < 0 || col >= mt->cols) {
	G_warning(_("Specified matrix column index is outside range"));
	return NULL;
    }

    if (!mt->is_init) {
	G_warning(_("Matrix is not initialised"));
	return NULL;
    }

    if ((vc1 = G_vector_init(mt->rows, mt->ldim, CVEC)) == NULL) {
	G_warning(_("Could not allocate space for vector structure"));
	return NULL;
    }

    for (i = 0; i < mt->rows; i++)
	G_matrix_set_element((mat_struct *) vc1, i, 0,
			     G_matrix_get_element(mt, i, col));

    return vc1;
}


/*!
 * \fn vec_struct *G_matvect_get_row (mat_struct *mt, int row)
 *
 * \brief Retrieve a row of the matrix to a vector structure
 *
 * Retrieves a row from matrix <b>mt</b> and returns it in a vector 
 * structure.
 *
 * \param mt
 * \param row
 * \return vec_struct
 */

vec_struct *G_matvect_get_row(mat_struct * mt, int row)
{
    int i;			/* loop */
    vec_struct *vc1;

    if (row < 0 || row >= mt->cols) {
	G_warning(_("Specified matrix row index is outside range"));
	return NULL;
    }

    if (!mt->is_init) {
	G_warning(_("Matrix is not initialised"));
	return NULL;
    }

    if ((vc1 = G_vector_init(mt->cols, mt->ldim, RVEC)) == NULL) {
	G_warning(_("Could not allocate space for vector structure"));
	return NULL;
    }

    for (i = 0; i < mt->cols; i++)
	G_matrix_set_element((mat_struct *) vc1, 0, i,
			     G_matrix_get_element(mt, row, i));

    return vc1;
}


/*!
 * \fn int G_matvect_extract_vector (mat_struct *mt, vtype vt, int indx)
 *
 * \brief Convert matrix to vector
 *
 * Convert the matrix <b>mt</b> to a vector structure. The vtype, 
 * <b>vt</b>, is RVEC or CVEC which specifies a row vector or column 
 * vector. The index, <b>indx</b>, indicates the row/column number (zero based).
 *
 * \param mt
 * \param vt
 * \param indx
 * \return int
 */

int G_matvect_extract_vector(mat_struct * mt, vtype vt, int indx)
{
    if (vt == RVEC && indx >= mt->rows) {
	G_warning(_("Specified row index is outside range"));
	return -1;
    }

    else if (vt == CVEC && indx >= mt->cols) {
	G_warning(_("Specified column index is outside range"));
	return -1;
    }

    switch (vt) {

    case RVEC:
	{
	    mt->type = ROWVEC_;
	    mt->v_indx = indx;
	}

    case CVEC:
	{
	    mt->type = COLVEC_;
	    mt->v_indx = indx;
	}

    default:
	{
	    G_warning(_("Unknown vector type."));
	    return -1;
	}

    }

    return 0;

}


/*!
 * \fn int G_matvect_retrieve_matrix (vec_struct *vc)
 *
 * \brief Revert a vector to matrix
 *
 * Revert vector <b>vc</b> to a matrix.
 *
 *  \param vc
 *  \return int
 */

int G_matvect_retrieve_matrix(vec_struct * vc)
{
    /* We have to take the integrity of the vector structure
       largely on trust
     */

    vc->type = MATRIX_;
    vc->v_indx = -1;

    return 0;
}


/*!
 * \fn vec_struct *G_vector_init (int cells, int ldim, vtype vt)
 *
 * \brief Initialize a vector structure
 *
 * Returns an initialized vector structure with <b>cell</b> cells, 
 * of dimension <b>ldim</b>, and of type <b>vt</b>.
 *
 * \param cells
 * \param ldim
 * \param vt
 * \return vec_struct
 */

vec_struct *G_vector_init(int cells, int ldim, vtype vt)
{
    vec_struct *tmp_arry;

    if ((cells < 1) || (vt == RVEC && ldim < 1)
	|| (vt == CVEC && ldim < cells) || ldim < 0) {
	G_warning("Vector dimensions out of range.");
	return NULL;
    }

    tmp_arry = (vec_struct *) G_malloc(sizeof(vec_struct));

    if (vt == RVEC) {
	tmp_arry->rows = 1;
	tmp_arry->cols = cells;
	tmp_arry->ldim = ldim;
	tmp_arry->type = ROWVEC_;
    }

    else if (vt == CVEC) {
	tmp_arry->rows = cells;
	tmp_arry->cols = 1;
	tmp_arry->ldim = ldim;
	tmp_arry->type = COLVEC_;
    }

    tmp_arry->v_indx = 0;

    tmp_arry->vals = (doublereal *) G_calloc(ldim * tmp_arry->cols,
					     sizeof(doublereal));
    tmp_arry->is_init = 1;

    return tmp_arry;
}


/*!
 * \fn void G_vector_free (vec_struct *v)
 *
 * \brief Free an allocated vector structure
 *
 * Free an allocated vector structure.
 *
 * \param v
 * \return void
 */

void G_vector_free(vec_struct * v)
{
    if (v->is_init)
	G_free(v->vals);

    G_free(v);
}


/*!
 * \fn vec_struct *G_vector_sub (vec_struct *v1, vec_struct *v2, vec_struct *out)
 *
 * \brief Subtract two vectors
 *
 * Subtracts two vectors, <b>v1</b> and <b>v2</b>, and returns and 
 * populates vector <b>out</b>.
 *
 * \param v1
 * \param v2
 * \param out
 * \return vec_struct
 */

vec_struct *G_vector_sub(vec_struct * v1, vec_struct * v2, vec_struct * out)
{
    int idx1, idx2, idx0;
    int i;

    if (!out->is_init) {
	G_warning(_("Output vector is uninitialized"));
	return NULL;
    }

    if (v1->type != v2->type) {
	G_warning(_("Vectors are not of the same type"));
	return NULL;
    }

    if (v1->type != out->type) {
	G_warning(_("Output vector is of incorrect type"));
	return NULL;
    }

    if (v1->type == MATRIX_) {
	G_warning(_("Matrices not allowed"));
	return NULL;
    }

    if ((v1->type == ROWVEC_ && v1->cols != v2->cols) ||
	(v1->type == COLVEC_ && v1->rows != v2->rows)) {
	G_warning(_("Vectors have differing dimensions"));
	return NULL;
    }

    if ((v1->type == ROWVEC_ && v1->cols != out->cols) ||
	(v1->type == COLVEC_ && v1->rows != out->rows)) {
	G_warning(_("Output vector has incorrect dimension"));
	return NULL;
    }

    idx1 = (v1->v_indx > 0) ? v1->v_indx : 0;
    idx2 = (v2->v_indx > 0) ? v2->v_indx : 0;
    idx0 = (out->v_indx > 0) ? out->v_indx : 0;

    if (v1->type == ROWVEC_) {
	for (i = 0; i < v1->cols; i++)
	    G_matrix_set_element(out, idx0, i,
				 G_matrix_get_element(v1, idx1, i) -
				 G_matrix_get_element(v2, idx2, i));
    }
    else {
	for (i = 0; i < v1->rows; i++)
	    G_matrix_set_element(out, i, idx0,
				 G_matrix_get_element(v1, i, idx1) -
				 G_matrix_get_element(v2, i, idx2));
    }

    return out;
}


/*!
 * \fn int G_vector_set (vec_struct *A, int cells, int ldim, vtype vt, int vindx)
 *
 * \brief Set parameters for vector structure
 *
 * Set parameters for a vector structure that is
 * allocated but not yet initialised fully. The vtype is RVEC or
 * CVEC which specifies a row vector or column vector.
 *
 * \param A
 * \param cells
 * \param ldim
 * \param vt
 * \param vindx
 * \return int
 */

int G_vector_set(vec_struct * A, int cells, int ldim, vtype vt, int vindx)
{
    if ((cells < 1) || (vt == RVEC && ldim < 1)
	|| (vt == CVEC && ldim < cells) || ldim < 0) {
	G_warning(_("Vector dimensions out of range"));
	return -1;
    }

    if ((vt == RVEC && vindx >= A->cols) || (vt == CVEC && vindx >= A->rows)) {
	G_warning(_("Row/column out of range"));
	return -1;
    }

    if (vt == RVEC) {
	A->rows = 1;
	A->cols = cells;
	A->ldim = ldim;
	A->type = ROWVEC_;
    }
    else {
	A->rows = cells;
	A->cols = 1;
	A->ldim = ldim;
	A->type = COLVEC_;
    }

    if (vindx < 0)
	A->v_indx = 0;
    else
	A->v_indx = vindx;

    A->vals = (doublereal *) G_calloc(ldim * A->cols, sizeof(doublereal));
    A->is_init = 1;

    return 0;

}


#if defined(HAVE_LIBBLAS)

/*!
 * \fn double G_vector_norm_euclid (vec_struct *vc)
 *
 * \brief Calculates euclidean norm
 *
 * Calculates the euclidean norm of a row or column vector, using BLAS
 * routine dnrm2_().
 *
 * \param vc
 * \return double
 */

double G_vector_norm_euclid(vec_struct * vc)
{
    integer incr, Nval;
    doublereal *startpt;

    if (!vc->is_init)
	G_fatal_error(_("Matrix is not initialised"));

    if (vc->type == ROWVEC_) {
	Nval = (integer) vc->cols;
	incr = (integer) vc->ldim;
	if (vc->v_indx < 0)
	    startpt = vc->vals;
	else
	    startpt = vc->vals + vc->v_indx;
    }
    else {
	Nval = (integer) vc->rows;
	incr = 1;
	if (vc->v_indx < 0)
	    startpt = vc->vals;
	else
	    startpt = vc->vals + vc->v_indx * vc->ldim;
    }

    /* Call the BLAS routine dnrm2_() */
    return (double)f77_dnrm2(&Nval, startpt, &incr);
}

#else /* defined(HAVE_LIBBLAS) */
#warning G_vector_norm_euclid() not compiled; requires BLAS library
#endif /* defined(HAVE_LIBBLAS) */


/*!
 * \fn double G_vector_norm_maxval (vec_struct *vc, int vflag)
 *
 * \brief Calculates maximum value
 *
 * Calculates the maximum value of a row or column vector.
 * The vflag setting defines which value to be calculated:
 * vflag:
 * 1 Indicates maximum value<br>
 * -1  Indicates minimum value<br>
 * 0 Indicates absolute value [???]
 *
 * \param vc
 * \param vflag
 * \return double
 */

double G_vector_norm_maxval(vec_struct * vc, int vflag)
{
    doublereal xval, *startpt, *curpt;
    double cellval;
    int ncells, incr;

    if (!vc->is_init)
	G_fatal_error(_("Matrix is not initialised"));

    if (vc->type == ROWVEC_) {
	ncells = (integer) vc->cols;
	incr = (integer) vc->ldim;
	if (vc->v_indx < 0)
	    startpt = vc->vals;
	else
	    startpt = vc->vals + vc->v_indx;
    }
    else {
	ncells = (integer) vc->rows;
	incr = 1;
	if (vc->v_indx < 0)
	    startpt = vc->vals;
	else
	    startpt = vc->vals + vc->v_indx * vc->ldim;
    }

    xval = *startpt;
    curpt = startpt;

    while (ncells > 0) {
	if (curpt != startpt) {
	    switch (vflag) {

	    case MAX_POS:
		{
		    if (*curpt > xval)
			xval = *curpt;
		    break;
		}

	    case MAX_NEG:
		{
		    if (*curpt < xval)
			xval = *curpt;
		    break;
		}

	    case MAX_ABS:
		{
		    cellval = (double)(*curpt);
		    if (hypot(cellval, cellval) > (double)xval)
			xval = *curpt;
		}
	    }			/* switch */
	}			/* if(curpt != startpt) */

	curpt += incr;
	ncells--;
    }

    return (double)xval;
}


/*!
 * \fn double G_vector_norm1 (vec_struct *vc)
 *
 * \brief Calculates the 1-norm of a vector
 *
 * Calculates the 1-norm of a vector
 *
 * \param vc
 * \return double
 */

double G_vector_norm1(vec_struct * vc)
{
    double result = 0.0;
    int idx;
    int i;

    if (!vc->is_init) {
	G_warning(_("Matrix is not initialised"));
	return 0.0 / 0.0;	/* NaN */
    }

    idx = (vc->v_indx > 0) ? vc->v_indx : 0;

    if (vc->type == ROWVEC_) {
	for (i = 0; i < vc->cols; i++)
	    result += fabs(G_matrix_get_element(vc, idx, i));
    }
    else {
	for (i = 0; i < vc->rows; i++)
	    result += fabs(G_matrix_get_element(vc, i, idx));
    }

    return result;
}


/*!
 * \fn vec_struct *G_vector_copy (const vec_struct *vc1, int comp_flag)
 *
 * \brief Returns a vector copied from <b>vc1</b>. Underlying structure 
 * is preserved unless DO_COMPACT flag.
 *
 * \param vc1
 * \param comp_flag
 * \return vec_struct
 */

vec_struct *G_vector_copy(const vec_struct * vc1, int comp_flag)
{
    vec_struct *tmp_arry;
    int incr1, incr2;
    doublereal *startpt1, *startpt2, *curpt1, *curpt2;
    int cnt;

    if (!vc1->is_init) {
	G_warning(_("Vector structure is not initialised"));
	return NULL;
    }

    tmp_arry = (vec_struct *) G_malloc(sizeof(vec_struct));

    if (comp_flag == DO_COMPACT) {
	if (vc1->type == ROWVEC_) {
	    tmp_arry->rows = 1;
	    tmp_arry->cols = vc1->cols;
	    tmp_arry->ldim = 1;
	    tmp_arry->type = ROWVEC_;
	    tmp_arry->v_indx = 0;
	}
	else if (vc1->type == COLVEC_) {
	    tmp_arry->rows = vc1->rows;
	    tmp_arry->cols = 1;
	    tmp_arry->ldim = vc1->ldim;
	    tmp_arry->type = COLVEC_;
	    tmp_arry->v_indx = 0;
	}
	else {
	    G_warning("Type is not vector.");
	    return NULL;
	}
    }
    else if (comp_flag == NO_COMPACT) {
	tmp_arry->v_indx = vc1->v_indx;
	tmp_arry->rows = vc1->rows;
	tmp_arry->cols = vc1->cols;
	tmp_arry->ldim = vc1->ldim;
	tmp_arry->type = vc1->type;
    }
    else {
	G_warning("Copy method must be specified: [DO,NO]_COMPACT.\n");
	return NULL;
    }

    tmp_arry->vals = (doublereal *) G_calloc(tmp_arry->ldim * tmp_arry->cols,
					     sizeof(doublereal));
    if (comp_flag == DO_COMPACT) {
	if (tmp_arry->type == ROWVEC_) {
	    startpt1 = tmp_arry->vals;
	    startpt2 = vc1->vals + vc1->v_indx;
	    curpt1 = startpt1;
	    curpt2 = startpt2;
	    incr1 = 1;
	    incr2 = vc1->ldim;
	    cnt = vc1->cols;
	}
	else if (tmp_arry->type == COLVEC_) {
	    startpt1 = tmp_arry->vals;
	    startpt2 = vc1->vals + vc1->v_indx * vc1->ldim;
	    curpt1 = startpt1;
	    curpt2 = startpt2;
	    incr1 = 1;
	    incr2 = 1;
	    cnt = vc1->rows;
	}
	else {
	    G_warning("Structure type is not vector.");
	    return NULL;
	}
    }
    else if (comp_flag == NO_COMPACT) {
	startpt1 = tmp_arry->vals;
	startpt2 = vc1->vals;
	curpt1 = startpt1;
	curpt2 = startpt2;
	incr1 = 1;
	incr2 = 1;
	cnt = vc1->ldim * vc1->cols;
    }
    else {
	G_warning("Copy method must be specified: [DO,NO]_COMPACT.\n");
	return NULL;
    }

    while (cnt > 0) {
	memcpy(curpt1, curpt2, sizeof(doublereal));
	curpt1 += incr1;
	curpt2 += incr2;
	cnt--;
    }

    tmp_arry->is_init = 1;

    return tmp_arry;
}


/*!
 * \fn int G_matrix_read (FILE *fp, mat_struct *out)
 *
 * \brief Read a matrix from a file stream
 *
 * Populates matrix structure <b>out</b> with matrix read from file 
 * stream <b>fp</b>. Matrix <b>out</b> is automatically initialized. 
 * Returns -1 on error and 0 on success.
 *
 * \param fp
 * \param out
 * \return int
 */

int G_matrix_read(FILE * fp, mat_struct * out)
{
    char buff[100];
    int rows, cols;
    int i, j, row;
    double val;

    /* skip comments */
    for (;;) {
	if (!G_getl(buff, sizeof(buff), fp))
	    return -1;
	if (buff[0] != '#')
	    break;
    }

    if (sscanf(buff, "Matrix: %d by %d", &rows, &cols) != 2) {
	G_warning(_("Input format error"));
	return -1;
    }

    G_matrix_set(out, rows, cols, rows);

    for (i = 0; i < rows; i++) {
	if (fscanf(fp, "row%d:", &row) != 1 || row != i) {
	    G_warning(_("Input format error"));
	    return -1;
	}
	for (j = 0; j < cols; j++) {
	    if (fscanf(fp, "%lf:", &val) != 1) {
		G_warning(_("Input format error"));
		return -1;
	    }

	    G_matrix_set_element(out, i, j, val);
	}
    }

    return 0;
}


/*!
 * \fn int G_matrix_read_stdin (mat_struct *out)
 *
 * \brief Read a matrix from standard input
 *
 * Populates matrix <b>out</b> with matrix read from stdin. Matrix 
 * <b>out</b> is automatically initialized. Returns -1 on failure or 0 
 * on success.
 *
 * \param out
 * \return int
 */

int G_matrix_stdin(mat_struct * out)
{
    return G_matrix_read(stdin, out);
}


/*!
 * \fn int G_matrix_eigen_sort (vec_struct *d, mat_struct *m)
 *
 * \brief Sort eigenvectors according to eigenvalues
 *
 * Sort eigenvectors according to eigenvalues. Returns 0.
 *
 * \param d
 * \param m
 * \return int
 */

int G_matrix_eigen_sort(vec_struct * d, mat_struct * m)
{
    mat_struct tmp;
    int i, j;
    int idx;

    G_matrix_set(&tmp, m->rows + 1, m->cols, m->ldim + 1);

    idx = (d->v_indx > 0) ? d->v_indx : 0;

    /* concatenate (vertically) m and d into tmp */
    for (i = 0; i < m->cols; i++) {
	for (j = 0; j < m->rows; j++)
	    G_matrix_set_element(&tmp, j + 1, i,
				 G_matrix_get_element(m, j, i));
	if (d->type == ROWVEC_)
	    G_matrix_set_element(&tmp, 0, i, G_matrix_get_element(d, idx, i));
	else
	    G_matrix_set_element(&tmp, 0, i, G_matrix_get_element(d, i, idx));
    }

    /* sort the combined matrix */
    qsort(tmp.vals, tmp.cols, tmp.ldim * sizeof(doublereal), egcmp);

    /* split tmp into m and d */
    for (i = 0; i < m->cols; i++) {
	for (j = 0; j < m->rows; j++)
	    G_matrix_set_element(m, j, i,
				 G_matrix_get_element(&tmp, j + 1, i));
	if (d->type == ROWVEC_)
	    G_matrix_set_element(d, idx, i, G_matrix_get_element(&tmp, 0, i));
	else
	    G_matrix_set_element(d, i, idx, G_matrix_get_element(&tmp, 0, i));
    }

    G_free(tmp.vals);

    return 0;
}


static int egcmp(const void *pa, const void *pb)
{
    double a = *(doublereal * const)pa;
    double b = *(doublereal * const)pb;

    if (a > b)
	return 1;
    if (a < b)
	return -1;

    return 0;
}


#endif /* HAVE_BLAS && HAVE_LAPACK && HAVE_G2C */
