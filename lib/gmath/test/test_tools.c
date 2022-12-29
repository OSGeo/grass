/*****************************************************************************
 *
 * MODULE:       Grass PDE Numerical Library
 * AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
 * 		soerengebbert <at> gmx <dot> de
 *               
 * PURPOSE:      Unit tests for les solving
 *
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/glocale.h>
#include <grass/gmath.h>
#include "test_gmath_lib.h"
#include <sys/time.h>

/* *************************************************************** */
/* Compute the difference between two time steps ***************** */

/* *************************************************************** */
double compute_time_difference(struct timeval start, struct timeval end) {
    int sec;
    int usec;

    sec = end.tv_sec - start.tv_sec;
    usec = end.tv_usec - start.tv_usec;

    return (double) sec + (double) usec / 1000000;
}

/* *************************************************************** */
/* create a normal matrix with values ** Hilbert matrix ********** */
/* *************************************************************** */
G_math_les *create_normal_symmetric_les(int rows)
{
	G_math_les *les;
	int i, j;
	int size =rows;
	double val;

	les = G_math_alloc_les(rows, G_MATH_NORMAL_LES);
	for (i = 0; i < size; i++)
	{
		val = 0.0;
		for (j = 0; j < size; j++)
		{
			if (j == i)
				les->A[i][j] = (double)(1.0/(((double)i + 1.0) + ((double)j + 1.0)));
			else
				les->A[i][j] = (double)(1.0/((((double)i + 1.0) + ((double)j + 1.0) + 100.0)));

			val += les->A[i][j];
		}
		les->b[i] = val;
		les->x[i] = 0.5;
	}

	return les;
}


/* *************************************************************** */
/* create a symmetric band matrix with values ** Hilbert matrix ** */
/* *************************************************************** */
G_math_les *create_symmetric_band_les(int rows)
{
	G_math_les *les;
	int i, j;
	int size =rows;
	double val;

        les = G_math_alloc_les(rows, G_MATH_NORMAL_LES);
	for (i = 0; i < size; i++)
	{
		val = 0.0;
		for (j = 0; j < size; j++)
		{
			if(i + j < size) {
				les->A[i][j] = (double)(1.0/((((double)i + 1.0) + ((double)(i + j) + 1.0) + 100.0)));
			} else if (j != i){
			   	les->A[i][j] = 0.0;
			}
			if (j == i) {
				les->A[i][0] = (double)(1.0/(((double)i + 1.0) + ((double)j + 1.0)));
			} 

			if (j == i) 
				val += (double)(1.0/(((double)i + 1.0) + ((double)j + 1.0)));
			else
				val += (double)(1.0/((((double)i + 1.0) + ((double)j + 1.0) + 100.0)));
		}
		les->b[i] = val;
		les->x[i] = 0.5;
	}

	return les;
}

/* ********************************************************************* */
/* create a bad conditioned normal matrix with values ** Hilbert matrix  */
/* ********************************************************************* */
G_math_les *create_normal_symmetric_pivot_les(int rows)
{
	G_math_les *les;
	int i, ii, j, jj;
	double val;

	les = G_math_alloc_les(rows, G_MATH_NORMAL_LES);
	for (i = 0, ii = rows - 1; i < rows; i++, ii--)
	{
		val = 0.0;
		for (j = 0, jj = rows - 1; j < rows; j++, jj--)
		{
			if (j == i)
				les->A[i][j] = (double)(1.0/(((double)ii*ii*ii*ii*ii + 1.0)*1.1
						+ ((double)jj*jj*jj*jj*jj + 1.0)*1.1));
			else
				les->A[i][j] = (double)(1.0/((((double)ii*ii*ii + 1.0)
						+ ((double)jj*jj*jj + 1.0))));

			val += les->A[i][j];
		}
		les->b[i] = val;
		les->x[i] = 0.0;
	}

	return les;
}

/* *************************************************************** */
/* create a normal matrix with values ** Hilbert matrix ********** */
/* *************************************************************** */
G_math_f_les *create_normal_symmetric_f_les(int rows)
{
	G_math_f_les *les;
	int i, j;
	int size =rows;
	float val;

	les = G_math_alloc_f_les(rows, G_MATH_NORMAL_LES);
	for (i = 0; i < size; i++)
	{
		val = 0.0;
		for (j = 0; j < size; j++)
		{
			if (j == i)
				les->A[i][j] = (float)(1.0
						/(((float)i + 1.0) + ((float)j + 1.0)));
			else
				les->A[i][j] = (float)(1.0/((((float)i + 1.0)
						+ ((float)j + 1.0) + 100.0)));

			val += les->A[i][j];
		}
		les->b[i] = val;
		les->x[i] = 0.5;
	}

	return les;
}

