
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:	Unit tests for arrays
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
#include <grass/N_pde.h>
#include "test_gpde_lib.h"

/* prototypes */
static int fill_array_2d(N_array_2d * a);
static int fill_array_2d_null(N_array_2d * a);
static int compare_array_2d(N_array_2d * a, N_array_2d * b);
static int fill_array_3d(N_array_3d * a);
static int fill_array_3d_null(N_array_3d * a);
static int compare_array_3d(N_array_3d * a, N_array_3d * b);
static int test_array_2d(void);
static int test_array_3d(void);
static int io_bench_2d(void);
static int io_bench_3d(void);

/* ************************************************************************* */
/* Performe the array unit tests ******************************************* */
/* ************************************************************************* */
int unit_test_arrays(void)
{
    int sum = 0;

    io_bench_2d();
    return sum;

    G_message("\n++ Running array unit tests ++");

    G_message("\t 1. testing 2d arrays");
    sum += test_array_2d();

    G_message("\t 2. testing 3d arrays");
    sum += test_array_3d();

    if (sum > 0)
	G_warning("\n-- Array unit tests failure --");
    else
	G_message("\n-- Array unit tests finished successfully --");

    return sum;
}

/* ************************************************************************* */
/* Fill an 2d array with valid data **************************************** */
/* ************************************************************************* */
int fill_array_2d(N_array_2d * a)
{
    int rows, cols, type;
    int i, j, res = 0;

    rows = a->rows;
    cols = a->cols;
    type = N_get_array_2d_type(a);

#pragma omp parallel for private (i, j) shared (cols, rows, type, a) reduction(+:res)
    for (j = 0; j < rows; j++) {
	for (i = 0; i < cols; i++) {
	    if (type == CELL_TYPE) {
		N_put_array_2d_c_value(a, i, j, (CELL) i * (CELL) j);
		if (N_get_array_2d_c_value(a, i, j) != (CELL) i * (CELL) j)
		    res++;
	    }
	    if (type == FCELL_TYPE) {
		N_put_array_2d_f_value(a, i, j, (FCELL) i * (FCELL) j);
		if (N_get_array_2d_f_value(a, i, j) != (FCELL) i * (FCELL) j)
		    res++;
	    }
	    if (type == DCELL_TYPE) {
		N_put_array_2d_d_value(a, i, j, (DCELL) i * (DCELL) j);
		if (N_get_array_2d_d_value(a, i, j) != (DCELL) i * (DCELL) j)
		    res++;
	    }
	}
    }

    return res;
}

/* ************************************************************************* */
/* Fill an 2d array with null values *************************************** */
/* ************************************************************************* */
int fill_array_2d_null(N_array_2d * a)
{
    int rows, cols;
    int i, j, res = 0;

    cols = a->cols;
    rows = a->rows;

#pragma omp parallel for private (i, j) shared (rows, cols, a) reduction(+:res)
    for (j = 0; j < rows; j++) {
	for (i = 0; i < cols; i++) {
	    N_put_array_2d_value_null(a, i, j);
	    if (!N_is_array_2d_value_null(a, i, j))
		res++;
	}
    }

    return res;
}

/* ************************************************************************* */
/* Compare two 2d arrays *************************************************** */
/* ************************************************************************* */
int compare_array_2d(N_array_2d * a, N_array_2d * b)
{
    int rows, cols, type;
    int i, j, res = 0;

    cols = a->cols;
    rows = a->rows;
    type = N_get_array_2d_type(a);

#pragma omp parallel for private (i, j) shared (cols, rows, type, a, b) reduction(+:res)
    for (j = 0; j < rows; j++) {
	for (i = 0; i < cols; i++) {
	    if (type == CELL_TYPE) {
		if (N_get_array_2d_c_value(a, i, j) !=
		    N_get_array_2d_c_value(b, i, j))
		    res++;
	    }
	    if (type == FCELL_TYPE) {
		if (N_get_array_2d_f_value(a, i, j) !=
		    N_get_array_2d_f_value(b, i, j))
		    res++;
	    }
	    if (type == DCELL_TYPE) {
		if (N_get_array_2d_d_value(a, i, j) !=
		    N_get_array_2d_d_value(b, i, j))
		    res++;
	    }
	}
    }

    return res;
}

