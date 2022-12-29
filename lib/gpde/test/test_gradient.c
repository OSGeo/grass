
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      Unit tests for gradient calculation
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
static int test_gradient_2d(void);
static int test_gradient_3d(void);
static N_array_2d *create_relax_array_2d(void);
static N_array_3d *create_relax_array_3d(void);
static N_array_2d *create_potential_array_2d(void);
static N_array_3d *create_potential_array_3d(void);

/* *************************************************************** */
/* Performe the gradient tests *********************************** */
/* *************************************************************** */
int unit_test_gradient(void)
{
    int sum = 0;

    G_message("\n++ Running gradient unit tests ++");

    G_message("\t 1. testing 2d gradient");
    sum += test_gradient_2d();

    G_message("\t 2. testing 3d gradient");
    sum += test_gradient_3d();

    if (sum > 0)
	G_warning("\n-- Gradient unit tests failure --");
    else
	G_message("\n-- Gradient unit tests finished successfully --");

    return sum;
}

/* *************************************************************** */
/* Create the status array with values of 1 and 2 **************** */
/* *************************************************************** */
N_array_2d *create_relax_array_2d(void)
{
    N_array_2d *data;
    int i, j;

    data = N_alloc_array_2d(TEST_N_NUM_COLS, TEST_N_NUM_ROWS, 1, CELL_TYPE);

#pragma omp parallel for private (i, j) shared (data)
    for (j = 0; j < TEST_N_NUM_ROWS; j++) {
	for (i = 0; i < TEST_N_NUM_COLS; i++) {
		N_put_array_2d_c_value(data, i, j, 1);
	}
    }
    return data;
}

/* *************************************************************** */
/* Create a value array ****************************************** */
/* *************************************************************** */
N_array_2d *create_potential_array_2d(void)
{
    N_array_2d *data;
    int i, j;

    data = N_alloc_array_2d(TEST_N_NUM_COLS, TEST_N_NUM_ROWS, 1, DCELL_TYPE);

#pragma omp parallel for private (i, j) shared (data)
    for (j = 0; j < TEST_N_NUM_ROWS; j++) {
	for (i = 0; i < TEST_N_NUM_COLS; i++) {
		N_put_array_2d_d_value(data, i, j, (double)i*j);
	}
    }
    return data;
}

/* *************************************************************** */
/* Create the status array with values of 1 and 2 **************** */
/* *************************************************************** */
N_array_3d *create_relax_array_3d(void)
{
    N_array_3d *data;
    int i, j, k;

    data =
	N_alloc_array_3d(TEST_N_NUM_COLS, TEST_N_NUM_ROWS, TEST_N_NUM_DEPTHS, 1,
			 FCELL_TYPE);

#pragma omp parallel for private (i, j, k) shared (data)
    for (k = 0; k < TEST_N_NUM_DEPTHS; k++) {
	for (j = 0; j < TEST_N_NUM_ROWS; j++) {
	    for (i = 0; i < TEST_N_NUM_COLS; i++) {
		    N_put_array_3d_f_value(data, i, j, k, 1.0);
		}
	    }
	}

    return data;

}

/* *************************************************************** */
/* Create a value array ****************************************** */
/* *************************************************************** */
N_array_3d *create_potential_array_3d(void)
{
    N_array_3d *data;
    int i, j, k;

    data =
	N_alloc_array_3d(TEST_N_NUM_COLS, TEST_N_NUM_ROWS, TEST_N_NUM_DEPTHS, 1,
			 DCELL_TYPE);

#pragma omp parallel for private (i, j, k) shared (data)
    for (k = 0; k < TEST_N_NUM_DEPTHS; k++)
	for (j = 0; j < TEST_N_NUM_ROWS; j++) {
	    for (i = 0; i < TEST_N_NUM_COLS; i++) {
		    N_put_array_3d_f_value(data, i, j, k, (float)i*j*k);
		}
	    }

    return data;

}

