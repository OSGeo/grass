
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      part of the gpde library
* 		allocation, destroying and initializing the geometric struct
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/


#include <grass/N_pde.h>

/* *************************************************************** * 
 * *********** Konstruktor *************************************** * 
 * *************************************************************** */
/*!
 * \brief Allocate the pde geometry data structure and return a pointer to the new allocated structure
 *
 * \return N_geom_data *
 * */
N_geom_data *N_alloc_geom_data(void)
{
    N_geom_data *geom = (N_geom_data *) G_calloc(1, sizeof(N_geom_data));

    geom->area = NULL;
    geom->planimetric = 1;
    geom->dim = 0;

    return geom;
}

/* *************************************************************** * 
 * *********** Destruktor **************************************** * 
 * *************************************************************** */
/*!
 * \brief Release memory of a pde geometry data structure
 *
 * \param  geom N_geom_data *
 * \return void
 * */
void N_free_geom_data(N_geom_data * geom)
{
    if (geom->area != NULL)
	G_free(geom->area);

    G_free(geom);
    return;
}

/* *************************************************************** * 
 * *************************************************************** * 
 * *************************************************************** */
/*!
 * \brief Initiate a pde geometry data structure with a 3d region
 *
 * If the projection is not planimetric, a double array will be created based on the 
 * number of rows of the provided region
 *
 * \param region3d RASTER3D_Region *
 * \param geodata N_geom_data * - if a NULL pointer is given, a new structure will be allocatet and returned
 *
 * \return N_geom_data *
 * */
N_geom_data *N_init_geom_data_3d(RASTER3D_Region * region3d, N_geom_data * geodata)
{
    N_geom_data *geom = geodata;
    struct Cell_head region2d;

#pragma omp critical
    {

	G_debug(2,
		"N_init_geom_data_3d: initializing the geometry structure");

	if (geom == NULL)
	    geom = N_alloc_geom_data();

	geom->dz = region3d->tb_res * G_database_units_to_meters_factor();	/*this function is not thread safe */
	geom->depths = region3d->depths;
	geom->dim = 3;

	/*convert the 3d into a 2d region and begin the area calculation */
	G_get_set_window(&region2d);	/*this function is not thread safe */
	Rast3d_region_to_cell_head(region3d, &region2d);
    }

    return N_init_geom_data_2d(&region2d, geom);
}


/* *************************************************************** * 
 * *************************************************************** * 
 * *************************************************************** */
/*!
 * \brief Initiate a pde geometry data structure with a 2d region
 *
 * If the projection is not planimetric, a double array will be created based on the 
 * number of rows of the provided region storing all computed areas for each row
 *
 * \param region sruct Cell_head *
 * \param geodata N_geom_data * - if a NULL pointer is given, a new structure will be allocatet and returned
 *
 * \return N_geom_data *
 * */
N_geom_data *N_init_geom_data_2d(struct Cell_head * region,
				 N_geom_data * geodata)
{
    N_geom_data *geom = geodata;
    struct Cell_head backup;
    double meters;
    short ll = 0;
    int i;


    /*create an openmp lock to assure that only one thread at a time will access this function */
#pragma omp critical
    {
	G_debug(2,
		"N_init_geom_data_2d: initializing the geometry structure");

	/*make a backup from this region */
	G_get_set_window(&backup);	/*this function is not thread safe */
	/*set the current region */
	Rast_set_window(region);	/*this function is not thread safe */

	if (geom == NULL)
	    geom = N_alloc_geom_data();

	meters = G_database_units_to_meters_factor();	/*this function is not thread safe */

	/*set the dim to 2d if it was not initiated with 3, that's a bit ugly :( */
	if (geom->dim != 3)
	    geom->dim = 2;

	geom->planimetric = 1;
	geom->rows = region->rows;
	geom->cols = region->cols;
	geom->dx = region->ew_res * meters;
	geom->dy = region->ns_res * meters;
	geom->Az = geom->dy * geom->dx;	/*square meters in planimetric proj */
	/*depths and dz are initialized with a 3d region */

	/*Begin the area calculation */
	ll = G_begin_cell_area_calculations();	/*this function is not thread safe */

	/*if the projection is not planimetric, calc the area for each row */
	if (ll == 2) {
	    G_debug(2,
		    "N_init_geom_data_2d: calculating the areas for non parametric projection");
	    geom->planimetric = 0;

	    if (geom->area != NULL)
		G_free(geom->area);
	    else
		geom->area = G_calloc(geom->rows, sizeof(double));

	    /*fill the area vector */
	    for (i = 0; i < geom->rows; i++) {
		geom->area[i] = G_area_of_cell_at_row(i);	/*square meters */
	    }
	}

	/*restore the old region */
	Rast_set_window(&backup);	/*this function is not thread safe */
    }

    return geom;
}

/* *************************************************************** * 
 * *************************************************************** * 
 * *************************************************************** */
/*!
 * \brief Get the areay size in square meter of one cell (x*y) at row
 *
 * This function works for two and three dimensions
 *
 * \param geom N_geom_data *
 * \param row int
 * \return area double
 *
 * */
double N_get_geom_data_area_of_cell(N_geom_data * geom, int row)
{
    if (geom->planimetric) {
	G_debug(6, "N_get_geom_data_area_of_cell: %g", geom->Az);
	return geom->Az;
    }
    else {
	G_debug(6, "N_get_geom_data_area_of_cell: %g", geom->area[row]);
	return geom->area[row];
    }

    return 0.0;
}
