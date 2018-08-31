
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2007
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      Unit tests for les creation
*
* COPYRIGHT:    (C) 2007 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/gmath.h>
#include <math.h>
#include "test_gmath_lib.h"

#define EPSILON 0.0000001

/* prototypes */
static int test_blas_level_3_double(void);
static int test_blas_level_3_float(void);


/* *************************************************************** */
/* Perfrome the blas level 3 unit tests ************************** */
/* *************************************************************** */
int unit_test_blas_level_3(void)
{
    int sum = 0;

    G_message(_("\n++ Running blas level 3 unit tests ++"));

    sum += test_blas_level_3_double();
    sum += test_blas_level_3_float();

    if (sum > 0)
	G_warning(_("\n-- blas level 3 unit tests failure --"));
    else
	G_message(_("\n-- blas level 3 unit tests finished successfully --"));

    return sum;
}

/* *************************************************************** */
/* ************** D O U B L E ************************************ */
/* *************************************************************** */
int test_blas_level_3_double(void)
{

    int sum = 0;
    int rows = TEST_NUM_ROWS;
    int cols = TEST_NUM_COLS;
    double **A, **B, **C, *x, *y, a = 0.0, b = 0.0, c = 0.0, d = 0.0;

    x = G_alloc_vector(cols);
    y = G_alloc_vector(rows);

    A = G_alloc_matrix(rows, cols);
    B = G_alloc_matrix(rows, cols);
    C = G_alloc_matrix(rows, cols);

    fill_d_vector_scalar(x, 1, cols);
    fill_d_vector_scalar(y, 0, rows);

    fill_d_vector_scalar(A[0], 1, rows*cols);
    fill_d_vector_scalar(B[0], 2, rows*cols);

#pragma omp parallel default(shared)
{
    G_math_d_aA_B(A, B, 1.0 , C, rows , cols );
    G_math_d_Ax(C, x, y, rows, cols);
}
    G_math_d_asum_norm(y, &a, rows);


    if(a != 3*rows*cols) {
    	G_message("Error in G_math_d_aA_B: %f != %f", a, (double)3*rows*cols);
	sum++;
    }
#pragma omp parallel default(shared)
{
    G_math_d_aA_B(A, B, -1.0 , C, rows , cols );
    G_math_d_Ax(C, x, y, rows, cols);
}
    G_math_d_asum_norm(y, &b, rows);


    if(b != rows*cols) {
    	G_message("Error in G_math_d_aA_B: %f != %f", b, (double)rows*cols);
	sum++;
    }
#pragma omp parallel default(shared)
{
    G_math_d_aA_B(A, B, 2.0 , C, rows , cols );
    G_math_d_Ax(C, x, y, rows, cols);
}
    G_math_d_asum_norm(y, &c, rows);


    if(c != 4*rows*cols) {
    	G_message("Error in G_math_d_aA_B: %f != %f", c, (double)4*rows*cols);
	sum++;
    }

    G_free_matrix(A);
    G_free_matrix(B);
    G_free_matrix(C);
    A = G_alloc_matrix(rows, cols);
    B = G_alloc_matrix(cols, rows);
    C = G_alloc_matrix(rows, rows);

    G_free_vector(x);
    G_free_vector(y);
    x = G_alloc_vector(rows);
    y = G_alloc_vector(rows);

    fill_d_vector_scalar(x, 1, rows);
    fill_d_vector_scalar(y, 0, rows);
    fill_d_vector_scalar(A[0], 1, rows*cols);
    fill_d_vector_scalar(B[0], 2, rows*cols);

#pragma omp parallel default(shared)
{
    G_math_d_AB(A, B, C, rows , cols , cols );
    G_math_d_Ax(C, x, y, rows, cols);
}
    G_math_d_asum_norm(y, &d, rows);


    if(d != 2*rows*cols*cols) {
    	G_message("Error in G_math_d_AB: %f != %f", d, (double)2*rows*cols*cols);
	sum++;
    }

    if(x)
      G_free_vector(x);
    if(y)
      G_free_vector(y);

    if(A)
      G_free_matrix(A);
    if(B)
      G_free_matrix(B);
    if(C)
      G_free_matrix(C);

    return sum;
}


/* *************************************************************** */
/* ************** F L O A T ************************************** */
/* *************************************************************** */
int test_blas_level_3_float(void)
{

    int sum = 0;
    int rows = TEST_NUM_ROWS;
    int cols = TEST_NUM_COLS;
    float **A, **B, **C, *x, *y, a = 0.0, b = 0.0, c = 0.0, d = 0.0;

    x = G_alloc_fvector(cols);
    y = G_alloc_fvector(rows);

    A = G_alloc_fmatrix(rows, cols);
    B = G_alloc_fmatrix(rows, cols);
    C = G_alloc_fmatrix(rows, cols);

    fill_f_vector_scalar(x, 1, cols);
    fill_f_vector_scalar(y, 0, rows);

    fill_f_vector_scalar(A[0], 1, rows*cols);
    fill_f_vector_scalar(B[0], 2, rows*cols);

#pragma omp parallel default(shared)
{
    G_math_f_aA_B(A, B, 1.0 , C, rows , cols );
    G_math_f_Ax(C, x, y, rows, cols);
}
    G_math_f_asum_norm(y, &a, rows);

    if(a != 3*rows*cols) {
    	G_message("Error in G_math_f_aA_B: %f != %f", a, (double)3*rows*cols);
	sum++;
    }
#pragma omp parallel default(shared)
{
    G_math_f_aA_B(A, B, -1.0 , C, rows , cols );
    G_math_f_Ax(C, x, y, rows, cols);
}
    G_math_f_asum_norm(y, &b, rows);

    if(b != rows*cols) {
    	G_message("Error in G_math_f_aA_B: %f != %f", b, (double)rows*cols);
	sum++;
    }
#pragma omp parallel default(shared)
{
    G_math_f_aA_B(A, B, 2.0 , C, rows , cols );
    G_math_f_Ax(C, x, y, rows, cols);
}
    G_math_f_asum_norm(y, &c, rows);

    if(c != 4*rows*cols) {
    	G_message("Error in G_math_f_aA_B: %f != %f", c, (double)4*rows*cols);
	sum++;
    }

    G_free_fmatrix(A);
    G_free_fmatrix(B);
    G_free_fmatrix(C);
    A = G_alloc_fmatrix(rows, cols);
    B = G_alloc_fmatrix(cols, rows);
    C = G_alloc_fmatrix(rows, rows);

    G_free_fvector(x);
    G_free_fvector(y);
    x = G_alloc_fvector(rows);
    y = G_alloc_fvector(rows);

    fill_f_vector_scalar(x, 1, rows);
    fill_f_vector_scalar(y, 0, rows);
    fill_f_vector_scalar(A[0], 1, rows*cols);
    fill_f_vector_scalar(B[0], 2, rows*cols);

#pragma omp parallel default(shared)
{
    G_math_f_AB(A, B, C, rows , cols , cols );
    G_math_f_Ax(C, x, y, rows, cols);
}
    G_math_f_asum_norm(y, &d, rows);


    if(d != 2*rows*cols*cols) {
    	G_message("Error in G_math_f_AB: %f != %f", d, (double)2*rows*cols*cols);
	sum++;
    }

    if(x)
      G_free_fvector(x);
    if(y)
      G_free_fvector(y);

    if(A)
      G_free_fmatrix(A);
    if(B)
      G_free_fmatrix(B);
    if(C)
      G_free_fmatrix(C);

    return sum;
}
