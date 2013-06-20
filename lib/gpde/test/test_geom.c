
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      Unit tests for geometry calculations
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
#include <grass/raster3d.h>
#include "test_gpde_lib.h"

/* prototypes */
static int test_geom_data(void);

/* ************************************************************************* */
/* Performe the geom_data unit tests *************************************** */
/* ************************************************************************* */
int unit_test_geom_data(void)
{
    int sum = 0;

    G_message("\n++ Running geom_data unit tests ++");

    sum += test_geom_data();

    if (sum > 0)
	G_warning("\n-- geom_data unit tests failure --");
    else
	G_message("\n-- geom_data unit tests finished successfully --");

    return sum;
}


/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
int test_geom_data(void)
{
    struct Cell_head region2d;
    RASTER3D_Region region3d;
    N_geom_data *geom = NULL;
    int sum = 0, i;
    double area = 0;

    G_get_set_window(&region2d);

    /*Set the defaults */
    Rast3d_init_defaults();

    /*get the current region */
    Rast3d_get_window(&region3d);

    geom = N_alloc_geom_data();
    if (!geom) {
	G_warning("error in N_alloc_geom_data");
	return 1;
    }
    N_free_geom_data(geom);
    geom = NULL;

    /* ************ 2d region *************** */
    geom = N_init_geom_data_2d(&region2d, geom);
    if (!geom) {
	G_warning("error in N_init_geom_data_2d");
	return 2;
    }

    geom = N_init_geom_data_2d(&region2d, geom);
    if (!geom) {
	G_warning("error in N_init_geom_data_2d");
	return 3;
    }

    if (geom->dim != 2)
	sum++;
    if (geom->planimetric == 0 && geom->area == NULL)
	sum++;
    if (geom->planimetric == 1 && geom->area != NULL)
	sum++;

    /*get areas */
    area = 0.0;
    if (geom->planimetric == 0) {
	for (i = 0; i < geom->rows; i++)
	    area += N_get_geom_data_area_of_cell(geom, i);

	if (area == 0) {
	    G_warning("Wrong area calculation in N_init_geom_data_2d");
	    sum++;
	}
    }

    area = 0.0;
    if (geom->planimetric == 1) {
	for (i = 0; i < geom->rows; i++)
	    area += N_get_geom_data_area_of_cell(geom, i);

	if (area == 0) {
	    G_warning("Wrong area calculation in N_get_geom_data_area_of_cell");
	    sum++;
	}
    }


    N_free_geom_data(geom);
    geom = NULL;

    /* ************ 3d region *************** */
    geom = N_init_geom_data_3d(&region3d, geom);
    if (!geom) {
	G_warning("error in N_init_geom_data_3d");
	return 2;
    }

    geom = N_init_geom_data_3d(&region3d, geom);
    if (!geom) {
	G_warning("error in N_init_geom_data_3d");
	return 3;
    }

    if (geom->dim != 3)
	sum++;
    if (geom->planimetric == 0 && geom->area == NULL)
	sum++;

    if (geom->planimetric == 1 && geom->area != NULL)
	sum++;

    /*get areas */
    area = 0.0;
    if (geom->planimetric == 0) {
	for (i = 0; i < geom->rows; i++)
	    area += N_get_geom_data_area_of_cell(geom, i);

	if (area == 0) {
	    G_warning("Wrong area calculation in N_get_geom_data_area_of_cell");
	    sum++;
	}
    }

    area = 0.0;
    if (geom->planimetric == 1) {
	for (i = 0; i < geom->rows; i++)
	    area += N_get_geom_data_area_of_cell(geom, i);

	if (area == 0) {
	    G_warning("Wrong area calculation in N_get_geom_data_area_of_cell");
	    sum++;
	}
    }

    return sum;

}
