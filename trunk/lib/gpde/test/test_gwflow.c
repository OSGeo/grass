
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      gwflow integration tests
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#include <grass/gmath.h>


#include <grass/gis.h>
#include <grass/N_pde.h>
#include <grass/gmath.h>
#include <grass/N_gwflow.h>
#include "test_gpde_lib.h"

/*redefine */
#define TEST_N_NUM_DEPTHS_LOCAL 2
#define TEST_N_NUM_ROWS_LOCAL 3
#define TEST_N_NUM_COLS_LOCAL 3

/* prototypes */
static N_gwflow_data2d *create_gwflow_data_2d(void);
static N_gwflow_data3d *create_gwflow_data_3d(void);
static int test_gwflow_2d(void);
static int test_gwflow_3d(void);


/* *************************************************************** */
/* Performe the gwflow integration tests ************************* */
/* *************************************************************** */
int integration_test_gwflow(void)
{
    int sum = 0;

    G_message("\n++ Running gwflow integration tests ++");

    G_message("\t 1. testing 2d gwflow");
    sum += test_gwflow_2d();

    G_message("\t 2. testing 3d gwflow");
    sum += test_gwflow_3d();

    if (sum > 0)
	G_warning("\n-- gwflow integration tests failure --");
    else
	G_message("\n-- gwflow integration tests finished successfully --");

    return sum;
}


/* *************************************************************** */
/* Create valid groundwater flow data **************************** */
/* *************************************************************** */
N_gwflow_data3d *create_gwflow_data_3d(void)
{
    N_gwflow_data3d *data;
    int i, j, k;

    data =
	N_alloc_gwflow_data3d(TEST_N_NUM_COLS_LOCAL, TEST_N_NUM_ROWS_LOCAL,
			      TEST_N_NUM_DEPTHS_LOCAL, 1, 1);

#pragma omp parallel for private (i, j, k) shared (data)
    for (k = 0; k < TEST_N_NUM_DEPTHS_LOCAL; k++)
	for (j = 0; j < TEST_N_NUM_ROWS_LOCAL; j++) {
	    for (i = 0; i < TEST_N_NUM_COLS_LOCAL; i++) {


		if (j == 0) {
		    N_put_array_3d_d_value(data->phead, i, j, k, 50);
		    N_put_array_3d_d_value(data->phead_start, i, j, k, 50);
		    N_put_array_3d_d_value(data->status, i, j, k, 2);
		}
		else {

		    N_put_array_3d_d_value(data->phead, i, j, k, 40);
		    N_put_array_3d_d_value(data->phead_start, i, j, k, 40);
		    N_put_array_3d_d_value(data->status, i, j, k, 1);
		}
		N_put_array_3d_d_value(data->hc_x, i, j, k, 0.0001);
		N_put_array_3d_d_value(data->hc_y, i, j, k, 0.0001);
		N_put_array_3d_d_value(data->hc_z, i, j, k, 0.0001);
		N_put_array_3d_d_value(data->q, i, j, k, 0.0);
		N_put_array_3d_d_value(data->s, i, j, k, 0.001);
		N_put_array_2d_d_value(data->r, i, j, 0.0);
		N_put_array_3d_d_value(data->nf, i, j, k, 0.1);
	    }
	}

    return data;
}


/* *************************************************************** */
/* Create valid groundwater flow data **************************** */
/* *************************************************************** */
N_gwflow_data2d *create_gwflow_data_2d(void)
{
    int i, j;
    N_gwflow_data2d *data;

    data = N_alloc_gwflow_data2d(TEST_N_NUM_COLS_LOCAL, TEST_N_NUM_ROWS_LOCAL, 1, 1);

#pragma omp parallel for private (i, j) shared (data)
    for (j = 0; j < TEST_N_NUM_ROWS_LOCAL; j++) {
	for (i = 0; i < TEST_N_NUM_COLS_LOCAL; i++) {

	    if (j == 0) {
		N_put_array_2d_d_value(data->phead, i, j, 50);
		N_put_array_2d_d_value(data->phead_start, i, j, 50);
		N_put_array_2d_d_value(data->status, i, j, 2);
	    }
	    else {

		N_put_array_2d_d_value(data->phead, i, j, 40);
		N_put_array_2d_d_value(data->phead_start, i, j, 40);
		N_put_array_2d_d_value(data->status, i, j, 1);
	    }
	    N_put_array_2d_d_value(data->hc_x, i, j, 30.0001);
	    N_put_array_2d_d_value(data->hc_y, i, j, 30.0001);
	    N_put_array_2d_d_value(data->q, i, j, 0.0);
	    N_put_array_2d_d_value(data->s, i, j, 0.001);
	    N_put_array_2d_d_value(data->r, i, j, 0.0);
	    N_put_array_2d_d_value(data->nf, i, j, 0.1);
	    N_put_array_2d_d_value(data->top, i, j, 20.0);
	    N_put_array_2d_d_value(data->bottom, i, j, 0.0);
	}
    }

    return data;
}

