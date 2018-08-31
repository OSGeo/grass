
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      solute_transport integration tests
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#include <grass/gis.h>
#include <grass/N_pde.h>
#include <grass/N_solute_transport.h>
#include "test_gpde_lib.h"

/*redefine */
#define TEST_N_NUM_DEPTHS_LOCAL 2
#define TEST_N_NUM_ROWS_LOCAL 3
#define TEST_N_NUM_COLS_LOCAL 3

/* prototypes */
static N_solute_transport_data2d *create_solute_transport_data_2d(void);
static N_solute_transport_data3d *create_solute_transport_data_3d(void);
static int test_solute_transport_2d(void);
static int test_solute_transport_3d(void);


/* *************************************************************** */
/* Performe the solute_transport integration tests ************************* */
/* *************************************************************** */
int integration_test_solute_transport(void)
{
    int sum = 0;

    G_message("\n++ Running solute_transport integration tests ++");

    G_message("\t 1. testing 2d solute_transport");
    sum += test_solute_transport_2d();

    G_message("\t 2. testing 3d solute_transport");
    sum += test_solute_transport_3d();

    if (sum > 0)
	G_warning("\n-- solute_transport integration tests failure --");
    else
	G_message("\n-- solute_transport integration tests finished successfully --");

    return sum;
}


/* *************************************************************** */
/* Create valid solute transport data **************************** */
/* *************************************************************** */
N_solute_transport_data3d *create_solute_transport_data_3d(void)
{
    N_solute_transport_data3d *data;
    int i, j, k;

    data =
	N_alloc_solute_transport_data3d(TEST_N_NUM_COLS_LOCAL,
					TEST_N_NUM_ROWS_LOCAL,
					TEST_N_NUM_DEPTHS_LOCAL);

#pragma omp parallel for private (i, j, k) shared (data)
    for (k = 0; k < TEST_N_NUM_DEPTHS_LOCAL; k++)
	for (j = 0; j < TEST_N_NUM_ROWS_LOCAL; j++) {
	    for (i = 0; i < TEST_N_NUM_COLS_LOCAL; i++) {


		if (j == 0) {
		    N_put_array_3d_d_value(data->c, i, j, k, 1);
		    N_put_array_3d_d_value(data->c_start, i, j, k, 1);
		    N_put_array_3d_d_value(data->status, i, j, k, 3);
		}
		else {

		    N_put_array_3d_d_value(data->c, i, j, k, 0);
		    N_put_array_3d_d_value(data->c_start, i, j, k, 0);
		    N_put_array_3d_d_value(data->status, i, j, k, 1);
		}
		N_put_array_3d_d_value(data->diff_x, i, j, k, 0.000001);
		N_put_array_3d_d_value(data->diff_y, i, j, k, 0.000001);
		N_put_array_3d_d_value(data->diff_z, i, j, k, 0.000001);
		N_put_array_3d_d_value(data->q, i, j, k, 0.0);
		N_put_array_3d_d_value(data->cs, i, j, k, 0.0);
		N_put_array_3d_d_value(data->R, i, j, k, 1.0);
		N_put_array_3d_d_value(data->nf, i, j, k, 0.1);
		if (j == 1 && i == 1 && k == 1)
		    N_put_array_3d_d_value(data->cs, i, j, k, 5.0);

	    }
	}

    return data;
}


/* *************************************************************** */
/* Create valid solute transport data **************************** */
/* *************************************************************** */
N_solute_transport_data2d *create_solute_transport_data_2d(void)
{
    int i, j;
    N_solute_transport_data2d *data;

    data =
	N_alloc_solute_transport_data2d(TEST_N_NUM_COLS_LOCAL,
					TEST_N_NUM_ROWS_LOCAL);

#pragma omp parallel for private (i, j) shared (data)
    for (j = 0; j < TEST_N_NUM_ROWS_LOCAL; j++) {
	for (i = 0; i < TEST_N_NUM_COLS_LOCAL; i++) {

	    if (j == 0) {
		N_put_array_2d_d_value(data->c, i, j, 0);
		N_put_array_2d_d_value(data->c_start, i, j, 0);
		N_put_array_2d_d_value(data->status, i, j, 2);
	    }
	    else {

		N_put_array_2d_d_value(data->c, i, j, 0);
		N_put_array_2d_d_value(data->c_start, i, j, 0);
		N_put_array_2d_d_value(data->status, i, j, 1);
	    }
	    N_put_array_2d_d_value(data->diff_x, i, j, 0.000001);
	    N_put_array_2d_d_value(data->diff_y, i, j, 0.000001);
	    N_put_array_2d_d_value(data->cs, i, j, 0.0);
	    N_put_array_2d_d_value(data->R, i, j, 1.0);
	    N_put_array_2d_d_value(data->q, i, j, 0.0);
	    N_put_array_2d_d_value(data->nf, i, j, 0.1);
	    N_put_array_2d_d_value(data->top, i, j, 20.0);
	    N_put_array_2d_d_value(data->bottom, i, j, 0.0);
	    if (j == 1 && i == 1)
		N_put_array_2d_d_value(data->cs, i, j, 1.0);
	}
    }
   /*dispersivity length*/
   data->al = 0.2;
   data->at = 0.02;





    return data;
}

