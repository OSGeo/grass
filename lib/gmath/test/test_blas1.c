
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

#define EPSILON 0.000001


/* prototypes */
static int test_blas_level_1_double(void);
static int test_blas_level_1_float(void);
static int test_blas_level_1_int(void);


/* *************************************************************** */
/* Perfrome the blas level 1 unit tests ************************** */
/* *************************************************************** */
int unit_test_blas_level_1(void)
{
    int sum = 0;

    G_message(_("\n++ Running blas level 1 unit tests ++"));

    sum += test_blas_level_1_double();
    sum += test_blas_level_1_float();
    sum += test_blas_level_1_int();

    if (sum > 0)
	G_warning(_("\n-- blas level 1 unit tests failure --"));
    else
	G_message(_("\n-- blas level 1 unit tests finished successfully --"));

    return sum;
}


/* *************************************************************** */
/* ************** D O U B L E ************************************ */
/* *************************************************************** */
int test_blas_level_1_double(void)
{

    int sum = 0;
    int rows = 10000;
    double *x, *y, *z, a = 0.0, b = 0.0, c = 0.0, d = 0.0, e = 0.0;

    x = G_alloc_vector(rows);
    y = G_alloc_vector(rows);
    z = G_alloc_vector(rows);

    fill_d_vector_scalar(x, 1, rows);
    fill_d_vector_scalar(y, 2, rows);

/*test the grass implementation*/
    G_math_d_x_dot_y(x, y, &a, rows);
    G_math_d_asum_norm(x, &b, rows);
    G_math_d_euclid_norm(x, &c, rows);


    if(a != 2.0*rows) {
    	G_message("Error in G_math_d_x_dot_y %f != %f", 2.0*rows, a);
	sum++;
    }

    if(b != rows) {
    	G_message("Error in G_math_d_asum_norm");
	sum++;
    }

    if(c != sqrt(rows)) {
    	G_message("Error in G_math_d_euclid_norm");
	sum++;
    }
    
    /*test the ATLAS implementation*/
    a = G_math_dnrm2(x, rows);
    b = G_math_dasum(x, rows);
    c = G_math_ddot(x, y, rows);

    if(a != sqrt(rows)) {
    	G_message("Error in G_math_dnrm2 %f != %f", sqrt(rows), a);
	sum++;
    }

    if(b != rows) {
    	G_message("Error in G_math_dasum %f != %f", 2.0*rows, b);
	sum++;
    }

    if(c != 2.0*rows) {
    	G_message("Error in G_math_ddot %f != %f", 2.0*rows, c);
	sum++;
    }

    fill_d_vector_range_1(x, 1.0, rows);
    fill_d_vector_range_2(y, 1.0, rows);

    /*grass function*/
    G_math_d_max_norm(x, &a, rows);
    /*atlas function*/
    b = G_math_idamax(x, rows);

    if(a != 1.0*(rows - 1)) {
    	G_message("Error in G_math_d_max_norm: %f != %f", (double)1.0*(rows - 1), a);
	sum++;
    }

    if(b != 1.0*(rows - 1)) {
    	G_message("Error in G_math_idamax: %f != %f", (double)1.0*(rows - 1), b);
	sum++;
    }

#pragma omp parallel default(shared)
{
    G_math_d_ax_by(x, y, z, 1.0, 1.0, rows);
}
    G_math_d_asum_norm(z, &a, rows);

#pragma omp parallel default(shared)
{
    G_math_d_ax_by(x, y, z, 1.0, -1.0, rows);
}
    G_math_d_asum_norm(z, &b, rows);

#pragma omp parallel default(shared)
{
    G_math_d_ax_by(x, y, z, 2.0, 1.0, rows);
}
    G_math_d_asum_norm(z, &c, rows);


    if(a != 1.0*(rows - 1)* rows) {
    	G_message("Error in G_math_d_ax_by: %f != %f", (double)1.0*(rows - 1)* rows, a);
	sum++;
    }

    if(b != 5.0*(rows)*(rows/10)) {
    	G_message("Error in G_math_d_ax_by: %f != %f", (double)5.0*(rows)*(rows/10), b);
	sum++;
    }

    if(c != 149985000) {
    	G_message("Error in G_math_d_ax_by: 149985000 != %f", c);
	sum++;
    }


#pragma omp parallel  default(shared)
{
    /*scale x with 1*/
    G_math_d_ax_by(x, z, z, 1.0, 0.0, rows);
}
    G_math_d_asum_norm(x, &a, rows);
    G_math_d_asum_norm(z, &b, rows);
    /*scale x with -1*/
#pragma omp parallel  default(shared)
{
    G_math_d_ax_by(x, z, z, -1.0, 0.0, rows);
}
    G_math_d_asum_norm(z, &c, rows);

    /*ATLAS implementation*/
    G_math_dscal(x, 1.0, rows);
    G_math_d_asum_norm(x, &d, rows);

    /*ATLAS implementation*/
    fill_d_vector_range_1(x, 1.0, rows);
    fill_d_vector_scalar(z, 0.0, rows);
    G_math_daxpy(x, z, 1.0, rows);
    G_math_d_asum_norm(z, &e, rows);


    if(a != 49995000 || a != b || b != c) {
    	G_message("Error in G_math_d_ax: 49995000 != %f", a);
	sum++;
    }

    if(49995000 != d) {
    	G_message("Error in G_math_dscal: 49995000 != %f", d);
	sum++;
    }

    if(49995000 != e) {
    	G_message("Error in G_math_daxpy: 49995000 != %f", e);
	sum++;
    }

    fill_d_vector_scalar(z, 0, rows);

    G_math_d_copy(x, z, rows);
    G_math_d_asum_norm(x, &a, rows);
    G_math_dcopy(x, z, rows);
    G_math_d_asum_norm(x, &b, rows);

    if(a != 49995000) {
    	G_message("Error in G_math_d_copy: 49995000 != %f", a);
	sum++;
    }

    if(b != 49995000) {
    	G_message("Error in G_math_dcopy: 49995000 != %f", a);
	sum++;
    }

    G_free_vector(x);
    G_free_vector(y);
    G_free_vector(z);

    return sum;
}