/* *************************************************************** */
/* Test the gradient calculation with 3d array data ************** */
/* *************************************************************** */
int test_gradient_3d(void)
{
    N_array_3d *relax = NULL;
    N_array_3d *pot = NULL;
    N_array_3d *xcomp = NULL;
    N_array_3d *ycomp = NULL;
    N_array_3d *zcomp = NULL;
    N_gradient_field_3d *field = NULL;
    N_gradient_3d *grad = NULL;
    N_geom_data *geom = NULL;

    geom = N_alloc_geom_data();

    geom->dx = 1;
    geom->dy = 1;
    geom->dz = 1;

    geom->Az = 1;
    geom->planimetric = 1;

    geom->depths = TEST_N_NUM_DEPTHS;
    geom->rows = TEST_N_NUM_ROWS;
    geom->cols = TEST_N_NUM_COLS;


    relax = create_relax_array_3d();
    pot = create_potential_array_3d();

    field = N_compute_gradient_field_3d(pot, relax, relax, relax, geom, NULL);
    field = N_compute_gradient_field_3d(pot, relax, relax, relax, geom, field);

    /*compute stats*/
    N_calc_gradient_field_3d_stats(field);
    N_print_gradient_field_3d_info(field);

    N_free_gradient_field_3d(field);

    N_free_array_3d(relax);
    N_free_array_3d(pot);

    relax = N_alloc_array_3d(3, 3, 3, 0, DCELL_TYPE);
    pot =   N_alloc_array_3d(3, 3, 3, 0, DCELL_TYPE);
    xcomp = N_alloc_array_3d(3, 3, 3, 0, DCELL_TYPE);
    ycomp = N_alloc_array_3d(3, 3, 3, 0, DCELL_TYPE);
    zcomp = N_alloc_array_3d(3, 3, 3, 0, DCELL_TYPE);

    N_put_array_3d_d_value(relax, 0, 0, 0, 1);
    N_put_array_3d_d_value(relax, 0, 1, 0, 1);
    N_put_array_3d_d_value(relax, 0, 2, 0, 1);
    N_put_array_3d_d_value(relax, 1, 0, 0, 1);
    N_put_array_3d_d_value(relax, 1, 1, 0, 1);
    N_put_array_3d_d_value(relax, 1, 2, 0, 1);
    N_put_array_3d_d_value(relax, 2, 0, 0, 1);
    N_put_array_3d_d_value(relax, 2, 1, 0, 1);
    N_put_array_3d_d_value(relax, 2, 2, 0, 1);

    N_put_array_3d_d_value(relax, 0, 0, 1, 1);
    N_put_array_3d_d_value(relax, 0, 1, 1, 1);
    N_put_array_3d_d_value(relax, 0, 2, 1, 1);
    N_put_array_3d_d_value(relax, 1, 0, 1, 1);
    N_put_array_3d_d_value(relax, 1, 1, 1, 1);
    N_put_array_3d_d_value(relax, 1, 2, 1, 1);
    N_put_array_3d_d_value(relax, 2, 0, 1, 1);
    N_put_array_3d_d_value(relax, 2, 1, 1, 1);
    N_put_array_3d_d_value(relax, 2, 2, 1, 1);


    N_put_array_3d_d_value(relax, 0, 0, 2, 1);
    N_put_array_3d_d_value(relax, 0, 1, 2, 1);
    N_put_array_3d_d_value(relax, 0, 2, 2, 1);
    N_put_array_3d_d_value(relax, 1, 0, 2, 1);
    N_put_array_3d_d_value(relax, 1, 1, 2, 1);
    N_put_array_3d_d_value(relax, 1, 2, 2, 1);
    N_put_array_3d_d_value(relax, 2, 0, 2, 1);
    N_put_array_3d_d_value(relax, 2, 1, 2, 1);
    N_put_array_3d_d_value(relax, 2, 2, 2, 1);


  /**
   * 1 2 6        5
   * 3 7 10 == -4  -3
   * 8 15 25      8
   * */

    N_put_array_3d_d_value(pot, 0, 0, 0, 1.0);
    N_put_array_3d_d_value(pot, 1, 0, 0, 2.0);
    N_put_array_3d_d_value(pot, 2, 0, 0, 6.0);
    N_put_array_3d_d_value(pot, 0, 1, 0, 3.0);
    N_put_array_3d_d_value(pot, 1, 1, 0, 7.0);
    N_put_array_3d_d_value(pot, 2, 1, 0, 10.0);
    N_put_array_3d_d_value(pot, 0, 2, 0, 8.0);
    N_put_array_3d_d_value(pot, 1, 2, 0, 15.0);
    N_put_array_3d_d_value(pot, 2, 2, 0, 25.0);

    N_put_array_3d_d_value(pot, 0, 0, 1, 1.2);
    N_put_array_3d_d_value(pot, 1, 0, 1, 2.2);
    N_put_array_3d_d_value(pot, 2, 0, 1, 6.2);
    N_put_array_3d_d_value(pot, 0, 1, 1, 3.2);
    N_put_array_3d_d_value(pot, 1, 1, 1, 7.2);
    N_put_array_3d_d_value(pot, 2, 1, 1, 10.2);
    N_put_array_3d_d_value(pot, 0, 2, 1, 8.2);
    N_put_array_3d_d_value(pot, 1, 2, 1, 15.2);
    N_put_array_3d_d_value(pot, 2, 2, 1, 25.2);


    N_put_array_3d_d_value(pot, 0, 0, 2, 1.5);
    N_put_array_3d_d_value(pot, 1, 0, 2, 2.5);
    N_put_array_3d_d_value(pot, 2, 0, 2, 6.5);
    N_put_array_3d_d_value(pot, 0, 1, 2, 3.5);
    N_put_array_3d_d_value(pot, 1, 1, 2, 7.5);
    N_put_array_3d_d_value(pot, 2, 1, 2, 10.5);
    N_put_array_3d_d_value(pot, 0, 2, 2, 8.5);
    N_put_array_3d_d_value(pot, 1, 2, 2, 15.5);
    N_put_array_3d_d_value(pot, 2, 2, 2, 25.5);


    geom->depths = 3;
    geom->rows = 3;
    geom->cols = 3;

    field = N_compute_gradient_field_3d(pot, relax, relax, relax, geom, NULL);
    field = N_compute_gradient_field_3d(pot, relax, relax, relax, geom, field);
    N_print_gradient_field_3d_info(field);

    grad = N_get_gradient_3d(field, NULL, 0, 0, 0);
    printf
	("Gradient 3d: NC %g == 0 ; SC %g == 2 ; WC %g == 0 ; EC %g == -1 BC %g == 0 TC %g == -0.2\n",
	 grad->NC, grad->SC, grad->WC, grad->EC, grad->BC, grad->TC);

    grad = N_get_gradient_3d(field, grad, 1, 0, 0);
    printf
	("Gradient 3d: NC %g == 0 ; SC %g == 5 ; WC %g == -1 ; EC %g == -4 BC %g == 0 TC %g == -0.2\n",
	 grad->NC, grad->SC, grad->WC, grad->EC, grad->BC, grad->TC);
    N_free_gradient_3d(grad);

    grad = N_get_gradient_3d(field, NULL, 1, 1, 1);
    printf
	("Gradient 3d: NC %g == 5 ; SC %g == 8 ; WC %g == -4 ; EC %g == -3 BC %g == -0.2 TC %g == -0.3\n",
	 grad->NC, grad->SC, grad->WC, grad->EC, grad->BC, grad->TC);

    grad = N_get_gradient_3d(field, grad, 1, 2, 2);
    printf
	("Gradient 3d: NC %g == 8 ; SC %g ==  0 ; WC %g == -7 ; EC %g == -10 BC %g == -0.3 TC %g == 0\n",
	 grad->NC, grad->SC, grad->WC, grad->EC, grad->BC, grad->TC);
    N_free_gradient_3d(grad);

    grad = N_get_gradient_3d(field, NULL, 2, 2, 2);
    printf
	("Gradient 3d: NC %g ==15 ; SC %g ==  0 ; WC %g == -10 ; EC %g ==  0 BC %g == -0.3 TC %g == 0\n",
	 grad->NC, grad->SC, grad->WC, grad->EC, grad->BC, grad->TC);
    N_free_gradient_3d(grad);

    N_compute_gradient_field_components_3d(field, xcomp, ycomp, zcomp);

    /*
    N_print_array_3d(xcomp);
    N_print_array_3d(ycomp);
    N_print_array_3d(zcomp);
    */

    N_free_gradient_field_3d(field);
    G_free(geom);

    N_free_array_3d(xcomp);
    N_free_array_3d(ycomp);
    N_free_array_3d(zcomp);
    N_free_array_3d(relax);
    N_free_array_3d(pot);

    return 0;
}