/* *************************************************************** */
/* Test the solute transport in 3d with different solvers ******** */
/* *************************************************************** */
int test_solute_transport_3d(void)
{
    N_solute_transport_data3d *data;
    N_geom_data *geom;
    N_les *les;
    N_les_callback_3d *call;

    call = N_alloc_les_callback_3d();
    N_set_les_callback_3d_func(call, (*N_callback_solute_transport_3d));	/*solute_transport 3d */

    data = create_solute_transport_data_3d();
 
    N_calc_solute_transport_disptensor_3d(data);

    data->dt = 86400;

    geom = N_alloc_geom_data();

    geom->dx = 10;
    geom->dy = 15;
    geom->dz = 3;

    geom->Az = 150;

    geom->depths = TEST_N_NUM_DEPTHS_LOCAL;
    geom->rows = TEST_N_NUM_ROWS_LOCAL;
    geom->cols = TEST_N_NUM_COLS_LOCAL;
    /*Assemble the matrix */
    /*  
     */

     /*BICG*/ les =
	N_assemble_les_3d(N_SPARSE_LES, geom, data->status, data->c_start,
			  (void *)data, call);
    G_math_solver_sparse_bicgstab(les->Asp, les->x, les->b, les->rows, 100, 0.1e-8);
    N_print_les(les);
    N_free_les(les);

     /*BICG*/ les =
	N_assemble_les_3d(N_NORMAL_LES, geom, data->status, data->c_start,
			  (void *)data, call);
    G_math_solver_bicgstab(les->A, les->x, les->b, les->rows, 100, 0.1e-8);
    N_print_les(les);
    N_free_les(les);

     /*GUASS*/ les =
	N_assemble_les_3d(N_NORMAL_LES, geom, data->status, data->c_start,
			  (void *)data, call);
    G_math_solver_gauss(les->A, les->x, les->b, les->rows);
    N_print_les(les);
    N_free_les(les);

     /*LU*/ les =
	N_assemble_les_3d(N_NORMAL_LES, geom, data->status, data->c_start,
			  (void *)data, call);
    G_math_solver_lu(les->A, les->x, les->b, les->rows);
    N_print_les(les);
    N_free_les(les);

    N_free_solute_transport_data3d(data);
    G_free(geom);
    G_free(call);

    return 0;
}

/* *************************************************************** */
/* Test the solute transport in 2d with different solvers ******** */
/* *************************************************************** */
int test_solute_transport_2d(void)
{
    N_solute_transport_data2d *data = NULL;
    N_geom_data *geom = NULL;
    N_les *les = NULL;
    N_les_callback_2d *call = NULL;
    N_array_2d *pot, *relax = NULL;
    N_gradient_field_2d *field = NULL;
    int i, j;

    /*set the callback */
    call = N_alloc_les_callback_2d();
    N_set_les_callback_2d_func(call, (*N_callback_solute_transport_2d));

    pot =
	N_alloc_array_2d(TEST_N_NUM_COLS_LOCAL, TEST_N_NUM_ROWS_LOCAL, 1,
			 DCELL_TYPE);
    relax =
	N_alloc_array_2d(TEST_N_NUM_COLS_LOCAL, TEST_N_NUM_ROWS_LOCAL, 1,
			 DCELL_TYPE);

    data = create_solute_transport_data_2d();


    data->dt = 600;

    geom = N_alloc_geom_data();

    geom->dx = 10;
    geom->dy = 15;

    geom->Az = 150;

    geom->rows = TEST_N_NUM_ROWS_LOCAL;
    geom->cols = TEST_N_NUM_COLS_LOCAL;


    for (j = 0; j < TEST_N_NUM_ROWS_LOCAL; j++) {
	for (i = 0; i < TEST_N_NUM_COLS_LOCAL; i++) {
	    N_put_array_2d_d_value(pot, i, j, j);
	    N_put_array_2d_d_value(relax, i, j, 1);
	}
    }

    field = N_compute_gradient_field_2d(pot, relax, relax, geom, NULL);
    N_copy_gradient_field_2d(field, data->grad);
    N_free_gradient_field_2d(field);

    N_compute_gradient_field_2d(pot, relax, relax, geom, data->grad);
    /*The dispersivity tensor*/
    N_calc_solute_transport_disptensor_2d(data);

    /*Assemble the matrix */
    /*  
     */
   /*BICG*/ les =
	N_assemble_les_2d(N_SPARSE_LES, geom, data->status, data->c_start,
			  (void *)data, call);
    G_math_solver_sparse_bicgstab(les->Asp, les->x, les->b, les->rows, 100, 0.1e-8);
    N_print_les(les);
    N_free_les(les);

     /*BICG*/ les =
	N_assemble_les_2d(N_NORMAL_LES, geom, data->status, data->c_start,
			  (void *)data, call);
    G_math_solver_bicgstab(les->A, les->x, les->b, les->rows, 100, 0.1e-8);
    N_print_les(les);
    N_free_les(les);

     /*GUASS*/ les =
	N_assemble_les_2d(N_NORMAL_LES, geom, data->status, data->c_start,
			  (void *)data, call);
    G_math_solver_gauss(les->A, les->x, les->b, les->rows);
    N_print_les(les);
    N_free_les(les);

     /*LU*/ les =
	N_assemble_les_2d(N_NORMAL_LES, geom, data->status, data->c_start,
			  (void *)data, call);
    G_math_solver_lu(les->A, les->x, les->b, les->rows);
    N_print_les(les);
    N_free_les(les);

    N_free_solute_transport_data2d(data);
    G_free(geom);
    G_free(call);

    return 0;
}