/* *************************************************************** */
/* ************** F L O A T ************************************** */
/* *************************************************************** */
int test_blas_level_1_float(void)
{

    int sum = 0;
    int rows = 1000;
    float *x, *y, *z, a = 0.0, b = 0.0, c = 0.0, d = 0.0, e = 0.0;

    x = G_alloc_fvector(rows);
    y = G_alloc_fvector(rows);
    z = G_alloc_fvector(rows);

    fill_f_vector_scalar(x, 1, rows);
    fill_f_vector_scalar(y, 2, rows);

/*test the grass implementation*/
    G_math_f_x_dot_y(x, y, &a, rows);
    G_math_f_asum_norm(x, &b, rows);
    G_math_f_euclid_norm(x, &c, rows);


    if(a != 2.0*rows) {
    	G_message("Error in G_math_f_x_dot_y %f != %f", 2.0*rows, a);
	sum++;
    }

    if(b != rows) {
    	G_message("Error in G_math_f_asum_norm");
	sum++;
    }

    if(fabs(c - (float)sqrt(rows)) > EPSILON) {
    	G_message("Error in G_math_f_euclid_norm");
	sum++;
    }
    
    /*test the ATLAS implementation*/
    a = G_math_snrm2(x, rows);
    b = G_math_sasum(x, rows);
    c = G_math_sdot(x, y, rows);

    if(fabs(a - sqrt(rows)) > EPSILON) {
    	G_message("Error in G_math_snrm2 %f != %f", sqrt(rows), a);
	sum++;
    }

    if(b != rows) {
    	G_message("Error in G_math_sasum %f != %f", 2.0*rows, b);
	sum++;
    }

    if(c != 2.0*rows) {
    	G_message("Error in G_math_sdot %f != %f", 2.0*rows, c);
	sum++;
    }

    fill_f_vector_range_1(x, 1.0, rows);
    fill_f_vector_range_2(y, 1.0, rows);

    /*grass function*/
    G_math_f_max_norm(x, &a, rows);
    /*atlas function*/
    b = G_math_isamax(x, rows);

    if(a != 1.0*(rows - 1)) {
    	G_message("Error in G_math_f_max_norm: %f != %f", (float)1.0*(rows - 1), a);
	sum++;
    }

    if(b != 1.0*(rows - 1)) {
    	G_message("Error in G_math_isamax: %f != %f", (float)1.0*(rows - 1), b);
	sum++;
    }

#pragma omp parallel  default(shared)
{
    G_math_f_ax_by(x, y, z, 1.0, 1.0, rows);
}
    G_math_f_asum_norm(z, &a, rows);
#pragma omp parallel  default(shared)
{
    G_math_f_ax_by(x, y, z, 1.0, -1.0, rows);
}
G_math_f_asum_norm(z, &b, rows);
#pragma omp parallel  default(shared)
{
    G_math_f_ax_by(x, y, z, 2.0, 1.0, rows);
}
    G_math_f_asum_norm(z, &c, rows);


    if(fabs(a - 1.0*(rows - 1)* rows) > EPSILON) {
    	G_message("Error in G_math_f_ax_by 1: %f != %f", (float)1.0*(rows - 1)* rows, a);
	sum++;
    }

    if(fabs(b - 5.0*(rows)*(rows/10)) > EPSILON) {
    	G_message("Error in G_math_f_ax_by 2: %f != %f", (float)5.0*(rows)*(rows/10), b);
	sum++;
    }

    if(fabs(c - 1498500) > EPSILON) {
    	G_message("Error in G_math_f_ax_by 3: 14998500 != %f", c);
	sum++;
    }


#pragma omp parallel default(shared)
{
    /*scale x with 1*/
    G_math_f_ax_by(x, z, z, 1.0, 0.0, rows);
}
    G_math_f_asum_norm(x, &a, rows);
    G_math_f_asum_norm(z, &b, rows);
    /*scale x with -1*/
#pragma omp parallel default(shared)
{
    G_math_f_ax_by(x, z, z, -1.0, 0.0, rows);
}
    G_math_f_asum_norm(z, &c, rows);

    /*ATLAS implementation*/
    G_math_sscal(x, 1.0, rows);
    G_math_f_asum_norm(x, &d, rows);

    /*ATLAS implementation*/
    fill_f_vector_range_1(x, 1.0, rows);
    fill_f_vector_scalar(z, 0.0, rows);
    G_math_saxpy(x, z, 1.0, rows);
    G_math_f_asum_norm(z, &e, rows);


    if(fabs(a - 499500) > EPSILON) {
    	G_message("Error in G_math_f_ax_by 4: 4999500 != %f", a);
	sum++;
    }
    if(fabs(b - 499500) > EPSILON) {
    	G_message("Error in G_math_f_ax_by 4: 4999500 != %f", b);
	sum++;
    }
    if(fabs(c - 499500) > EPSILON) {
    	G_message("Error in G_math_f_ax_by 4: 4999500 != %f", c);
	sum++;
    }
    if(fabs(d - 499500) > EPSILON) {
    	G_message("Error in G_math_sscal: 4999500 != %f", d);
	sum++;
    }

    if(fabs(e - 499500) > EPSILON) {
    	G_message("Error in G_math_saxpy: 4999500 != %f", e);
	sum++;
    }
    
    fill_f_vector_range_1(x, 1.0, rows);
    fill_f_vector_scalar(z, 0, rows);

    G_math_f_copy(x, z, rows);
    G_math_f_asum_norm(x, &a, rows);
    G_math_scopy(x, z, rows);
    G_math_f_asum_norm(x, &b, rows);

    if(fabs(a - 499500) > EPSILON) {
    	G_message("Error in G_math_f_copy: 4999500 != %f", a);
	sum++;
    }

    if(fabs(b - 499500) > EPSILON) {
    	G_message("Error in G_math_scopy: 4999500 != %f", b);
	sum++;
    }

    G_free_fvector(x);
    G_free_fvector(y);
    G_free_fvector(z);

    return sum;
}


