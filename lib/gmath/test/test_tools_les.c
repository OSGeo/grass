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

#include "test_gmath_lib.h"
#include <stdlib.h>
#include <math.h>

/*!
 * \brief Allocate memory for a (not) quadratic linear equation system which includes the Matrix A, vector x and vector b
 *
 * This function calls #G_math_alloc_les_param
 *
 * \param rows int
 * \param cols int
 * \param type int
 * \return G_math_les *
 *
 * */
G_math_les *G_math_alloc_nquad_les(int rows, int cols, int type)
{
	return G_math_alloc_les_param(rows, cols, type, 2);
}

/*!
 * \brief Allocate memory for a (not) quadratic linear equation system which includes the Matrix A and vector x
 *
 * This function calls #G_math_alloc_les_param
 *
 * \param rows int
 * \param cols int
 * \param type int
 * \return G_math_les *
 *
 * */
G_math_les *G_math_alloc_nquad_les_Ax(int rows, int cols, int type)
{
	return G_math_alloc_les_param(rows, cols, type, 1);
}

/*!
 * \brief Allocate memory for a (not) quadratic linear equation system which includes the Matrix A
 *
 * This function calls #G_math_alloc_les_param
 *
 * \param rows int
 * \param cols int
 * \param type int
 * \return G_math_les *
 *
 * */
G_math_les *G_math_alloc_nquad_les_A(int rows, int cols, int type)
{
	return G_math_alloc_les_param(rows, cols, type, 0);
}

/*!
 * \brief Allocate memory for a (not) quadratic linear equation system which includes the Matrix A, vector x and vector b
 *
 * This function calls #G_math_alloc_les_param
 *
 * \param rows int
 * \param cols int
 * \param type int
 * \return G_math_les *
 *
 * */
G_math_les *G_math_alloc_nquad_les_Ax_b(int rows, int cols, int type)
{
	return G_math_alloc_les_param(rows, cols, type, 2);
}

/*!
 * \brief Allocate memory for a quadratic linear equation system which includes the Matrix A, vector x and vector b
 *
 * This function calls #G_math_alloc_les_param
 *
 * \param rows int
 * \param type int
 * \return G_math_les *
 *
 * */
G_math_les *G_math_alloc_les(int rows, int type)
{
	return G_math_alloc_les_param(rows, rows, type, 2);
}

/*!
 * \brief Allocate memory for a quadratic linear equation system which includes the Matrix A and vector x
 *
 * This function calls #G_math_alloc_les_param
 *
 * \param rows int
 * \param type int
 * \return G_math_les *
 *
 * */
G_math_les *G_math_alloc_les_Ax(int rows, int type)
{
	return G_math_alloc_les_param(rows, rows, type, 1);
}

/*!
 * \brief Allocate memory for a quadratic linear equation system which includes the Matrix A
 *
 * This function calls #G_math_alloc_les_param
 *
 * \param rows int
 * \param type int
 * \return G_math_les *
 *
 * */
G_math_les *G_math_alloc_les_A(int rows, int type)
{
	return G_math_alloc_les_param(rows, rows, type, 0);
}

/*!
 * \brief Allocate memory for a quadratic linear equation system which includes the Matrix A, vector x and vector b
 *
 * This function calls #G_math_alloc_les_param
 *
 * \param rows int
 * \param type int
 * \return G_math_les *
 *
 * */
G_math_les *G_math_alloc_les_Ax_b(int rows, int type)
{
	return G_math_alloc_les_param(rows, rows, type, 2);
}

/*!
 * \brief Allocate memory for a quadratic or not quadratic linear equation system
 *
 * The type of the linear equation system must be G_MATH_NORMAL_LES for
 * a regular quadratic matrix or G_MATH_SPARSE_LES for a sparse matrix
 *
 * <p>
 * In case of G_MATH_NORMAL_LES
 * 
 * A quadratic matrix of size rows*rows*sizeof(double) will allocated
 *
 * <p>
 * In case of G_MATH_SPARSE_LES
 *
 * a vector of size row will be allocated, ready to hold additional allocated sparse vectors.
 * each sparse vector may have a different size.
 *
 * Parameter parts defines which parts of the les should be allocated.
 * The number of columns and rows defines if the matrix is quadratic.
 *
 * \param rows int
 * \param cols int
 * \param type int
 * \param parts int -- 2 = A, x and b; 1 = A and x; 0 = A allocated
 * \return G_math_les *
 *
 * */
G_math_les *G_math_alloc_les_param(int rows, int cols, int type, int parts)
{
	G_math_les *les;

	if (type == G_MATH_SPARSE_LES)
		G_debug(
				2,
				"Allocate memory for a sparse linear equation system with %i rows\n",
				rows);
	else
		G_debug(
				2,
				"Allocate memory for a regular linear equation system with %i rows and %i cols\n",
				rows, cols);

	les = (G_math_les *) G_calloc(1, sizeof(G_math_les));
	les->x = NULL;
	les->b = NULL;

	if (parts > 0)
	{
		les->x = (double *)G_calloc(cols, sizeof(double));
	}

	if (parts > 1)
	{
		les->b = (double *)G_calloc(cols, sizeof(double));
	}

	les->A = NULL;
	les->data = NULL;
	les->Asp = NULL;
	les->rows = rows;
	les->cols = cols;
	les->symm = 0;
	les->bandwidth = cols;
	if (rows == cols)
		les->quad = 1;
	else
		les->quad = 0;

	if (type == G_MATH_SPARSE_LES)
	{
		les->Asp = (G_math_spvector **) G_calloc(rows,
				sizeof(G_math_spvector *));
		les->type = G_MATH_SPARSE_LES;
	}
	else
	{
		les->A = G_alloc_matrix(rows, cols);
		/*save the start pointer of the matrix*/
		les->data = les->A[0];
		les->type = G_MATH_NORMAL_LES;
	}

	return les;
}

