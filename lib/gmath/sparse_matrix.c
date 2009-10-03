
/*****************************************************************************
 *
 * MODULE:       Grass Gmath Library
 * AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
 * 		soerengebbert <at> gmx <dot> de
 *               
 * PURPOSE:      functions to manage linear equation systems
 * 		part of the gmath library
 *               
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
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
 * \param spmatrix G_math_spvector ** 
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
 * \param spvector G_math_spvector **
 * \param rows int
 * \return void
 *
 * */
void G_math_free_spmatrix(G_math_spvector ** spmatrix, int rows)
{
    int i;

    if (spmatrix) {
	for (i = 0; i < rows; i++)
	    G_math_free_spvector(spmatrix[i]);

	G_free(spmatrix);
	spmatrix = NULL;
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
