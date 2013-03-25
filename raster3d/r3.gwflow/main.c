
/****************************************************************************
*
* MODULE:       r3.gwflow 
*   	    	
* AUTHOR(S):    Original author 
*               Soeren Gebbert soerengebbert <at> gmx <dot> de
* 		27 11 2006 Berlin
* PURPOSE:      Calculates confined transient three dimensional groundwater flow
*
* COPYRIGHT:    (C) 2006 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster3d.h>
#include <grass/gmath.h>
#include <grass/glocale.h>
#include <grass/N_pde.h>
#include <grass/N_gwflow.h>


/*- Parameters and global variables -----------------------------------------*/
typedef struct
{
    struct Option *output, *phead, *status, *hc_x, *hc_y, *hc_z, *q, *s, *r,
	*vector_x, *vector_y, *vector_z, *budget, *dt, *maxit, *error, *solver;
    struct Flag *mask;
    struct Flag *full_les;
} paramType;

paramType param;		/*Parameters */

/*- prototypes --------------------------------------------------------------*/
static void set_params(void);	/*Fill the paramType structure */

static void write_result(N_array_3d * status, N_array_3d * phead_start,
			 N_array_3d * phead, double *result,
			 RASTER3D_Region * region, char *name);

/* ************************************************************************* */
/* Set up the arguments we are expecting ********************************** */
/* ************************************************************************* */
void set_params(void)
{
    param.phead = G_define_standard_option(G_OPT_R3_INPUT);
    param.phead->key = "phead";
    param.phead->description = _("Input 3D raster map with initial piezometric heads in [m]");

    param.status = G_define_standard_option(G_OPT_R3_INPUT);
    param.status->key = "status";
    param.status->description =
	_
	("Input 3D raster map providing the status for each cell, = 0 - inactive, 1 - active, 2 - dirichlet");

    param.hc_x = G_define_standard_option(G_OPT_R3_INPUT);
    param.hc_x->key = "hc_x";
    param.hc_x->description =
	_("Input 3D raster map with the x-part of the hydraulic conductivity tensor in [m/s]");

    param.hc_y = G_define_standard_option(G_OPT_R3_INPUT);
    param.hc_y->key = "hc_y";
    param.hc_y->description =
	_("Input 3D raster map with the y-part of the hydraulic conductivity tensor in [m/s]");

    param.hc_z = G_define_standard_option(G_OPT_R3_INPUT);
    param.hc_z->key = "hc_z";
    param.hc_z->description =
	_("Input 3D raster map with the z-part of the hydraulic conductivity tensor in [m/s]");

    param.q = G_define_standard_option(G_OPT_R3_INPUT);
    param.q->key = "q";
    param.q->required = NO;
    param.q->description = _("Input 3D raster map with sources and sinks in [m^3/s]");

    param.s = G_define_standard_option(G_OPT_R3_INPUT);
    param.s->key = "s";
    param.s->description = _("Specific yield [1/m] input 3D raster map");

    param.r = G_define_standard_option(G_OPT_R3_INPUT);
    param.r->key = "r";
    param.r->required = NO;
    param.r->description = _("Recharge input 3D raster map in m^3/s");

    param.output = G_define_standard_option(G_OPT_R3_OUTPUT);
    param.output->key = "output";
    param.output->description = _("Output 3D raster map storing the piezometric head result of the numerical calculation");

    param.vector_x = G_define_standard_option(G_OPT_R3_OUTPUT);
    param.vector_x->key = "vx";
    param.vector_x->required = NO;
    param.vector_x->description =
	_("Output 3D raster map storing the groundwater filter velocity vector part in x direction [m/s]");

    param.vector_y = G_define_standard_option(G_OPT_R3_OUTPUT);
    param.vector_y->key = "vy";
    param.vector_y->required = NO;
    param.vector_y->description =
	_("Output 3D raster map storing the groundwater filter velocity vector part in y direction [m/s]");

    param.vector_z = G_define_standard_option(G_OPT_R3_OUTPUT);
    param.vector_z->key = "vz";
    param.vector_z->required = NO;
    param.vector_z->description =
	_("Output 3D raster map storing the groundwater filter velocity vector part in z direction [m/s]");

    param.budget = G_define_standard_option(G_OPT_R3_OUTPUT);
    param.budget->key = "budget";
    param.budget->required = NO;
    param.budget->description =
	_("Output 3D raster map Storing the groundwater budget for each cell [m^3/s]\n");

    param.dt = N_define_standard_option(N_OPT_CALC_TIME);
    param.maxit = N_define_standard_option(N_OPT_MAX_ITERATIONS);
    param.error = N_define_standard_option(N_OPT_ITERATION_ERROR);
    param.solver = N_define_standard_option(N_OPT_SOLVER_SYMM);
    param.solver->options = "cg,pcg,cholesky";

    param.mask = G_define_flag();
    param.mask->key = 'm';
    param.mask->description = _("Use RASTER3D mask (if exists)");

    param.full_les = G_define_flag();
    param.full_les->key = 'f';
    param.full_les->description = _("Use a full filled quadratic linear equation system,"
            " default is a sparse linear equation system.");
}

