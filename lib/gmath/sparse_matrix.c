
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

#include <stdlib.h>
#include <math.h>
#include <grass/gmath.h>
#include <grass/gis.h>

/*!
 * \brief Adds a sparse vector to a sparse matrix at position row
 *
 * Return 1 for success and -1 for failure
 *
 * \param Asp G_math_spvector **
 * \param spvector G_math_spvector * 
 * \param row int
 * \return int 1 success, -1 failure
 *
 * */
int G_math_add_spvector(G_math_spvector ** Asp, G_math_spvector * spvector,
			int row)
{
    if (Asp != NULL) {
	G_debug(5,
		"Add sparse vector %p to the sparse linear equation system at row %i\n",
		spvector, row);
	Asp[row] = spvector;
    }
    else {
	return -1;
    }

    return 1;
}

/*!
 * \brief Allocate memory for a sparse matrix
 *
 * \param rows int
 * \return G_math_spvector **
 *
 * */
G_math_spvector **G_math_alloc_spmatrix(int rows)
{
    G_math_spvector **spmatrix;

    G_debug(4, "Allocate memory for a sparse matrix with %i rows\n", rows);

    spmatrix = (G_math_spvector **) G_calloc(rows, sizeof(G_math_spvector *));

    return spmatrix;
}

/*!
 * \brief Allocate memory for a sparse vector
 *
 * \param cols int
 * \return G_math_spvector *
 *
 * */
G_math_spvector *G_math_alloc_spvector(int cols)
{
    G_math_spvector *spvector;

    G_debug(4, "Allocate memory for a sparse vector with %i cols\n", cols);

    spvector = (G_math_spvector *) G_calloc(1, sizeof(G_math_spvector));

    spvector->cols = cols;
    spvector->index = (unsigned int *)G_calloc(cols, sizeof(unsigned int));
    spvector->values = (double *)G_calloc(cols, sizeof(double));

    return spvector;
}

/*!
 * \brief Release the memory of the sparse vector
 *
 * \param spvector G_math_spvector *
 * \return void
 *
 * */
void G_math_free_spvector(G_math_spvector * spvector)
{
    if (spvector) {
	if (spvector->values)
	    G_free(spvector->values);
	if (spvector->index)
	    G_free(spvector->index);
	G_free(spvector);

	spvector = NULL;
    }

    return;
}

/*!
 * \brief Release the memory of the sparse matrix
 *
 * \param Asp G_math_spvector **
 * \param rows int
 * \return void
 *
 * */
void G_math_free_spmatrix(G_math_spvector ** Asp, int rows)
{
    int i;

    if (Asp) {
	for (i = 0; i < rows; i++)
	    G_math_free_spvector(Asp[i]);

	G_free(Asp);
	Asp = NULL;
    }

    return;
}

/*!
 *
 * \brief print the sparse matrix Asp to stdout
 *
 *
 * \param Asp (G_math_spvector **)
 * \param rows (int)
 * \return void
 *  
 * */
void G_math_print_spmatrix(G_math_spvector ** Asp, int rows)
{
    int i, j, k, out;

    for (i = 0; i < rows; i++) {
	for (j = 0; j < rows; j++) {
	    out = 0;
	    for (k = 0; k < Asp[i]->cols; k++) {
		if (Asp[i]->index[k] == j) {
		    fprintf(stdout, "%4.5f ", Asp[i]->values[k]);
		    out = 1;
		}
	    }
	    if (!out)
		fprintf(stdout, "%4.5f ", 0.0);
	}
	fprintf(stdout, "\n");
    }

    return;
}


/*!
 * \brief Convert a sparse matrix into a quadratic matrix
 *
 * This function is multi-threaded with OpenMP. It creates its own parallel OpenMP region.
 *
 * \param Asp (G_math_spvector **) 
 * \param rows (int)
 * \return (double **)
 *
 * */
double **G_math_Asp_to_A(G_math_spvector ** Asp, int rows)
{
    int i, j;

    double **A = NULL;

    A = G_alloc_matrix(rows, rows);

#pragma omp parallel for schedule (static) private(i, j)
    for (i = 0; i < rows; i++) {
	for (j = 0; j < Asp[i]->cols; j++) {
	    A[i][Asp[i]->index[j]] = Asp[i]->values[j];
	}
    }
    return A;
}

/*!
 * \brief Convert a symmetric sparse matrix into a symmetric band matrix
 *
 \verbatim
 Symmetric matrix with bandwidth of 3

 5 2 1 0
 2 5 2 1
 1 2 5 2
 0 1 2 5

 will be converted into the band matrix
 
 5 2 1
 5 2 1
 5 2 0
 5 0 0

 \endverbatim
 * \param Asp (G_math_spvector **) 
 * \param rows (int)
 * \param bandwidth (int)
 * \return (double **) the resulting ymmetric band matrix [rows][bandwidth]
 *
 * */