/* ************************************************************************* */
/* Fill an 3d array with valid data **************************************** */
/* ************************************************************************* */
int fill_array_3d(N_array_3d * a)
{
    int rows, cols, depths, type;
    int i, j, k, res = 0;

    cols = a->cols;
    rows = a->rows;
    depths = a->depths;
    type = N_get_array_3d_type(a);

#pragma omp parallel for private (i, j, k) shared (depths, rows, cols, type, a) reduction(+:res)
    for (k = 0; k < depths; k++) {
	for (j = 0; j < rows; j++) {
	    for (i = 0; i < cols; i++) {
		if (type == FCELL_TYPE) {
		    N_put_array_3d_f_value(a, i, j, k,
					   (float)i * (float)j * (float)k);
		    if (N_get_array_3d_f_value(a, i, j, k) !=
			(float)i * (float)j * (float)k)
			res++;
		}
		if (type == DCELL_TYPE) {
		    N_put_array_3d_d_value(a, i, j, k,
					   (double)i * (double)j * (double)k);
		    if (N_get_array_3d_d_value(a, i, j, k) !=
			(double)i * (double)j * (double)k)
			res++;
		}
	    }
	}
    }

    return res;
}

/* ************************************************************************* */
/* Fill an 3d array with null data ***************************************** */
/* ************************************************************************* */
int fill_array_3d_null(N_array_3d * a)
{
    int rows, cols, depths, type;
    int i, j, k, res = 0;

    cols = a->cols;
    rows = a->rows;
    depths = a->depths;
    type = N_get_array_3d_type(a);

#pragma omp parallel for private (i, j, k) shared (cols, rows, depths, type, a) reduction(+:res)
    for (k = 0; k < depths; k++) {
	for (j = 0; j < rows; j++) {
	    for (i = 0; i < cols; i++) {
		N_put_array_3d_value_null(a, i, j, k);
		if (!N_is_array_3d_value_null(a, i, j, k))
		    res++;
	    }
	}
    }

    return res;
}