/* ************************************************************************* */
/* Main function *********************************************************** */
/* ************************************************************************* */
int main(int argc, char *argv[])
{
    struct GModule *module = NULL;
    N_gwflow_data3d *data = NULL;
    N_geom_data *geom = NULL;
    N_les *les = NULL;
    N_les_callback_3d *call = NULL;
    RASTER3D_Region region;
    N_gradient_field_3d *field = NULL;
    N_array_3d *xcomp = NULL;
    N_array_3d *ycomp = NULL;
    N_array_3d *zcomp = NULL;
    double error;
    int maxit;
    const char *solver;
    int x, y, z, stat;

    /* Initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("groundwater flow"));
    G_add_keyword(_("voxel"));
    G_add_keyword(_("hydrology"));
    module->description = _("Numerical calculation program for transient, confined groundwater flow in three dimensions.");

    /* Get parameters from user */
    set_params();

    /* Have GRASS get pheads */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /*Set the maximum iterations */
    sscanf(param.maxit->answer, "%i", &(maxit));
    /*Set the calculation error break criteria */
    sscanf(param.error->answer, "%lf", &(error));
    /*Set the solver */
    solver = param.solver->answer;

    if (strcmp(solver, G_MATH_SOLVER_DIRECT_CHOLESKY) == 0 && !param.full_les->answer)
	G_fatal_error(_("The cholesky solver does not work with sparse matrices.\n"
                "Consider to choose a full filled quadratic matrix with flag -f "));



    /*Set the defaults */
    Rast3d_init_defaults();

    /*get the current region */
    Rast3d_get_window(&region);

    /*allocate the geometry structure  for geometry and area calculation */
    geom = N_init_geom_data_3d(&region, geom);

    /*Set the function callback to the groundwater flow function */
    call = N_alloc_les_callback_3d();
    N_set_les_callback_3d_func(call, (*N_callback_gwflow_3d));	/*gwflow 3d */

    /*Allocate the groundwater flow data structure */
    data = N_alloc_gwflow_data3d(geom->cols, geom->rows, geom->depths, 0, 0);

    /*Set the calculation time */
    sscanf(param.dt->answer, "%lf", &(data->dt));

    /*read the g3d maps into the memory and convert the null values */
    N_read_rast3d_to_array_3d(param.phead->answer, data->phead,
			      param.mask->answer);
    N_convert_array_3d_null_to_zero(data->phead);
    N_read_rast3d_to_array_3d(param.phead->answer, data->phead_start,
			      param.mask->answer);
    N_convert_array_3d_null_to_zero(data->phead_start);
    N_read_rast3d_to_array_3d(param.status->answer, data->status,
			      param.mask->answer);
    N_convert_array_3d_null_to_zero(data->status);
    N_read_rast3d_to_array_3d(param.hc_x->answer, data->hc_x,
			      param.mask->answer);
    N_convert_array_3d_null_to_zero(data->hc_x);
    N_read_rast3d_to_array_3d(param.hc_y->answer, data->hc_y,
			      param.mask->answer);
    N_convert_array_3d_null_to_zero(data->hc_y);
    N_read_rast3d_to_array_3d(param.hc_z->answer, data->hc_z,
			      param.mask->answer);
    N_convert_array_3d_null_to_zero(data->hc_z);
    N_read_rast3d_to_array_3d(param.q->answer, data->q, param.mask->answer);
    N_convert_array_3d_null_to_zero(data->q);
    N_read_rast3d_to_array_3d(param.s->answer, data->s, param.mask->answer);
    N_convert_array_3d_null_to_zero(data->s);

    /* Set the inactive values to zero, to assure a no flow boundary */
    for (z = 0; z < geom->depths; z++) {
	for (y = 0; y < geom->rows; y++) {
	    for (x = 0; x < geom->cols; x++) {
		stat = (int)N_get_array_3d_d_value(data->status, x, y, z);
		if (stat == N_CELL_INACTIVE) {	/*only inactive cells */
		    N_put_array_3d_d_value(data->hc_x, x, y, z, 0);
		    N_put_array_3d_d_value(data->hc_y, x, y, z, 0);
		    N_put_array_3d_d_value(data->hc_z, x, y, z, 0);
		    N_put_array_3d_d_value(data->s, x, y, z, 0);
		    N_put_array_3d_d_value(data->q, x, y, z, 0);
		}
	    }
	}
    }

    /*assemble the linear equation system */
    if (!param.full_les->answer) {
	les =
	    N_assemble_les_3d(N_SPARSE_LES, geom, data->status, data->phead,
			      (void *)data, call);
    }
    else {
	les =
	    N_assemble_les_3d(N_NORMAL_LES, geom, data->status, data->phead,
			      (void *)data, call);
    }

    if (les && les->type == N_NORMAL_LES) {
	if (strcmp(solver, G_MATH_SOLVER_ITERATIVE_CG) == 0)
	    G_math_solver_cg(les->A, les->x, les->b, les->rows, maxit, error);

	if (strcmp(solver, G_MATH_SOLVER_ITERATIVE_PCG) == 0)
	    G_math_solver_pcg(les->A, les->x, les->b, les->rows, maxit, error,
			      G_MATH_DIAGONAL_PRECONDITION);

	if (strcmp(solver, G_MATH_SOLVER_DIRECT_CHOLESKY) == 0)
	    G_math_solver_cholesky(les->A, les->x, les->b, les->rows,
				   les->rows);
    }
    else if (les && les->type == N_SPARSE_LES) {
	if (strcmp(solver, G_MATH_SOLVER_ITERATIVE_CG) == 0)
	    G_math_solver_sparse_cg(les->Asp, les->x, les->b, les->rows,
				    maxit, error);

	if (strcmp(solver, G_MATH_SOLVER_ITERATIVE_PCG) == 0)
	    G_math_solver_sparse_pcg(les->Asp, les->x, les->b, les->rows,
				     maxit, error, G_MATH_DIAGONAL_PRECONDITION);
    }

    if (les == NULL)
	G_fatal_error(_("Unable to create and solve the linear equation system"));


    /*write the result to the output file and copy the values to the data->phead array */
    write_result(data->status, data->phead_start, data->phead, les->x,
		 &region, param.output->answer);
    N_free_les(les);

    /* Compute the water budget for each cell */
    N_array_3d *budget = N_alloc_array_3d(geom->cols, geom->rows, geom->depths, 1, DCELL_TYPE);
    N_gwflow_3d_calc_water_budget(data, geom, budget);
    
    /*Write the water balance */
    if(param.budget->answer)
    {
	N_write_array_3d_to_rast3d(budget, param.budget->answer, 1);
    }

    /*Compute the the velocity field if required and write the result into three rast3d maps */
    if (param.vector_x->answer || param.vector_y->answer || param.vector_z->answer) {
	field =
	    N_compute_gradient_field_3d(data->phead, data->hc_x, data->hc_y,
					data->hc_z, geom, NULL);

	xcomp =
	    N_alloc_array_3d(geom->cols, geom->rows, geom->depths, 1,
			     DCELL_TYPE);
	ycomp =
	    N_alloc_array_3d(geom->cols, geom->rows, geom->depths, 1,
			     DCELL_TYPE);
	zcomp =
	    N_alloc_array_3d(geom->cols, geom->rows, geom->depths, 1,
			     DCELL_TYPE);

	N_compute_gradient_field_components_3d(field, xcomp, ycomp, zcomp);


        if(param.vector_x->answer)
            N_write_array_3d_to_rast3d(xcomp, param.vector_x->answer, 1);
        if(param.vector_y->answer)
            N_write_array_3d_to_rast3d(ycomp, param.vector_y->answer, 1);
        if(param.vector_z->answer)
            N_write_array_3d_to_rast3d(zcomp, param.vector_z->answer, 1);

	if (xcomp)
	    N_free_array_3d(xcomp);
	if (ycomp)
	    N_free_array_3d(ycomp);
	if (zcomp)
	    N_free_array_3d(zcomp);
	if (field)
	    N_free_gradient_field_3d(field);
    }

    if (data)
	N_free_gwflow_data3d(data);
    if (geom)
	N_free_geom_data(geom);
    if (call)
	G_free(call);

    return (EXIT_SUCCESS);
}