/* *************************************************************** */
/* Test the groundwater flow in 3d with different solvers ******** */
/* *************************************************************** */
int test_gwflow_3d(void)
{


    N_gwflow_data3d *data;
    N_geom_data *geom;
    N_les *les;
    N_les_callback_3d *call;

    call = N_alloc_les_callback_3d();
    N_set_les_callback_3d_func(call, (*N_callback_gwflow_3d));	/*gwflow 3d */

    data = create_gwflow_data_3d();

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
     /*CG*/ les =
	N_assemble_les_3d(N_SPARSE_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    G_math_solver_sparse_cg(les->Asp, les->x, les->b, les->rows, 100, 0.1e-8);
    N_print_les(les);
    N_free_les(les);

     /*PCG G_MATH_DIAGONAL_PRECONDITION*/ les =
	N_assemble_les_3d(N_SPARSE_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    G_math_solver_sparse_pcg(les->Asp, les->x, les->b, les->rows, 100, 0.1e-8, G_MATH_DIAGONAL_PRECONDITION);
    N_print_les(les);
    N_free_les(les);

     /*PCG G_MATH_ROWSCALE_EUKLIDNORM_PRECONDITION*/ les =
	N_assemble_les_3d(N_SPARSE_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    G_math_solver_sparse_pcg(les->Asp, les->x, les->b, les->rows, 100, 0.1e-8, G_MATH_ROWSCALE_EUKLIDNORM_PRECONDITION);
    N_print_les(les);
    N_free_les(les);

     /*PCG G_MATH_ROWSCALE_ABSSUMNORM_PRECONDITION*/ les =
	N_assemble_les_3d(N_SPARSE_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    G_math_solver_sparse_pcg(les->Asp, les->x, les->b, les->rows, 100, 0.1e-8, G_MATH_ROWSCALE_ABSSUMNORM_PRECONDITION);
    N_print_les(les);
    N_free_les(les);


     /*CG*/ les =
	N_assemble_les_3d_dirichlet(N_SPARSE_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    N_les_integrate_dirichlet_3d(les, geom, data->status, data->phead_start);
    G_math_solver_sparse_cg(les->Asp, les->x, les->b, les->rows, 100, 0.1e-8);
    N_print_les(les);
    N_free_les(les);


     /*CG*/ les =
	N_assemble_les_3d(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
	  
    G_math_solver_cg(les->A, les->x, les->b, les->rows,  100, 0.1e-8);
    N_print_les(les);
    N_free_les(les);

     /*PCG G_MATH_DIAGONAL_PRECONDITION*/ les =
	N_assemble_les_3d(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
	  
    G_math_solver_pcg(les->A, les->x, les->b, les->rows,  100, 0.1e-8, G_MATH_DIAGONAL_PRECONDITION);
    N_print_les(les);
    N_free_les(les);

     /*PCG G_MATH_ROWSCALE_EUKLIDNORM_PRECONDITION*/ les =
	N_assemble_les_3d(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
	  
    G_math_solver_pcg(les->A, les->x, les->b, les->rows,  100, 0.1e-8, G_MATH_ROWSCALE_EUKLIDNORM_PRECONDITION);
    N_print_les(les);
    N_free_les(les);

     /*PCG G_MATH_ROWSCALE_ABSSUMNORM_PRECONDITION*/ les =
	N_assemble_les_3d(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
	  
    G_math_solver_pcg(les->A, les->x, les->b, les->rows, 100, 0.1e-8, G_MATH_ROWSCALE_ABSSUMNORM_PRECONDITION);
    N_print_les(les);
    N_free_les(les);


     /*CG*/ les =
	N_assemble_les_3d_dirichlet(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    N_les_integrate_dirichlet_3d(les, geom, data->status, data->phead_start);
    G_math_solver_cg(les->A, les->x, les->b, les->rows, 100, 0.1e-8);
    N_print_les(les);
    N_free_les(les);


     /*BICG*/ les =
	N_assemble_les_3d(N_SPARSE_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    G_math_solver_sparse_bicgstab(les->Asp, les->x, les->b, les->rows,  100, 0.1e-8);
    N_print_les(les);
    N_free_les(les);

     /*BICG*/ les =
	N_assemble_les_3d(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    G_math_solver_bicgstab(les->A, les->x, les->b, les->rows, 100, 0.1e-8);
    N_print_les(les);
    N_free_les(les);

     /*BICG*/ les =
	N_assemble_les_3d_dirichlet(N_SPARSE_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    N_les_integrate_dirichlet_3d(les, geom, data->status, data->phead_start);
    G_math_solver_sparse_bicgstab(les->Asp, les->x, les->b, les->rows, 100, 0.1e-8);
    N_print_les(les);
    N_free_les(les);

     /*BICG*/ les =
	N_assemble_les_3d_dirichlet(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    N_les_integrate_dirichlet_3d(les, geom, data->status, data->phead_start);
    G_math_solver_bicgstab(les->A, les->x, les->b, les->rows, 100, 0.1e-8);
    N_print_les(les);
    N_free_les(les);


     /*GUASS*/ les =
	N_assemble_les_3d(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    G_math_solver_gauss(les->A, les->x, les->b, les->rows);
    N_print_les(les);
    N_free_les(les);

     /*LU*/ les =
	N_assemble_les_3d(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    G_math_solver_lu(les->A, les->x, les->b, les->rows);
    N_print_les(les);
    N_free_les(les);

     /*GUASS*/ les =
	N_assemble_les_3d_dirichlet(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    N_les_integrate_dirichlet_3d(les, geom, data->status, data->phead_start);
    G_math_solver_gauss(les->A, les->x, les->b, les->rows);
    N_print_les(les);
    N_free_les(les);

     /*LU*/ les =
	N_assemble_les_3d_dirichlet(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    N_les_integrate_dirichlet_3d(les, geom, data->status, data->phead_start);
    G_math_solver_lu(les->A, les->x, les->b, les->rows);
    N_print_les(les);
    N_free_les(les);

     /*Cholesky*/ les =
	N_assemble_les_3d(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    G_math_solver_cholesky(les->A, les->x, les->b, les->rows, les->rows);
    N_print_les(les);
    N_free_les(les);

     /*Cholesky*/ les =
	N_assemble_les_3d_dirichlet(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    N_les_integrate_dirichlet_3d(les, geom, data->status, data->phead_start);
    G_math_solver_cholesky(les->A, les->x, les->b, les->rows, les->rows);
    N_print_les(les);
    N_free_les(les);

    N_free_gwflow_data3d(data);
    G_free(geom);
    G_free(call);

    return 0;
}

/* *************************************************************** */
int test_gwflow_2d(void)
{
    N_gwflow_data2d *data;
    N_geom_data *geom;
    N_les *les;
    N_les_callback_2d *call;

    /*set the callback */
    call = N_alloc_les_callback_2d();
    N_set_les_callback_2d_func(call, (*N_callback_gwflow_2d));

    data = create_gwflow_data_2d();
    data->dt = 600;

    geom = N_alloc_geom_data();

    geom->dx = 10;
    geom->dy = 15;

    geom->Az = 150;

    geom->rows = TEST_N_NUM_ROWS_LOCAL;
    geom->cols = TEST_N_NUM_COLS_LOCAL;


     /*Assemble the matrix */
    /*  
     */
     /*CG*/ les =
	N_assemble_les_2d(N_SPARSE_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    G_math_solver_sparse_cg(les->Asp, les->x, les->b, les->rows, 100, 0.1e-8);
    N_print_les(les);
    N_free_les(les);

     /*PCG G_MATH_DIAGONAL_PRECONDITION*/ les =
	N_assemble_les_2d(N_SPARSE_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    G_math_solver_sparse_pcg(les->Asp, les->x, les->b, les->rows, 100, 0.1e-8, G_MATH_DIAGONAL_PRECONDITION);
    N_print_les(les);
    N_free_les(les);

     /*PCG G_MATH_ROWSCALE_EUKLIDNORM_PRECONDITION*/ les =
	N_assemble_les_2d(N_SPARSE_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    G_math_solver_sparse_pcg(les->Asp, les->x, les->b, les->rows, 100, 0.1e-8, G_MATH_ROWSCALE_EUKLIDNORM_PRECONDITION);
    N_print_les(les);
    N_free_les(les);

     /*PCG G_MATH_ROWSCALE_ABSSUMNORM_PRECONDITION*/ les =
	N_assemble_les_2d(N_SPARSE_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    G_math_solver_sparse_pcg(les->Asp, les->x, les->b, les->rows, 100, 0.1e-8, G_MATH_ROWSCALE_ABSSUMNORM_PRECONDITION);
    N_print_les(les);
    N_free_les(les);


     /*CG*/ les =
	N_assemble_les_2d_dirichlet(N_SPARSE_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    N_les_integrate_dirichlet_2d(les, geom, data->status, data->phead_start);
    G_math_solver_sparse_cg(les->Asp, les->x, les->b, les->rows, 100, 0.1e-8);
    N_print_les(les);
    N_free_les(les);


     /*CG*/ les =
	N_assemble_les_2d(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
	  
    G_math_solver_cg(les->A, les->x, les->b, les->rows,  100, 0.1e-8);
    N_print_les(les);
    N_free_les(les);

     /*PCG G_MATH_DIAGONAL_PRECONDITION*/ les =
	N_assemble_les_2d(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
	  
    G_math_solver_pcg(les->A, les->x, les->b, les->rows,  100, 0.1e-8, G_MATH_DIAGONAL_PRECONDITION);
    N_print_les(les);
    N_free_les(les);

     /*PCG G_MATH_ROWSCALE_EUKLIDNORM_PRECONDITION*/ les =
	N_assemble_les_2d(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
	  
    G_math_solver_pcg(les->A, les->x, les->b, les->rows,  100, 0.1e-8, G_MATH_ROWSCALE_EUKLIDNORM_PRECONDITION);
    N_print_les(les);
    N_free_les(les);

     /*PCG G_MATH_ROWSCALE_ABSSUMNORM_PRECONDITION*/ les =
	N_assemble_les_2d(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
	  
    G_math_solver_pcg(les->A, les->x, les->b, les->rows, 100, 0.1e-8, G_MATH_ROWSCALE_ABSSUMNORM_PRECONDITION);
    N_print_les(les);
    N_free_les(les);


     /*CG*/ les =
	N_assemble_les_2d_dirichlet(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    N_les_integrate_dirichlet_2d(les, geom, data->status, data->phead_start);
    G_math_solver_cg(les->A, les->x, les->b, les->rows, 100, 0.1e-8);
    N_print_les(les);
    N_free_les(les);


     /*BICG*/ les =
	N_assemble_les_2d(N_SPARSE_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    G_math_solver_sparse_bicgstab(les->Asp, les->x, les->b, les->rows,  100, 0.1e-8);
    N_print_les(les);
    N_free_les(les);

     /*BICG*/ les =
	N_assemble_les_2d(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    G_math_solver_bicgstab(les->A, les->x, les->b, les->rows, 100, 0.1e-8);
    N_print_les(les);
    N_free_les(les);

     /*BICG*/ les =
	N_assemble_les_2d_dirichlet(N_SPARSE_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    N_les_integrate_dirichlet_2d(les, geom, data->status, data->phead_start);
    G_math_solver_sparse_bicgstab(les->Asp, les->x, les->b, les->rows, 100, 0.1e-8);
    N_print_les(les);
    N_free_les(les);

     /*BICG*/ les =
	N_assemble_les_2d_dirichlet(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    N_les_integrate_dirichlet_2d(les, geom, data->status, data->phead_start);
    G_math_solver_bicgstab(les->A, les->x, les->b, les->rows, 100, 0.1e-8);
    N_print_les(les);
    N_free_les(les);


     /*GUASS*/ les =
	N_assemble_les_2d(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    G_math_solver_gauss(les->A, les->x, les->b, les->rows);
    N_print_les(les);
    N_free_les(les);

     /*LU*/ les =
	N_assemble_les_2d(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    G_math_solver_lu(les->A, les->x, les->b, les->rows);
    N_print_les(les);
    N_free_les(les);

     /*GUASS*/ les =
	N_assemble_les_2d_dirichlet(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    N_les_integrate_dirichlet_2d(les, geom, data->status, data->phead_start);
    G_math_solver_gauss(les->A, les->x, les->b, les->rows);
    N_print_les(les);
    N_free_les(les);

     /*LU*/ les =
	N_assemble_les_2d_dirichlet(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    N_les_integrate_dirichlet_2d(les, geom, data->status, data->phead_start);
    G_math_solver_lu(les->A, les->x, les->b, les->rows);
    N_print_les(les);
    N_free_les(les);

     /*Cholesky*/ les =
	N_assemble_les_2d(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    G_math_solver_cholesky(les->A, les->x, les->b, les->rows, les->rows);
    N_print_les(les);
    N_free_les(les);

     /*Cholesky*/ les =
	N_assemble_les_2d_dirichlet(N_NORMAL_LES, geom, data->status, data->phead_start,
			  (void *)data, call);
    N_les_integrate_dirichlet_2d(les, geom, data->status, data->phead_start);
    G_math_solver_cholesky(les->A, les->x, les->b, les->rows, les->rows);
    N_print_les(les);
    N_free_les(les);
    
    N_free_gwflow_data2d(data);
    G_free(geom);
    G_free(call);

    return 0;
}
