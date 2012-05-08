
/*****************************************************************************
*
* MODULE:       Grass numerical math interface
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> googlemail <dot> com
*               
* PURPOSE:      grass blas implementation
* 		part of the gmath library
*               
* COPYRIGHT:    (C) 2010 by the GRASS Development Team
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
#include <stdlib.h>
#include <grass/gmath.h>
#include <grass/gis.h>

#define EPSILON 0.00000000000000001


/*!
 * \brief Compute the matrix - vector product  
 * of matrix A and vector x.
 *
 * This function is multi-threaded with OpenMP and can be called within a parallel OpenMP region.
 *
 * y = A * x
 *
 *
 * \param A (double ** )
 * \param x (double *)
 * \param y (double *) 
 * \param rows (int)
 * \param cols (int)
 * \return (void)
 *
 * */
void G_math_d_Ax(double **A, double *x, double *y, int rows, int cols)
{
    int i, j;

    double tmp;

#pragma omp for schedule (static) private(i, j, tmp)
    for (i = 0; i < rows; i++) {
	tmp = 0;
	for (j = cols - 1; j >= 0; j--) {
	    tmp += A[i][j] * x[j];
	}
	y[i] = tmp;
    }
    return;
}

/*!
 * \brief Compute the matrix - vector product  
 * of matrix A and vector x.
 *
 * This function is multi-threaded with OpenMP and can be called within a parallel OpenMP region.
 *
 * y = A * x
 *
 *
 * \param A (float ** )
 * \param x (float *)
 * \param y (float *) 
 * \param rows (int)
 * \param cols (int)
 * \return (void)
 *
 * */
void G_math_f_Ax(float **A, float *x, float *y, int rows, int cols)
{
    int i, j;

    float tmp;

#pragma omp for schedule (static) private(i, j, tmp)
    for (i = 0; i < rows; i++) {
	tmp = 0;
	for (j = cols - 1; j >= 0; j--) {
	    tmp += A[i][j] * x[j];
	}
	y[i] = tmp;
    }
    return;
}

/*!
 * \brief Compute the dyadic product of two vectors. 
 * The result is stored in the matrix A.
 *
 * This function is multi-threaded with OpenMP and can be called within a parallel OpenMP region.
 *
 * A = x * y^T
 *
 *
 * \param x (double *)
 * \param y (double *) 
 * \param A (float **)  -- matrix of size rows*cols
 * \param rows (int) -- length of vector x
 * \param cols (int) -- lengt of vector y
 * \return (void)
 *
 * */
void G_math_d_x_dyad_y(double *x, double *y, double **A, int rows, int cols)
{
    int i, j;

#pragma omp for schedule (static) private(i, j)
    for (i = 0; i < rows; i++) {
	for (j = cols - 1; j >= 0; j--) {
	    A[i][j] = x[i] * y[j];
	}
    }
    return;
}

/*!
 * \brief Compute the dyadic product of two vectors. 
 * The result is stored in the matrix A.
 *
 * This function is multi-threaded with OpenMP and can be called within a parallel OpenMP region.
 *
 * A = x * y^T
 *
 *
 * \param x (float *)
 * \param y (float *) 
 * \param A (float **=  -- matrix of size rows*cols 
 * \param rows (int) -- length of vector x
 * \param cols (int) -- lengt of vector y
 * \return (void)
 *
 * */
void G_math_f_x_dyad_y(float *x, float *y, float **A, int rows, int cols)
{
    int i, j;

#pragma omp for schedule (static) private(i, j)
    for (i = 0; i < rows; i++) {
	for (j = cols - 1; j >= 0; j--) {
	    A[i][j] = x[i] * y[j];
	}
    }
    return;
}

/*!
 * \brief Compute the scaled matrix - vector product  
 * of matrix double **A and vector x and y.
 *
 * z = a * A * x + b * y
 *
 * This function is multi-threaded with OpenMP and can be called within a parallel OpenMP region.
 *
 *
 * \param A (double **) 
 * \param x (double *)
 * \param y (double *) 
 * \param a (double)
 * \param b (double)
 * \param z (double *) 
 * \param rows (int)
 * \param cols (int)
 * \return (void)
 *
 * */