/***************** Floating point version ************************/

G_math_f_les *G_math_alloc_f_les(int rows, int type)
{
	return G_math_alloc_f_les_param(rows, rows, type, 2);
}

G_math_f_les *G_math_alloc_f_nquad_les_A(int rows, int cols, int type)
{
	return G_math_alloc_f_les_param(rows, cols, type, 0);
}

G_math_f_les *G_math_alloc_f_les_param(int rows, int cols, int type, int parts)
{
	G_math_f_les *les;

	G_debug(
			2,
			"Allocate memory for a regular float linear equation system with %i rows\n",
			rows);

	les = (G_math_f_les *) G_calloc(1, sizeof(G_math_f_les));
	les->x = NULL;
	les->b = NULL;

	if (parts > 0)
	{
		les->x = (float *)G_calloc(cols, sizeof(float));
	}

	if (parts > 1)
	{
		les->b = (float *)G_calloc(cols, sizeof(float));
	}

	les->A = NULL;
	les->data = NULL;
	les->rows = rows;
	les->cols = cols;
	les->symm = 0;
	les->bandwidth = cols;
	if (rows == cols)
		les->quad = 1;
	else
		les->quad = 0;

	les->A = G_alloc_fmatrix(rows, cols);
	/*save the start pointer of the matrix*/
	les->data = les->A[0];
	les->type = G_MATH_NORMAL_LES;

	return les;
}

/*!
 * \brief Adds a sparse vector to a sparse linear equation system at position row
 *
 * Return 1 for success and -1 for failure
 *
 * \param les G_math_les *
 * \param spvector G_math_spvector * 
 * \param row int
 * \return int 0 success, -1 failure
 *
 * */
int G_math_add_spvector_to_les(G_math_les * les, G_math_spvector * spvector,
		int row)
{

	if (les != NULL)
	{
		if (les->type != G_MATH_SPARSE_LES)
			return -1;

		if (les->rows > row)
		{
			G_debug(
					5,
					"Add sparse vector %p to the sparse linear equation system at row %i\n",
					spvector, row);
			les->Asp[row] = spvector;
		}
		else
			return -1;

	}
	else
	{
		return -1;
	}

	return 1;
}

/*!
 *
 * \brief prints the linear equation system to stdout
 *
 * <p>
 * Format:
 * A*x = b
 *
 * <p>
 * Example
 \verbatim
 
 2 1 1 1 * 2 = 0.1
 1 2 0 0 * 3 = 0.2
 1 0 2 0 * 3 = 0.2
 1 0 0 2 * 2 = 0.1
 
 \endverbatim
 *
 * \param les G_math_les * 
 * \return void
 *  
 * */
void G_math_print_les(G_math_les * les)
{
	int i, j, k, out;

        if (les->type == G_MATH_SPARSE_LES)
	{
		for (i = 0; i < les->rows; i++)
		{
			for (j = 0; j < les->cols; j++)
			{
				out = 0;
				for (k = 0; k < les->Asp[i]->cols; k++)
				{
					if (les->Asp[i]->index[k] == j)
					{
						fprintf(stdout, "%4.5f ", les->Asp[i]->values[k]);
						out = 1;
					}
				}
				if (!out)
					fprintf(stdout, "%4.5f ", 0.0);
			}
			if (les->x)
				fprintf(stdout, "  *  %4.5f", les->x[i]);
			if (les->b)
				fprintf(stdout, " =  %4.5f ", les->b[i]);

			fprintf(stdout, "\n");
		}
	}
	else
	{

		for (i = 0; i < les->rows; i++)
		{
			for (j = 0; j < les->cols; j++)
			{
				fprintf(stdout, "%4.5f ", les->A[i][j]);
			}
			if (les->x)
				fprintf(stdout, "  *  %4.5f", les->x[i]);
			if (les->b)
				fprintf(stdout, " =  %4.5f ", les->b[i]);

			fprintf(stdout, "\n");
		}

	}
	return;
}

/*!
 * \brief Release the memory of the linear equation system
 *
 * \param les G_math_les *            
 * \return void
 *
 * */

void G_math_free_les(G_math_les * les)
{
	int i;

	if (les->type == G_MATH_SPARSE_LES)
		G_debug(2, "Releasing memory of a sparse linear equation system\n");
	else
		G_debug(2, "Releasing memory of a regular linear equation system\n");

	if (les)
	{

		if (les->x)
			G_free(les->x);
		if (les->b)
			G_free(les->b);

		if (les->type == G_MATH_SPARSE_LES)
		{

			if (les->Asp)
			{
				for (i = 0; i < les->rows; i++)
					if (les->Asp[i])
						G_math_free_spvector(les->Asp[i]);

				G_free(les->Asp);
			}
		}
		else
		{
			/*We don't know if the rows have been changed by pivoting, 
			 * so we restore the data pointer*/
			les->A[0] = les->data;
			G_free_matrix(les->A);
		}

		free(les);
	}

	return;
}

/*!
 * \brief Release the memory of the float linear equation system
 *
 * \param les G_math_f_les *            
 * \return void
 *
 * */

void G_math_free_f_les(G_math_f_les * les)
{
	G_debug(2, "Releasing memory of a regular float linear equation system\n");

	if (les)
	{

		if (les->x)
			G_free(les->x);
		if (les->b)
			G_free(les->b);

		/*We don't know if the rows have been changed by pivoting, 
		 * so we restore the data pointer*/
		les->A[0] = les->data;
		G_free_fmatrix(les->A);

		free(les);
	}

	return;
}