double **G_math_Asp_to_sband_matrix(G_math_spvector ** Asp, int rows, int bandwidth)
{
    int i, j;

    double **A = NULL;

    A = G_alloc_matrix(rows, bandwidth);

    for (i = 0; i < rows; i++) {
	for (j = 0; j < Asp[i]->cols; j++) {
	   if(Asp[i]->index[j] == i) {
	      A[i][0] = Asp[i]->values[j];
	   } else if (Asp[i]->index[j] > i) {
	      A[i][Asp[i]->index[j] - i] = Asp[i]->values[j];
	   }
	}
    }
    return A;
}


/*!
 * \brief Convert a quadratic matrix into a sparse matrix
 *
 * This function is multi-threaded with OpenMP. It creates its own parallel OpenMP region.
 *
 * \param A (double **) 
 * \param rows (int)
 * \param epsilon (double) -- non-zero values are greater then epsilon
 * \return (G_math_spvector **)
 *
 * */
G_math_spvector **G_math_A_to_Asp(double **A, int rows, double epsilon)
{
    int i, j;

    int nonull, count = 0;

    G_math_spvector **Asp = NULL;

    Asp = G_math_alloc_spmatrix(rows);

#pragma omp parallel for schedule (static) private(i, j, nonull, count)
    for (i = 0; i < rows; i++) {
	nonull = 0;
	/*Count the number of non zero entries */
	for (j = 0; j < rows; j++) {
	    if (A[i][j] > epsilon)
		nonull++;
	}
	/*Allocate the sparse vector and insert values */
	G_math_spvector *v = G_math_alloc_spvector(nonull);

	count = 0;
	for (j = 0; j < rows; j++) {
	    if (A[i][j] > epsilon) {
		v->index[count] = j;
		v->values[count] = A[i][j];
		count++;
	    }
	}
	/*Add vector to sparse matrix */
	G_math_add_spvector(Asp, v, i);
    }
    return Asp;
}



/*!
 * \brief Convert a symmetric band matrix into a sparse matrix
 *
 * WARNING:
 * This function is experimental, do not use.
 * Only the upper triangle matrix of the band strcuture is copied.
 *
 * \param A (double **) the symmetric band matrix
 * \param rows (int)
 * \param bandwidth (int)
 * \param epsilon (double) -- non-zero values are greater then epsilon
 * \return (G_math_spvector **)
 *
 * */
G_math_spvector **G_math_sband_matrix_to_Asp(double **A, int rows, int bandwidth, double epsilon)
{
    int i, j;

    int nonull, count = 0;

    G_math_spvector **Asp = NULL;

    Asp = G_math_alloc_spmatrix(rows);

    for (i = 0; i < rows; i++) {
	nonull = 0;
	/*Count the number of non zero entries */
	for (j = 0; j < bandwidth; j++) {
	    if (A[i][j] > epsilon)
		nonull++;
	}

	/*Allocate the sparse vector and insert values */

	G_math_spvector *v = G_math_alloc_spvector(nonull);

	count = 0;
	if (A[i][0] > epsilon) {
	    v->index[count] = i;
	    v->values[count] = A[i][0];
	    count++;
	}

	for (j = 1; j < bandwidth; j++) {
	    if (A[i][j] > epsilon && i + j < rows) {
		v->index[count] = i + j;
		v->values[count] = A[i][j];
		count++;
	    }
	}
	/*Add vector to sparse matrix */
	G_math_add_spvector(Asp, v, i);
    }
    return Asp;
}


/*!
 * \brief Compute the matrix - vector product  
 * of sparse matrix **Asp and vector x.
 *
 * This function is multi-threaded with OpenMP and can be called within a parallel OpenMP region.
 *
 * y = A * x
 *
 *
 * \param Asp (G_math_spvector **) 
 * \param x (double) *)
 * \param y (double * )
 * \param rows (int)
 * \return (void)
 *
 * */
void G_math_Ax_sparse(G_math_spvector ** Asp, double *x, double *y, int rows)
{
    int i, j;

    double tmp;

#pragma omp for schedule (static) private(i, j, tmp)
    for (i = 0; i < rows; i++) {
	tmp = 0;
	for (j = 0; j < Asp[i]->cols; j++) {
	    tmp += Asp[i]->values[j] * x[Asp[i]->index[j]];
	}
	y[i] = tmp;
    }
    return;
}