/* *************************************************************** */
/* create a sparse matrix with values ** Hilbert matrix ********** */
/* *************************************************************** */
G_math_les *create_sparse_unsymmetric_les(int rows)
{
	G_math_les *les;
	G_math_spvector *spvector;
	int i, j;
	double val;

	les = G_math_alloc_les(rows, G_MATH_SPARSE_LES);

	for (i = 0; i < rows; i++)
	{
		spvector = G_math_alloc_spvector(rows);
		val = 0;

		for (j = 0; j < rows; j++)
		{
			if (j == i)
			{
				spvector->values[j] = (double)(1.0/((((double)i + 1.0)
						+ ((double)j))));
				spvector->index[j] = j;
			}
			if (j < i)
			{
				spvector->values[j] = (double)(1.0/((((double)i + 1.0)
						+ ((double)j + 1.0) + 100)));
				spvector->index[j] = j;
			}
			if (j > i)
			{
				spvector->values[j] = (double)(1.0/((((double)i + 1.0)
						+ ((double)j + 1.0) + 120)));
				spvector->index[j] = j;
			}

			val += spvector->values[j];
		}

		G_math_add_spvector_to_les(les, spvector, i);
		les->b[i] = val;
		les->x[i] = 0.5;
	}

	return les;
}

/* *************************************************************** */
/* create a normal matrix with values ** Hilbert matrix ********** */
/* *************************************************************** */
G_math_les *create_normal_unsymmetric_les(int rows)
{
	G_math_les *les;
	int i, j;
	int size =rows;
	double val;

	les = G_math_alloc_les(rows, G_MATH_NORMAL_LES);
	for (i = 0; i < size; i++)
	{
		val = 0.0;
		for (j = 0; j < size; j++)
		{
			if (j == i)
				les->A[i][j]
						= (double)(1.0/((((double)i + 1.0) + ((double)j))));
			if (j < i)
				les->A[i][j] = (double)(1.0/((((double)i + 1.0) + ((double)j
						+ 1.0) + 100)));
			if (j > i)
				les->A[i][j] = (double)(1.0/((((double)i + 1.0) + ((double)j
						+ 1.0) + 120)));

			val += les->A[i][j];
		}
		les->b[i] = val;
		les->x[i] = 0.5;
	}

	return les;
}

/* *********************************************************************** */
/* create a non quadratic unsymmetric matrix with values ** Hilbert matrix */
/* *********************************************************************** */
G_math_les *create_normal_unsymmetric_nquad_les_A(int rows, int cols)
{
	G_math_les *les;
	int i, j;

	les = G_math_alloc_nquad_les_A(rows, cols, G_MATH_NORMAL_LES);
	for (i = 0; i < rows; i++)
	{
		for (j = 0; j < cols; j++)
		{
			if (j == i)
				les->A[i][j] = (float)(1.0/((((float)i + 1.0) + ((float)j))));
			if (j < i && j < cols && i < rows)
				les->A[i][j] = (float)(1.0/((((float)i + 1.0)
						+ ((float)j + 1.0) + 100)));
			if (j > i && j < cols && i < rows)
				les->A[i][j] = (float)(1.0/((((float)i + 1.0)
						+ ((float)j + 1.0) + 120)));
		}
	}

	return les;
}

/* ***************************************************************************** */
/* create a non quadratic unsymmetric float matrix with values ** Hilbert matrix */
/* ***************************************************************************** */
G_math_f_les *create_normal_unsymmetric_f_nquad_les_A(int rows, int cols)
{
	G_math_f_les *les;
	int i, j;

	les = G_math_alloc_f_nquad_les_A(rows, cols, G_MATH_NORMAL_LES);
	for (i = 0; i < rows; i++)
	{
		for (j = 0; j < cols; j++)
		{
			if (j == i)
				les->A[i][j] = (float)(1.0/((((float)i + 1.0) + ((float)j))));
			if (j < i&& j < cols && i < rows)
				les->A[i][j] = (float)(1.0/((((float)i + 1.0)
						+ ((float)j + 1.0) + 100)));
			if (j > i&& j < cols && i < rows)
				les->A[i][j] = (float)(1.0/((((float)i + 1.0)
						+ ((float)j + 1.0) + 120)));
		}
	}

	return les;
}