void G_math_d_aAx_by(double **A, double *x, double *y, double a, double b,
		     double *z, int rows, int cols)
{
    int i, j;

    double tmp;

    /*catch specific cases */
    if (a == b) {
#pragma omp for schedule (static) private(i, j, tmp)
	for (i = 0; i < rows; i++) {
	    tmp = 0;
	    for (j = cols - 1; j >= 0; j--) {
		tmp += A[i][j] * x[j] + y[j];
	    }
	    z[i] = a * tmp;
	}
    }
    else if (b == -1.0) {
#pragma omp for schedule (static) private(i, j, tmp)
	for (i = 0; i < rows; i++) {
	    tmp = 0;
	    for (j = cols - 1; j >= 0; j--) {
		tmp += a * A[i][j] * x[j] - y[j];
	    }
	    z[i] = tmp;
	}
    }
    else if (b == 0.0) {
#pragma omp for schedule (static) private(i, j, tmp)
	for (i = 0; i < rows; i++) {
	    tmp = 0;
	    for (j = cols - 1; j >= 0; j--) {
		tmp += A[i][j] * x[j];
	    }
	    z[i] = a * tmp;
	}
    }
    else if (a == -1.0) {
#pragma omp for schedule (static) private(i, j, tmp)
	for (i = 0; i < rows; i++) {
	    tmp = 0;
	    for (j = cols - 1; j >= 0; j--) {
		tmp += b * y[j] - A[i][j] * x[j];
	    }
	    z[i] = tmp;
	}
    }
    else {
#pragma omp for schedule (static) private(i, j, tmp)
	for (i = 0; i < rows; i++) {
	    tmp = 0;
	    for (j = cols - 1; j >= 0; j--) {
		tmp += a * A[i][j] * x[j] + b * y[j];
	    }
	    z[i] = tmp;
	}
    }
    return;
}

/*!
 * \brief Compute the scaled matrix - vector product  
 * of matrix A and vectors x and y.
 *
 * z = a * A * x + b * y
 *
 * This function is multi-threaded with OpenMP and can be called within a parallel OpenMP region.
 *
 *
 * \param A (float **) 
 * \param x (float *)
 * \param y (float *) 
 * \param a (float)
 * \param b (float)
 * \param z (float *) 
 * \param rows (int)
 * \param cols (int)
 * \return (void)
 *
 * */

void G_math_f_aAx_by(float **A, float *x, float *y, float a, float b,
		     float *z, int rows, int cols)
{
    int i, j;

    float tmp;

    /*catch specific cases */
    if (a == b) {
#pragma omp for schedule (static) private(i, j, tmp)
	for (i = 0; i < rows; i++) {
	    tmp = 0;
	    for (j = cols - 1; j >= 0; j--) {
		tmp += A[i][j] * x[j] + y[j];
	    }
	    z[i] = a * tmp;
	}
    }
    else if (b == -1.0) {
#pragma omp for schedule (static) private(i, j, tmp)
	for (i = 0; i < rows; i++) {
	    tmp = 0;
	    for (j = cols - 1; j >= 0; j--) {
		tmp += a * A[i][j] * x[j] - y[j];
	    }
	    z[i] = tmp;
	}
    }
    else if (b == 0.0) {
#pragma omp for schedule (static) private(i, j, tmp)
	for (i = 0; i < rows; i++) {
	    tmp = 0;
	    for (j = cols - 1; j >= 0; j--) {
		tmp += A[i][j] * x[j];
	    }
	    z[i] = a * tmp;
	}
    }
    else if (a == -1.0) {
#pragma omp for schedule (static) private(i, j, tmp)
	for (i = 0; i < rows; i++) {
	    tmp = 0;
	    for (j = cols - 1; j >= 0; j--) {
		tmp += b * y[j] - A[i][j] * x[j];
	    }
	    z[i] = tmp;
	}
    }
    else {
#pragma omp for schedule (static) private(i, j, tmp)
	for (i = 0; i < rows; i++) {
	    tmp = 0;
	    for (j = cols - 1; j >= 0; j--) {
		tmp += a * A[i][j] * x[j] + b * y[j];
	    }
	    z[i] = tmp;
	}
    }
    return;
}



/*!
 * \fn int G_math_d_A_T(double **A, int rows)
 *
 * \brief Compute the transposition of matrix A.
 * Matrix A will be overwritten.
 *
 * This function is multi-threaded with OpenMP and can be called within a parallel OpenMP region.
 *
 * Returns 0.
 *
 * \param A (double **)
 * \param rows (int)
 * \return int
 */

int G_math_d_A_T(double **A, int rows)
{
    int i, j;

    double tmp;

#pragma omp for schedule (static) private(i, j, tmp)
    for (i = 0; i < rows; i++)
	for (j = 0; j < i; j++) {
	    tmp = A[i][j];

	    A[i][j] = A[j][i];
	    A[j][i] = tmp;
	}

    return 0;
}

/*!
 * \fn int G_math_f_A_T(float **A, int rows)
 *
 * \brief Compute the transposition of matrix A.
 * Matrix A will be overwritten.
 *
 * This function is multi-threaded with OpenMP and can be called within a parallel OpenMP region.
 *
 * Returns 0.
 *
 * \param A (float **)
 * \param rows (int)
 * \return int
 */

int G_math_f_A_T(float **A, int rows)
{
    int i, j;

    float tmp;

#pragma omp for schedule (static) private(i, j, tmp)
    for (i = 0; i < rows; i++)
	for (j = 0; j < i; j++) {
	    tmp = A[i][j];

	    A[i][j] = A[j][i];
	    A[j][i] = tmp;
	}

    return 0;
}