/* *************************************************************** */
/* ************** I N T E G E R ********************************** */
/* *************************************************************** */
int test_blas_level_1_int(void)
{

    int sum = 0;
    int rows = 10000;
    int *x, *y, *z, max;
    double a, b, c;

    x = G_alloc_ivector(rows);
    y = G_alloc_ivector(rows);
    z = G_alloc_ivector(rows);

    fill_i_vector_scalar(x, 1, rows);
    fill_i_vector_scalar(y, 2, rows);


    G_math_i_x_dot_y(x, y, &a, rows);
    G_math_i_asum_norm(x, &b, rows);
    G_math_i_euclid_norm(x, &c, rows);


    if(a != 2*rows) {
    	G_message("Error in G_math_i_x_dot_y");
	sum++;
    }

    if(b != rows) {
    	G_message("Error in G_math_i_asum_norm");
	sum++;
    }

    if(c != sqrt((double)rows)) {
    	G_message("Error in G_math_i_euclid_norm");
	sum++;
    }

    fill_i_vector_range_1(x, 1, rows);
    fill_i_vector_range_2(y, 1, rows);

    G_math_i_max_norm(x, &max, rows);

    if(max != (rows - 1)) {
    	G_message("Error in G_math_i_max_norm: %i != %i", (rows - 1), max);
	sum++;
    }

#pragma omp parallel default(shared)
{
    G_math_i_ax_by(x, y, z, 1, 1, rows);
}
    G_math_i_asum_norm(z, &a, rows);
#pragma omp parallel default(shared)
{
    G_math_i_ax_by(x, y, z, 1, -1, rows);
}
    G_math_i_asum_norm(z, &b, rows);
#pragma omp parallel default(shared)
{
    G_math_i_ax_by(x, y, z, 2, 1, rows);
}
    G_math_i_asum_norm(z, &c, rows);


    if(a != 1.0*(rows - 1)* rows) {
    	G_message("Error in G_math_i_ax_by: %f != %f", 1.0*(rows - 1)* rows, a);
	sum++;
    }

    if(b != 5.0*(rows)*(rows/10)) {
    	G_message("Error in G_math_i_ax_by: %f != %f", 5.0*(rows)*(rows/10), b);
	sum++;
    }

    if(c != 149985000) {
    	G_message("Error in G_math_i_ax_by: 149985000 != %f", c);
	sum++;
    }


#pragma omp parallel default(shared)
{
    /*scale x with 1*/
    G_math_i_ax_by(x, z, z, 1, 0, rows);
}
    G_math_i_asum_norm(x, &a, rows);
    G_math_i_asum_norm(z, &b, rows);
    
    /*scale a with -1*/
#pragma omp parallel default(shared)
{
    G_math_i_ax_by(x, z, z, -1, 0, rows);
}
    G_math_i_asum_norm(z, &c, rows);


    if(a != 49995000 || a != b || b != c) {
    	G_message("Error in G_math_i_ax_by: 49995000 != %f", a);
	sum++;
    }

    return sum;
}