/* *************************************************************** */
/* create a normal matrix with values ** Hilbert matrix ********** */
/* *************************************************************** */
G_math_f_les *create_normal_unsymmetric_f_les(int rows)
{
	G_math_f_les *les;
	int i, j;
	int size =rows;
	float val;

	les = G_math_alloc_f_les(rows, G_MATH_NORMAL_LES);
	for (i = 0; i < size; i++)
	{
		val = 0.0;
		for (j = 0; j < size; j++)
		{
			if (j == i)
				les->A[i][j] = (float)(1.0/((((float)i + 1.0) + ((float)j))));
			if (j < i)
				les->A[i][j] = (float)(1.0/((((float)i + 1.0)
						+ ((float)j + 1.0) + 100)));
			if (j > i)
				les->A[i][j] = (float)(1.0/((((float)i + 1.0)
						+ ((float)j + 1.0) + 120)));

			val += les->A[i][j];
		}
		les->b[i] = val;
		les->x[i] = 0.5;
	}

	return les;
}

/* *************************************************************** */
/* create a sparse matrix with values ** Hilbert matrix ********** */
/* *************************************************************** */
G_math_les *create_sparse_symmetric_les(int rows)
{
	G_math_les *les;
	G_math_spvector *spvector;
	int i, j;
	double val;

	les = G_math_alloc_les(rows, G_MATH_SPARSE_LES);

	for (i = 0; i < rows; i++)
	{
		spvector = G_math_alloc_spvector(rows);
		val = 0;

		for (j = 0; j < rows; j++)
		{
			if (j == i)
			{
				spvector->values[j] = (double)(1.0/((((double)i + 1.0)
						+ ((double)j + 1.0))));
				spvector->index[j] = j;
			}
			else
			{
				spvector->values[j] = (double)(1.0/(((((double)i + 1.0)
						+ ((double)j + 1.0)) + 100)));
				spvector->index[j] = j;
			}

			val += spvector->values[j];
		}

		G_math_add_spvector_to_les(les, spvector, i);
		les->b[i] = val;
		les->x[i] = 0.5;
	}

	return les;
}

/* *************************************************************** */
void fill_d_vector_range_1(double *x, double a, int rows)
{
	int i = 0;

	for (i = 0; i < rows; i++)
        {
		x[i] = a*(double)i;
        }

}

/* *************************************************************** */
void fill_d_vector_range_2(double *x, double a, int rows)
{
	int i = 0, count = 0;

	for (i = rows - 1; i >= 0; i--)
	{
		x[i] = a*(double)count;
		count ++;
	}

}

/* *************************************************************** */
void fill_d_vector_scalar(double *x, double a, int rows)
{
	int i = 0;

	for (i = 0; i < rows; i++)
        {
		x[i] = a;
        }
}

/* *************************************************************** */
void fill_f_vector_range_1(float *x, float a, int rows)
{
	int i = 0;

	for (i = 0; i < rows; i++)
        {
		x[i] = a*(float)i;
                //printf("%f ", x[i]);
        }
}

/* *************************************************************** */
void fill_f_vector_range_2(float *x, float a, int rows)
{
	int i = 0, count = 0;

	for (i = rows - 1; i >= 0; i--)
	{
		x[i] = a*(float)count;
                //printf("%f ", x[i]);
		count ++;
	}

}

/* *************************************************************** */
void fill_f_vector_scalar(float *x, float a, int rows)
{
	int i = 0;

	for (i = 0; i < rows; i++)
        {
		x[i] = a;
                //printf("%f ", x[i]);
        }
}

/* *************************************************************** */
void fill_i_vector_range_1(int *x, int a, int rows)
{
	int i = 0;

	for (i = 0; i < rows; i++)
        {
		x[i] = a*i;
        }
}

/* *************************************************************** */
void fill_i_vector_range_2(int *x, int a, int rows)
{
	int i = 0, count = 0;

	for (i = rows - 1; i >= 0; i--)
	{
		x[i] = a*count;
		count ++;
	}

}

/* *************************************************************** */
void fill_i_vector_scalar(int *x, int a, int rows)
{
	int i = 0;

	for (i = 0; i < rows; i++)
        {
		x[i] = a;
        }
}