/* ************************************************************************* */
/* this function writes the result from the x vector to a g3d map ********** */
/* ************************************************************************* */
void
write_result(N_array_3d * status, N_array_3d * phead_start,
	     N_array_3d * phead, double *result, RASTER3D_Region * region,
	     char *name)
{
    void *map = NULL;
    int changemask = 0;
    int z, y, x, rows, cols, depths, count, stat;
    double d1 = 0;

    rows = region->rows;
    cols = region->cols;
    depths = region->depths;

    /*Open the new map */
    map = Rast3d_open_new_opt_tile_size(name, RASTER3D_USE_CACHE_XY, region, DCELL_TYPE, 32);

    if (map == NULL)
	Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"), name);

    /*if requested set the Mask on */
    if (param.mask->answer) {
	if (Rast3d_mask_file_exists()) {
	    changemask = 0;
	    if (Rast3d_mask_is_off(map)) {
		Rast3d_mask_on(map);
		changemask = 1;
	    }
	}
    }

    count = 0;
    for (z = 0; z < depths; z++) {
	G_percent(z, depths - 1, 10);
	for (y = 0; y < rows; y++) {
	    for (x = 0; x < cols; x++) {
		stat = (int)N_get_array_3d_d_value(status, x, y, z);
		if (stat == N_CELL_ACTIVE) {	/*only active cells */
		    d1 = result[count];
		    /*copy the values */
		    N_put_array_3d_d_value(phead, x, y, z, d1);
		    count++;
		}
		else if (stat == N_CELL_DIRICHLET) {	/*dirichlet cells */
		    d1 = N_get_array_3d_d_value(phead_start, x, y, z);
		}
		else {
		    Rast3d_set_null_value(&d1, 1, DCELL_TYPE);
		}
		Rast3d_put_double(map, x, y, z, d1);
	    }
	}
    }

    /*We set the Mask off, if it was off before */
    if (param.mask->answer) {
	if (Rast3d_mask_file_exists())
	    if (Rast3d_mask_is_on(map) && changemask)
		Rast3d_mask_off(map);
    }

    /* Flush all tile */
    if (!Rast3d_flush_all_tiles(map))
	Rast3d_fatal_error("Error flushing tiles with Rast3d_flush_all_tiles");
    if (!Rast3d_close(map))
	Rast3d_fatal_error(map, NULL, 0, _("Unable to close 3D raster map"));

    return;
}