/* ************************************************************************* */
/* Compare two 3d arrays *************************************************** */
/* ************************************************************************* */
int compare_array_3d(N_array_3d * a, N_array_3d * b)
{
    int rows, cols, depths, type;
    int i, j, k, res = 0;

    rows = a->rows;
    cols = a->cols;
    depths = a->depths;
    type = N_get_array_3d_type(a);

#pragma omp parallel for private (i, j, k) shared (depths, rows, cols, type, a, b) reduction(+:res)
    for (k = 0; k < depths; k++) {
	for (i = 0; i < rows; i++) {
	    for (j = 0; j < cols; j++) {
		if (type == FCELL_TYPE) {
		    if (N_get_array_3d_f_value(a, i, j, k) !=
			N_get_array_3d_f_value(b, i, j, k))
			res++;
		}
		if (type == DCELL_TYPE) {
		    if (N_get_array_3d_d_value(a, i, j, k) !=
			N_get_array_3d_d_value(b, i, j, k))
			res++;
		}
	    }
	}
    }

    return res;
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
int io_bench_2d(void)
{
    int sum = 0, res = 0;
    char buff[1024];

    struct Cell_head region;
    N_array_2d *data1;
    N_array_2d *data2;
    N_array_2d *data3;
    N_array_2d *tmp;

    G_get_set_window(&region);

    data1 = N_alloc_array_2d(region.cols, region.rows, 0, CELL_TYPE);
    data2 = N_alloc_array_2d(region.cols, region.rows, 0, FCELL_TYPE);
    data3 = N_alloc_array_2d(region.cols, region.rows, 0, DCELL_TYPE);

    fill_array_2d(data1);
    fill_array_2d(data2);
    fill_array_2d(data3);

    /*raster IO methods */
    N_write_array_2d_to_rast(data1, "gpde_lib_test_raster_1");
    N_write_array_2d_to_rast(data2, "gpde_lib_test_raster_2");
    N_write_array_2d_to_rast(data2, "gpde_lib_test_raster_3");
    tmp = N_read_rast_to_array_2d("gpde_lib_test_raster_1", NULL);
    N_read_rast_to_array_2d("gpde_lib_test_raster_1", tmp);
    N_free_array_2d(tmp);
    tmp = N_read_rast_to_array_2d("gpde_lib_test_raster_2", NULL);
    N_read_rast_to_array_2d("gpde_lib_test_raster_2", tmp);
    N_free_array_2d(tmp);
    tmp = N_read_rast_to_array_2d("gpde_lib_test_raster_3", NULL);
    N_read_rast_to_array_2d("gpde_lib_test_raster_3", tmp);
    N_free_array_2d(tmp);


    sprintf(buff,
	    "g.remove rast=gpde_lib_test_raster_1,gpde_lib_test_raster_2,gpde_lib_test_raster_3");
    system(buff);

    N_free_array_2d(data1);
    N_free_array_2d(data2);
    N_free_array_2d(data3);

    return sum;
}


/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
int test_array_2d(void)
{
    int sum = 0, res = 0;

    struct Cell_head region;
    N_array_2d *data1;
    N_array_2d *data11;
    N_array_2d *data2;
    N_array_2d *data22;
    N_array_2d *data3;
    N_array_2d *data33;
    char buff[1024];
    double min, max, ssum;
    int nonzero;

    N_array_2d *tmp;

    /*Alloacte memory for all arrays */
    data1 = N_alloc_array_2d(TEST_N_NUM_COLS, TEST_N_NUM_ROWS, 1, CELL_TYPE);
    N_print_array_2d_info(data1);
    data11 = N_alloc_array_2d(TEST_N_NUM_COLS, TEST_N_NUM_ROWS, 1, CELL_TYPE);
    data2 = N_alloc_array_2d(TEST_N_NUM_COLS, TEST_N_NUM_ROWS, 1, FCELL_TYPE);
    N_print_array_2d_info(data2);
    data22 = N_alloc_array_2d(TEST_N_NUM_COLS, TEST_N_NUM_ROWS, 1, FCELL_TYPE);
    data3 = N_alloc_array_2d(TEST_N_NUM_COLS, TEST_N_NUM_ROWS, 1, DCELL_TYPE);
    N_print_array_2d_info(data3);
    data33 = N_alloc_array_2d(TEST_N_NUM_COLS, TEST_N_NUM_ROWS, 1, DCELL_TYPE);

    /*Fill the first arrays with data */

    res = fill_array_2d(data1);
    if (res != 0)
	G_warning("test_array_2d: error while filling array with values");
    sum += res;
    res = fill_array_2d(data2);
    if (res != 0)
	G_warning("test_array_2d: error while filling array with values");
    sum += res;
    res = fill_array_2d(data3);
    if (res != 0)
	G_warning("test_array_2d: error while filling array with values");
    sum += res;

    /*Copy the data */
    N_copy_array_2d(data1, data11);
    N_copy_array_2d(data2, data22);
    N_copy_array_2d(data3, data33);

    /*Compare the data */
    res = compare_array_2d(data1, data11);
    if (res != 0)
	G_warning("test_array_2d: error in  N_copy_array_2d");
    sum += res;
    res = compare_array_2d(data2, data22);
    if (res != 0)
	G_warning("test_array_2d: error in  N_copy_array_2d");
    sum += res;
    res = compare_array_2d(data3, data33);
    if (res != 0)
	G_warning("test_array_2d: error in  N_copy_array_2d");
    sum += res;

    /*compute statistics */
    N_calc_array_2d_stats(data1, &min, &max, &ssum, &nonzero, 0);
    G_message("CELL Min %g Max %g Sum %g  nonzero %i\n", min, max, ssum,
	      nonzero);
    if (min != 0 || max != 81 || ssum != 2025 || nonzero != 100) {
	G_warning("test_array_2d: error in  N_calc_array_2d_stats");
	sum++;
    }
    N_calc_array_2d_stats(data1, &min, &max, &ssum, &nonzero, 1);
    G_message("CELL Min %g Max %g Sum %g  nonzero %i\n", min, max, ssum,
	      nonzero);
    if (min != 0 || max != 81 || ssum != 2025 || nonzero != 144) {
	G_warning("test_array_2d: error in  N_calc_array_2d_stats");
	sum++;
    }

    N_calc_array_2d_stats(data2, &min, &max, &ssum, &nonzero, 0);
    G_message("FCELL Min %g Max %g Sum %g  nonzero %i\n", min, max, ssum,
	      nonzero);
    if (min != 0 || max != 81 || ssum != 2025 || nonzero != 100) {
	G_warning("test_array_2d: error in  N_calc_array_2d_stats");
	sum++;
    }

    N_calc_array_2d_stats(data2, &min, &max, &ssum, &nonzero, 1);
    G_message("FCELL Min %g Max %g Sum %g  nonzero %i\n", min, max, ssum,
	      nonzero);
    if (min != 0 || max != 81 || ssum != 2025 || nonzero != 144) {
	G_warning("test_array_2d: error in  N_calc_array_2d_stats");
	sum++;
    }

    N_calc_array_2d_stats(data3, &min, &max, &ssum, &nonzero, 0);
    G_message("DCELL Min %g Max %g Sum %g  nonzero %i\n", min, max, ssum,
	      nonzero);
    if (min != 0 || max != 81 || ssum != 2025 || nonzero != 100) {
	G_warning("test_array_2d: error in  N_calc_array_2d_stats");
	sum++;
    }

    N_calc_array_2d_stats(data3, &min, &max, &ssum, &nonzero, 1);
    G_message("DCELL Min %g Max %g Sum %g  nonzero %i\n", min, max, ssum,
	      nonzero);
    if (min != 0 || max != 81 || ssum != 2025 || nonzero != 144) {
	G_warning("test_array_2d: error in  N_calc_array_2d_stats");
	sum++;
    }



    /*test the array math functions */
    tmp = N_math_array_2d(data1, data2, NULL, N_ARRAY_SUM);
    N_math_array_2d(data2, data2, tmp, N_ARRAY_SUM);
    res = N_convert_array_2d_null_to_zero(tmp);
    if (res != 0)
	G_warning("test_array_2d: error in  N_convert_array_2d_null_to_zero");
    sum = res;
    N_free_array_2d(tmp);

    tmp = N_math_array_2d(data2, data3, NULL, N_ARRAY_DIF);
    N_math_array_2d(data1, data2, tmp, N_ARRAY_DIF);
    res = N_convert_array_2d_null_to_zero(tmp);
    if (res != 0)
	G_warning("test_array_2d: error in  N_convert_array_2d_null_to_zero");
    sum = res;
    N_free_array_2d(tmp);

    tmp = N_math_array_2d(data1, data1, NULL, N_ARRAY_MUL);
    N_math_array_2d(data1, data1, tmp, N_ARRAY_MUL);
    res = N_convert_array_2d_null_to_zero(tmp);
    if (res != 0)
	G_warning("test_array_2d: error in  N_convert_array_2d_null_to_zero");
    sum = res;
    N_free_array_2d(tmp);

    tmp = N_math_array_2d(data2, data3, NULL, N_ARRAY_DIV);
    N_math_array_2d(data1, data2, tmp, N_ARRAY_DIV);
    res = N_convert_array_2d_null_to_zero(tmp);
    if (res == 0) {		/* if a division with zero is detected, the value is set to null, not to nan */
	G_warning("test_array_2d: error in  N_convert_array_2d_null_to_zero");
	sum++;
    }
    N_free_array_2d(tmp);



    /*check for correct norm calculation */
    if (N_norm_array_2d(data1, data11, N_EUKLID_NORM) != 0.0) {
	G_warning("test_array_2d: error in  N_norm_array_2d");
	sum++;
    }
    if (N_norm_array_2d(data1, data11, N_MAXIMUM_NORM) != 0.0) {
	G_warning("test_array_2d: error in  N_norm_array_2d");
	sum++;
    }

    if (N_norm_array_2d(data2, data3, N_EUKLID_NORM) != 0.0) {
	G_warning("test_array_2d: error in  N_norm_array_2d");
	sum++;
    }
    if (N_norm_array_2d(data2, data3, N_MAXIMUM_NORM) != 0.0) {
	G_warning("test_array_2d: error in  N_norm_array_2d");
	sum++;
    }

    /*fill arrays with null values */
    res = fill_array_2d_null(data1);
    if (res != 0)
	G_warning
	    ("test_array_2d: error while filling array with cell null values");
    sum += res;
    res = fill_array_2d_null(data2);
    if (res != 0)
	G_warning
	    ("test_array_2d: error while filling array with fcell null values");
    sum += res;
    res = fill_array_2d_null(data3);
    if (res != 0)
	G_warning
	    ("test_array_2d: error while filling array with dcell null values");
    sum += res;

    /*Copy the data */
    N_copy_array_2d(data1, data11);
    N_copy_array_2d(data2, data22);
    N_copy_array_2d(data3, data33);

    /*Compare the data */
    compare_array_2d(data1, data11);
    compare_array_2d(data2, data22);
    compare_array_2d(data3, data33);

    /*check for correct norm calculation in case of null values */
    if (N_norm_array_2d(data1, data11, N_EUKLID_NORM) != 0.0) {
	G_warning("test_array_2d: error in  N_norm_array_2d");
	sum++;
    }
    if (N_norm_array_2d(data1, data11, N_MAXIMUM_NORM) != 0.0) {
	G_warning("test_array_2d: error in  N_norm_array_2d");
	sum++;
    }

    if (N_norm_array_2d(data2, data3, N_EUKLID_NORM) != 0.0) {
	G_warning("test_array_2d: error in  N_norm_array_2d");
	sum++;
    }
    if (N_norm_array_2d(data2, data3, N_MAXIMUM_NORM) != 0.0) {
	G_warning("test_array_2d: error in  N_norm_array_2d");
	sum++;
    }

    /*test the array math functions with null values */
    tmp = N_math_array_2d(data1, data11, NULL, N_ARRAY_SUM);
    N_math_array_2d(data2, data22, tmp, N_ARRAY_SUM);
    res = N_convert_array_2d_null_to_zero(tmp);
    if (res == 0) {
	G_warning("test_array_2d: error in  N_convert_array_2d_null_to_zero ");
	sum++;
    }
    N_free_array_2d(tmp);

    tmp = N_math_array_2d(data2, data22, NULL, N_ARRAY_DIF);
    N_math_array_2d(data3, data33, tmp, N_ARRAY_DIF);
    res = N_convert_array_2d_null_to_zero(tmp);
    if (res == 0) {
	G_warning("test_array_2d: error in  N_convert_array_2d_null_to_zero");
	sum++;
    }
    N_free_array_2d(tmp);

    tmp = N_math_array_2d(data1, data11, NULL, N_ARRAY_MUL);
    N_math_array_2d(data3, data33, tmp, N_ARRAY_MUL);
    res = N_convert_array_2d_null_to_zero(tmp);
    if (res == 0) {
	G_warning("test_array_2d: error in  N_convert_array_2d_null_to_zero");
	sum++;
    }
    N_free_array_2d(tmp);

    tmp = N_math_array_2d(data2, data3, NULL, N_ARRAY_DIV);
    N_math_array_2d(data1, data11, tmp, N_ARRAY_DIV);
    res = N_convert_array_2d_null_to_zero(tmp);
    if (res == 0) {
	G_warning("test_array_2d: error in  N_convert_array_2d_null_to_zero");
	sum++;
    }
    N_free_array_2d(tmp);


    N_free_array_2d(data1);
    N_free_array_2d(data2);
    N_free_array_2d(data3);

    G_get_set_window(&region);

    data1 = N_alloc_array_2d(region.cols, region.rows, 0, CELL_TYPE);
    data2 = N_alloc_array_2d(region.cols, region.rows, 0, FCELL_TYPE);
    data3 = N_alloc_array_2d(region.cols, region.rows, 0, DCELL_TYPE);
    fill_array_2d(data1);
    fill_array_2d(data2);
    fill_array_2d(data3);

    /*raster IO methods */
    N_write_array_2d_to_rast(data1, "gpde_lib_test_raster_1");
    N_write_array_2d_to_rast(data2, "gpde_lib_test_raster_2");
    N_write_array_2d_to_rast(data2, "gpde_lib_test_raster_3");
    tmp = N_read_rast_to_array_2d("gpde_lib_test_raster_1", NULL);
    N_read_rast_to_array_2d("gpde_lib_test_raster_1", tmp);
    N_free_array_2d(tmp);
    tmp = N_read_rast_to_array_2d("gpde_lib_test_raster_2", NULL);
    N_read_rast_to_array_2d("gpde_lib_test_raster_2", tmp);
    N_free_array_2d(tmp);
    tmp = N_read_rast_to_array_2d("gpde_lib_test_raster_3", NULL);
    N_read_rast_to_array_2d("gpde_lib_test_raster_3", tmp);
    N_free_array_2d(tmp);


    sprintf(buff,
	    "g.remove rast=gpde_lib_test_raster_1,gpde_lib_test_raster_2,gpde_lib_test_raster_3");
    system(buff);



    N_free_array_2d(data1);
    N_free_array_2d(data11);
    N_free_array_2d(data2);
    N_free_array_2d(data22);
    N_free_array_2d(data3);
    N_free_array_2d(data33);

    return sum;
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
int test_array_3d(void)
{
    int sum = 0, res = 0;
    char buff[1024];
    RASTER3D_Region region;

    N_array_3d *data1;
    N_array_3d *data11;
    N_array_3d *data2;
    N_array_3d *data22;

    N_array_3d *tmp;
    double min, max, ssum;
    int nonzero;

    /*Alloacte memory for all arrays */
    data1 =
	N_alloc_array_3d(TEST_N_NUM_COLS, TEST_N_NUM_ROWS, TEST_N_NUM_DEPTHS, 2,
			 FCELL_TYPE);
    N_print_array_3d_info(data1);
    data11 =
	N_alloc_array_3d(TEST_N_NUM_COLS, TEST_N_NUM_ROWS, TEST_N_NUM_DEPTHS, 2,
			 FCELL_TYPE);
    data2 =
	N_alloc_array_3d(TEST_N_NUM_COLS, TEST_N_NUM_ROWS, TEST_N_NUM_DEPTHS, 2,
			 DCELL_TYPE);
    N_print_array_3d_info(data2);
    data22 =
	N_alloc_array_3d(TEST_N_NUM_COLS, TEST_N_NUM_ROWS, TEST_N_NUM_DEPTHS, 2,
			 DCELL_TYPE);


    /*Fill the first arrays with data */

    res = fill_array_3d(data1);
    if (res != 0)
	G_warning("test_array_3d: error while filling array with values");
    sum += res;
    res = fill_array_3d(data2);
    if (res != 0)
	G_warning("test_array_3d: error while filling array with values");
    sum += res;

    /*Copy the data */
    N_copy_array_3d(data1, data11);
    N_copy_array_3d(data2, data22);

    /*Compare the data */
    res = compare_array_3d(data1, data11);
    if (res != 0)
	G_warning("test_array_3d: error in  N_copy_array_2d");
    sum += res;
    res = compare_array_3d(data1, data11);
    if (res != 0)
	G_warning("test_array_3d: error in  N_copy_array_2d");
    sum += res;



    /*compute statistics */
    N_calc_array_3d_stats(data1, &min, &max, &ssum, &nonzero, 0);
    G_message("FELL Min %g Max %g Sum %g  nonzero %i\n", min, max, ssum,
	      nonzero);
    if (min != 0 || max != 729 || ssum != 91125 || nonzero != 1000) {
	G_warning("test_array_3d: error in  N_calc_array_3d_stats");
	sum++;
    }
    N_calc_array_3d_stats(data1, &min, &max, &ssum, &nonzero, 1);
    G_message("FELL Min %g Max %g Sum %g  nonzero %i\n", min, max, ssum,
	      nonzero);
    if (min != 0 || max != 729 || ssum != 91125 || nonzero != 2744) {
	G_warning("test_array_3d: error in  N_calc_array_3d_stats");
	sum++;
    }

    N_calc_array_3d_stats(data2, &min, &max, &ssum, &nonzero, 0);
    G_message("DCELL Min %g Max %g Sum %g  nonzero %i\n", min, max, ssum,
	      nonzero);
    if (min != 0 || max != 729 || ssum != 91125 || nonzero != 1000) {
	G_warning("test_array_3d: error in  N_calc_array_3d_stats");
	sum++;
    }

    N_calc_array_3d_stats(data2, &min, &max, &ssum, &nonzero, 1);
    G_message("DCELL Min %g Max %g Sum %g  nonzero %i\n", min, max, ssum,
	      nonzero);
    if (min != 0 || max != 729 || ssum != 91125 || nonzero != 2744) {
	G_warning("test_array_3d: error in  N_calc_array_3d_stats");
	sum++;
    }


    /*test the array math functions */
    tmp = N_math_array_3d(data1, data2, NULL, N_ARRAY_SUM);
    N_math_array_3d(data2, data2, tmp, N_ARRAY_SUM);
    res = N_convert_array_3d_null_to_zero(tmp);
    if (res != 0)
	G_warning("test_array_3d: error in  N_convert_array_3d_null_to_zero");
    sum = res;
    N_free_array_3d(tmp);

    tmp = N_math_array_3d(data2, data1, NULL, N_ARRAY_DIF);
    N_math_array_3d(data1, data2, tmp, N_ARRAY_DIF);
    res = N_convert_array_3d_null_to_zero(tmp);
    if (res != 0)
	G_warning("test_array_3d: error in  N_convert_array_3d_null_to_zero");
    sum = res;
    N_free_array_3d(tmp);

    tmp = N_math_array_3d(data1, data1, NULL, N_ARRAY_MUL);
    N_math_array_3d(data1, data1, tmp, N_ARRAY_MUL);
    res = N_convert_array_3d_null_to_zero(tmp);
    if (res != 0)
	G_warning("test_array_3d: error in  N_convert_array_3d_null_to_zero");
    sum = res;
    N_free_array_3d(tmp);

    tmp = N_math_array_3d(data2, data1, NULL, N_ARRAY_DIV);
    N_math_array_3d(data1, data2, tmp, N_ARRAY_DIV);
    res = N_convert_array_3d_null_to_zero(tmp);
    if (res == 0) {		/* if a division with zero is detected, the value is set to null, not to nan */
	G_warning("test_array_3d: error in  N_convert_array_3d_null_to_zero");
	sum++;
    }
    N_free_array_3d(tmp);


    /*check for correct norm calculation */
    if (N_norm_array_3d(data1, data11, N_EUKLID_NORM) != 0.0) {
	G_warning("test_array_3d: error in  N_norm_array_3d");
	sum++;
    }
    if (N_norm_array_3d(data1, data11, N_MAXIMUM_NORM) != 0.0) {
	G_warning("test_array_3d: error in  N_norm_array_3d");
	sum++;
    }

    if (N_norm_array_3d(data1, data2, N_EUKLID_NORM) != 0.0) {
	G_warning("test_array_3d: error in  N_norm_array_3d");
	sum++;
    }
    if (N_norm_array_3d(data1, data2, N_MAXIMUM_NORM) != 0.0) {
	G_warning("test_array_3d: error in  N_norm_array_3d");
	sum++;
    }

    /*fill arrays with null values */
    res = fill_array_3d_null(data1);
    if (res != 0)
	G_warning
	    ("test_array_3d: error while filling array with float null values");
    sum += res;
    res = fill_array_3d_null(data2);
    if (res != 0)
	G_warning
	    ("test_array_3d: error while filling array with double null values");
    sum += res;

    /*Copy the data */
    N_copy_array_3d(data1, data11);
    N_copy_array_3d(data2, data22);

    /*Compare the data */
    compare_array_3d(data1, data11);
    compare_array_3d(data2, data22);

    /*test the array math functions */
    tmp = N_math_array_3d(data1, data2, NULL, N_ARRAY_SUM);
    N_math_array_3d(data2, data2, tmp, N_ARRAY_SUM);
    res = N_convert_array_3d_null_to_zero(tmp);
    if (res == 0) {
	G_warning("test_array_3d: error in  N_convert_array_3d_null_to_zero");
	sum++;
    }
    N_free_array_3d(tmp);

    tmp = N_math_array_3d(data2, data1, NULL, N_ARRAY_DIF);
    N_math_array_3d(data1, data2, tmp, N_ARRAY_DIF);
    res = N_convert_array_3d_null_to_zero(tmp);
    if (res == 0) {
	G_warning("test_array_3d: error in  N_convert_array_3d_null_to_zero");
	sum++;
    }
    N_free_array_3d(tmp);

    tmp = N_math_array_3d(data1, data1, NULL, N_ARRAY_MUL);
    N_math_array_3d(data1, data1, tmp, N_ARRAY_MUL);
    res = N_convert_array_3d_null_to_zero(tmp);
    if (res == 0) {
	G_warning("test_array_3d: error in  N_convert_array_3d_null_to_zero");
	sum++;
    }
    N_free_array_3d(tmp);

    tmp = N_math_array_3d(data2, data1, NULL, N_ARRAY_DIV);
    N_math_array_3d(data1, data2, tmp, N_ARRAY_DIV);
    res = N_convert_array_3d_null_to_zero(tmp);
    if (res == 0) {
	G_warning("test_array_3d: error in  N_convert_array_3d_null_to_zero");
	sum++;
    }
    N_free_array_3d(tmp);


    /*check for correct norm calculation in case of null values */
    if (N_norm_array_3d(data1, data11, N_EUKLID_NORM) != 0.0) {
	G_warning("test_array_3d: error in  N_norm_array_3d");
	sum++;
    }
    if (N_norm_array_3d(data1, data11, N_MAXIMUM_NORM) != 0.0) {
	G_warning("test_array_3d: error in  N_norm_array_3d");
	sum++;
    }

    if (N_norm_array_3d(data1, data2, N_EUKLID_NORM) != 0.0) {
	G_warning("test_array_3d: error in  N_norm_array_3d");
	sum++;
    }
    if (N_norm_array_3d(data1, data2, N_MAXIMUM_NORM) != 0.0) {
	G_warning("test_array_3d: error in  N_norm_array_3d");
	sum++;
    }

    N_free_array_3d(data1);
    N_free_array_3d(data2);

    /*Set the defaults */
    Rast3d_init_defaults();
    Rast3d_get_window(&region);

    data1 =
	N_alloc_array_3d(region.cols, region.rows, region.depths, 0,
			 FCELL_TYPE);
    data2 =
	N_alloc_array_3d(region.cols, region.rows, region.depths, 0,
			 DCELL_TYPE);
    fill_array_3d(data1);
    fill_array_3d(data2);


    /*Volume IO methods */
    N_write_array_3d_to_rast3d(data1, "gpde_lib_test_volume_1", 1);
    N_write_array_3d_to_rast3d(data2, "gpde_lib_test_volume_2", 1);
    tmp = N_read_rast3d_to_array_3d("gpde_lib_test_volume_1", NULL, 1);
    N_read_rast3d_to_array_3d("gpde_lib_test_volume_1", tmp, 1);
    N_free_array_3d(tmp);
    tmp = N_read_rast3d_to_array_3d("gpde_lib_test_volume_2", NULL, 1);
    N_read_rast3d_to_array_3d("gpde_lib_test_volume_2", tmp, 1);
    N_free_array_3d(tmp);

    sprintf(buff,
	    "g.remove rast3d=gpde_lib_test_volume_1,gpde_lib_test_volume_2");
    system(buff);

    N_free_array_3d(data1);
    N_free_array_3d(data11);
    N_free_array_3d(data2);
    N_free_array_3d(data22);

    return sum;
}
