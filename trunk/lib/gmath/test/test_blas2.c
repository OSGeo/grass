
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      Unit tests for les creation
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
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

#define EPSILON 0.00001

/* prototypes */
static int test_blas_level_2_double(void);
static int test_blas_level_2_float(void);


/* *************************************************************** */
/* Perfrome the blas level 2 unit tests ************************** */
/* *************************************************************** */
int unit_test_blas_level_2(void)
{
    int sum = 0;

    G_message(_("\n++ Running blas level 2 unit tests ++"));

    sum += test_blas_level_2_double();
    sum += test_blas_level_2_float();

    if (sum > 0)
	G_warning(_("\n-- blas level 2 unit tests failure --"));
    else
	G_message(_("\n-- blas level 2 unit tests finished successfully --"));

    return sum;
}

/* *************************************************************** */
/* ************** D O U B L E ************************************ */
/* *************************************************************** */
int test_blas_level_2_double(void)
{

    int sum = 0;
    int rows = TEST_NUM_ROWS;
    double **A, **B, **C, *x, *y, *z, a = 0.0, b = 0.0, c = 0.0, d = 0.0, e = 0.0, f = 0.0, g = 0.0, h = 0.0, i = 0.0, j =0.0;

    G_math_les *les;
    les = create_normal_unsymmetric_les(rows);
    G_math_les *sples;
    sples = create_sparse_symmetric_les(rows);
    G_math_les *bles;
    bles = create_symmetric_band_les(rows);

    x = G_alloc_vector(rows);
    y = G_alloc_vector(rows);
    z = G_alloc_vector(rows);

    A = G_alloc_matrix(rows, rows);
    B = G_alloc_matrix(rows, rows);
    C = G_alloc_matrix(rows, rows);

    fill_d_vector_scalar(x, 1, rows);
    fill_d_vector_scalar(y, 0, rows);


#pragma omp parallel default(shared)
{
    G_math_Ax_sparse(sples->Asp, x, sples->b, rows);
}
    G_math_d_asum_norm(sples->b, &a, rows);
#pragma omp parallel default(shared)
{
    G_math_Ax_sband(bles->A, x, bles->b, rows, rows);
}
    G_math_d_asum_norm(bles->b, &j, rows);
#pragma omp parallel default(shared)
{
    G_math_d_Ax(les->A, x, z, rows, rows);
}
    G_math_d_asum_norm(z, &b, rows);
#pragma omp parallel default(shared)
{
    G_math_d_aAx_by(les->A, x, y, 1.0, 1.0, z, rows, rows);
}
    G_math_d_asum_norm(z, &c, rows);
#pragma omp parallel default(shared)
{
    G_math_d_aAx_by(les->A, x, y, -1.0, 1.0, z, rows, rows);
}
    G_math_d_asum_norm(z, &d, rows);
#pragma omp parallel default(shared)
{
    G_math_d_aAx_by(les->A, x, y, 1.0, 0.0, z, rows, rows);
}
    G_math_d_asum_norm(z, &e, rows);
#pragma omp parallel default(shared)
{
    G_math_d_aAx_by(les->A, x, y, -1.0, -1.0, z, rows, rows);
}
    G_math_d_asum_norm(z, &f, rows);
#pragma omp parallel default(shared)
{
    G_math_d_x_dyad_y(x, x, A, rows, rows);
    G_math_d_Ax(A, x, z, rows, rows);
}
    G_math_d_asum_norm(z, &g, rows);
#pragma omp parallel default(shared)
{
    G_math_d_x_dyad_y(x, x, C, rows, rows);
    G_math_d_Ax(A, x, z, rows, rows);
}
    G_math_d_asum_norm(z, &h, rows);

    G_math_d_asum_norm(les->b, &i, rows);

    if(a - i > EPSILON) {
    	G_message("Error in G_math_Ax_sparse: %f != %f", i, a);
	sum++;
    }

    if(j - i > EPSILON) {
    	G_message("Error in G_math_Ax_sband: %f != %f", i, j);
	sum++;
    }

    if(b - i > EPSILON) {
    	G_message("Error in G_math_d_Ax: %f != %f", i, b);
	sum++;
    }

    if(c - i > EPSILON) {
    	G_message("Error in G_math_aAx_by: %f != %f", i, c);
	sum++;
    }

    if(d - i > EPSILON) {
    	G_message("Error in G_math_aAx_by: %f != %f", i, d);
	sum++;
    }

    if(e - i > EPSILON) {
    	G_message("Error in G_math_aAx_by: %f != %f", i, e);
	sum++;
    }

    if(f - i > EPSILON) {
    	G_message("Error in G_math_aAx_by: %f != %f", i, f);
	sum++;
    }

    if(g - (double)rows*rows > EPSILON) {
    	G_message("Error in G_math_d_x_dyad_y: %f != %f", (double)rows*rows, g);
	sum++;
    }

    if(h - (double)rows*rows > EPSILON) {
    	G_message("Error in G_math_d_x_dyad_y: %f != %f", (double)rows*rows, h);
	sum++;
    }

    if(x)
      G_free_vector(x);
    if(y)
      G_free_vector(y);
    if(z)
    G_free_vector(z);

    G_math_free_les(les);
    G_math_free_les(bles);
    G_math_free_les(sples);

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
int test_blas_level_2_float(void)
{

    int sum = 0;
    int rows =TEST_NUM_ROWS;
    float **A, **B, **C, *x, *y, *z, b = 0.0, c = 0.0, d = 0.0, e = 0.0, f = 0.0, g = 0.0, h = 0.0, i = 0.0;

    G_math_f_les *les;
    les = create_normal_unsymmetric_f_les(rows);

    x = G_alloc_fvector(rows);
    y = G_alloc_fvector(rows);
    z = G_alloc_fvector(rows);

    A = G_alloc_fmatrix(rows, rows);
    B = G_alloc_fmatrix(rows, rows);
    C = G_alloc_fmatrix(rows, rows);

    fill_f_vector_scalar(x, 1, rows);
    fill_f_vector_scalar(y, 0, rows);


#pragma omp parallel default(shared)
{
    G_math_f_Ax(les->A, x, z, rows, rows);
}
    G_math_f_asum_norm(z, &b, rows);
#pragma omp parallel default(shared)
{
    G_math_f_aAx_by(les->A, x, y, 1.0, 1.0, z, rows, rows);
}
    G_math_f_asum_norm(z, &c, rows);
#pragma omp parallel default(shared)
{
    G_math_f_aAx_by(les->A, x, y, -1.0, 1.0, z, rows, rows);
}
    G_math_f_asum_norm(z, &d, rows);
#pragma omp parallel default(shared)
{
    G_math_f_aAx_by(les->A, x, y, 1.0, 0.0, z, rows, rows);
}
    G_math_f_asum_norm(z, &e, rows);
#pragma omp parallel default(shared)
{
    G_math_f_aAx_by(les->A, x, y, -1.0, -1.0, z, rows, rows);
}
    G_math_f_asum_norm(z, &f, rows);
#pragma omp parallel default(shared)
{
    G_math_f_x_dyad_y(x, x, A, rows, rows);
    G_math_f_Ax(A, x, z, rows, rows);
}
    G_math_f_asum_norm(z, &g, rows);
#pragma omp parallel default(shared)
{
    G_math_f_x_dyad_y(x, x, C, rows, rows);
    G_math_f_Ax(A, x, z, rows, rows);
}
    G_math_f_asum_norm(z, &h, rows);



    G_math_f_asum_norm(les->b, &i, rows);

    if(b - i > EPSILON) {
    	G_message("Error in G_math_f_Ax: %f != %f", i, b);
	sum++;
    }

    if(c - i > EPSILON) {
    	G_message("Error in G_math_f_aAx_by: %f != %f", i, c);
	sum++;
    }

    if(d - i > EPSILON) {
    	G_message("Error in G_math_f_aAx_by: %f != %f", i, d);
	sum++;
    }

    if(e - i > EPSILON) {
    	G_message("Error in G_math_f_aAx_by: %f != %f", i, e);
	sum++;
    }

    if(f - i > EPSILON) {
    	G_message("Error in G_math_f_aAx_by: %f != %f", i, f);
	sum++;
    }

    if(g - (float)rows*rows > EPSILON) {
    	G_message("Error in G_math_f_x_dyad_y: %f != %f", (float)rows*rows, g);
	sum++;
    }

    if(h - (float)rows*rows > EPSILON) {
    	G_message("Error in G_math_f_x_dyad_y: %f != %f", (float)rows*rows, h);
	sum++;
    }

    if(x)
      G_free_fvector(x);
    if(y)
      G_free_fvector(y);
    if(z)
    G_free_fvector(z);

    G_math_free_f_les(les);

    if(A)
      G_free_fmatrix(A);
    if(B)
      G_free_fmatrix(B);
    if(C)
      G_free_fmatrix(C);

    return sum;
}