/* *************************************************************** */
/* Test the gradient calculation with 2d array data ************** */
/* *************************************************************** */
int test_gradient_2d(void)
{
    N_array_2d *relax = NULL;
    N_array_2d *pot = NULL;
    N_array_2d *xcomp = NULL;
    N_array_2d *ycomp = NULL;
    N_gradient_field_2d *field = NULL;
    N_geom_data *geom = NULL;
    N_gradient_2d *grad = NULL;
    N_gradient_neighbours_2d *grad_2d = NULL;

    geom = N_alloc_geom_data();

    geom->dx = 1;
    geom->dy = 1;
    geom->dz = 1;

    geom->Az = 1;
    geom->planimetric = 1;

    geom->rows = TEST_N_NUM_ROWS;
    geom->cols = TEST_N_NUM_COLS;


    relax = create_relax_array_2d();
    pot = create_potential_array_2d();


    field = N_compute_gradient_field_2d(pot, relax, relax, geom, field);
    field = N_compute_gradient_field_2d(pot, relax, relax, geom, field);

    /*compute stats*/
    N_calc_gradient_field_2d_stats(field);
    N_print_gradient_field_2d_info(field);

    N_free_gradient_field_2d(field);

    N_free_array_2d(relax);
    N_free_array_2d(pot);

    relax = N_alloc_array_2d(3, 3, 0, DCELL_TYPE);
    pot =   N_alloc_array_2d(3, 3, 0, DCELL_TYPE);
    xcomp = N_alloc_array_2d(3, 3, 0, DCELL_TYPE);
    ycomp = N_alloc_array_2d(3, 3, 0, DCELL_TYPE);

    N_put_array_2d_d_value(relax, 0, 0, 1.0);
    N_put_array_2d_d_value(relax, 0, 1, 1.0);
    N_put_array_2d_d_value(relax, 0, 2, 1.0);
    N_put_array_2d_d_value(relax, 1, 0, 1.0);
    N_put_array_2d_d_value(relax, 1, 1, 1.0);
    N_put_array_2d_d_value(relax, 1, 2, 1.0);
    N_put_array_2d_d_value(relax, 2, 0, 1.0);
    N_put_array_2d_d_value(relax, 2, 1, 1.0);
    N_put_array_2d_d_value(relax, 2, 2, 1.0);

  /**
   * 1 2 6        5
   * 3 7 10 == -4  -3
   * 8 15 25      8
   * */

    N_put_array_2d_d_value(pot, 0, 0, 1.0);
    N_put_array_2d_d_value(pot, 1, 0, 2.0);
    N_put_array_2d_d_value(pot, 2, 0, 6.0);
    N_put_array_2d_d_value(pot, 0, 1, 3.0);
    N_put_array_2d_d_value(pot, 1, 1, 7.0);
    N_put_array_2d_d_value(pot, 2, 1, 10.0);
    N_put_array_2d_d_value(pot, 0, 2, 8.0);
    N_put_array_2d_d_value(pot, 1, 2, 15.0);
    N_put_array_2d_d_value(pot, 2, 2, 25.0);

    geom->rows = 3;
    geom->cols = 3;

    field = N_compute_gradient_field_2d(pot, relax, relax, geom, NULL);
    field = N_compute_gradient_field_2d(pot, relax, relax, geom, field);
    N_print_gradient_field_2d_info(field);

    /*common gradient calculation*/
    grad = N_get_gradient_2d(field, NULL, 0, 0);
    G_message("Gradient 2d: pos 0,0 NC %g == 0 ; SC %g == 2 ; WC %g == 0 ; EC %g == -1\n",
	   grad->NC, grad->SC, grad->WC, grad->EC);

    grad = N_get_gradient_2d(field, grad, 1, 0);
    G_message("Gradient 2d: pos 1,0 NC %g == 0 ; SC %g == 5 ; WC %g == -1 ; EC %g == -4\n",
	   grad->NC, grad->SC, grad->WC, grad->EC);
    N_free_gradient_2d(grad);

    grad = N_get_gradient_2d(field, NULL, 1, 1);
    G_message("Gradient 2d: pos 1,1 NC %g == 5 ; SC %g == 8 ; WC %g == -4 ; EC %g == -3\n",
	   grad->NC, grad->SC, grad->WC, grad->EC);

    grad = N_get_gradient_2d(field, grad, 1, 2);
    G_message("Gradient 2d: pos 1,2 NC %g == 8 ; SC %g ==  0 ; WC %g == -7 ; EC %g == -10\n",
	   grad->NC, grad->SC, grad->WC, grad->EC);
    N_free_gradient_2d(grad);

    grad = N_get_gradient_2d(field, NULL, 2, 2);
    G_message("Gradient 2d: pos 2,2 NC %g ==15 ; SC %g ==  0 ; WC %g == -10 ; EC %g ==  0\n",
	   grad->NC, grad->SC, grad->WC, grad->EC);
    N_free_gradient_2d(grad);

    N_compute_gradient_field_components_2d(field, xcomp, ycomp);

    /*gradient neighbour calculation*/
    grad_2d = N_get_gradient_neighbours_2d(field, NULL, 1, 1);
    grad_2d = N_get_gradient_neighbours_2d(field, grad_2d, 1, 1);
    G_message("N_gradient_neighbours_x; pos 1,1 NWN %g NEN %g WC %g EC %g SWS %g SES %g\n",
	   grad_2d->x->NWN, grad_2d->x->NEN, grad_2d->x->WC, grad_2d->x->EC, grad_2d->x->SWS, grad_2d->x->SES);
    G_message("N_gradient_neighbours_y: pos 1,1 NWW %g NEE %g NC %g SC %g SWW %g SEE %g\n",
	   grad_2d->y->NWW, grad_2d->y->NEE, grad_2d->y->NC, grad_2d->y->SC, grad_2d->y->SWW, grad_2d->y->SEE);
	   
    N_free_gradient_neighbours_2d(grad_2d);

    /*
    N_print_array_2d(xcomp);
    N_print_array_2d(ycomp);
    */

    N_free_gradient_field_2d(field);
    G_free(geom);

    N_free_array_2d(xcomp);
    N_free_array_2d(ycomp);
    N_free_array_2d(relax);
    N_free_array_2d(pot);


    return 0;
}
