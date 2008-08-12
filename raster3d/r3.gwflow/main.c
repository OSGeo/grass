
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
#include <grass/G3d.h>
#include <grass/glocale.h>
#include <grass/N_pde.h>
#include <grass/N_gwflow.h>


/*- Parameters and global variables -----------------------------------------*/
typedef struct
{
    struct Option *output, *phead, *status, *hc_x, *hc_y, *hc_z, *q, *s, *r,
	*vector, *dt, *maxit, *error, *solver, *sor;
    struct Flag *mask;
    struct Flag *sparse;
} paramType;

paramType param;		/*Parameters */

/*- prototypes --------------------------------------------------------------*/
void set_params(void);		/*Fill the paramType structure */
void write_result(N_array_3d * status, N_array_3d * phead_start,
		  N_array_3d * phead, double *result, G3D_Region * region,
		  char *name);

/* ************************************************************************* */
/* Set up the arguments we are expecting ********************************** */
/* ************************************************************************* */
void set_params(void)
{
    param.phead = G_define_option();
    param.phead->key = "phead";
    param.phead->type = TYPE_STRING;
    param.phead->required = YES;
    param.phead->gisprompt = "old,grid3,3d-raster";
    param.phead->description = _("The initial piezometric head in [m]");

    param.status = G_define_option();
    param.status->key = "status";
    param.status->type = TYPE_STRING;
    param.status->required = YES;
    param.status->gisprompt = "old,grid3,3d-raster";
    param.status->description =
	_("The status for each cell, = 0 - inactive, 1 - active, 2 - dirichlet");

    param.hc_x = G_define_option();
    param.hc_x->key = "hc_x";
    param.hc_x->type = TYPE_STRING;
    param.hc_x->required = YES;
    param.hc_x->gisprompt = "old,grid3,3d-raster";
    param.hc_x->description =
	_("The x-part of the hydraulic conductivity tensor in [m/s]");

    param.hc_y = G_define_option();
    param.hc_y->key = "hc_y";
    param.hc_y->type = TYPE_STRING;
    param.hc_y->required = YES;
    param.hc_y->gisprompt = "old,grid3,3d-raster";
    param.hc_y->description =
	_("The y-part of the hydraulic conductivity tensor in [m/s]");

    param.hc_z = G_define_option();
    param.hc_z->key = "hc_z";
    param.hc_z->type = TYPE_STRING;
    param.hc_z->required = YES;
    param.hc_z->gisprompt = "old,grid3,3d-raster";
    param.hc_z->description =
	_("The z-part of the hydraulic conductivity tensor in [m/s]");

    param.q = G_define_option();
    param.q->key = "q";
    param.q->type = TYPE_STRING;
    param.q->required = NO;
    param.q->gisprompt = "old,grid3,3d-raster";
    param.q->description = _("Sources and sinks in [m^3/s]");

    param.s = G_define_option();
    param.s->key = "s";
    param.s->type = TYPE_STRING;
    param.s->required = YES;
    param.s->gisprompt = "old,grid3,3d-raster";
    param.s->description = _("Specific yield in 1/m");

    param.r = G_define_option();
    param.r->key = "r";
    param.r->type = TYPE_STRING;
    param.r->required = NO;
    param.r->gisprompt = "old,raster,raster";
    param.r->description = _("Recharge raster map in m^3/s");

    param.output = G_define_option();
    param.output->key = "output";
    param.output->type = TYPE_STRING;
    param.output->required = YES;
    param.output->gisprompt = "new,grid3,3d-raster";
    param.output->description =
	_("The piezometric head result of the numerical calculation will be written to this map");

    param.vector = G_define_option();
    param.vector->key = "velocity";
    param.vector->type = TYPE_STRING;
    param.vector->required = NO;
    param.vector->gisprompt = "new,grid3,3d-raster";
    param.vector->description =
	_("Calculate the groundwater distance velocity vector field and write the x, y, and z components to maps named name_[xyz]. Name is basename for the new raster3d maps");


    param.dt = N_define_standard_option(N_OPT_CALC_TIME);
    param.maxit = N_define_standard_option(N_OPT_MAX_ITERATIONS);
    param.error = N_define_standard_option(N_OPT_ITERATION_ERROR);
    param.solver = N_define_standard_option(N_OPT_SOLVER_SYMM);
    param.sor = N_define_standard_option(N_OPT_SOR_VALUE);

    param.mask = G_define_flag();
    param.mask->key = 'm';
    param.mask->description = _("Use G3D mask (if exists)");

    param.sparse = G_define_flag();
    param.sparse->key = 's';
    param.sparse->description =
	_("Use a sparse linear equation system, only available with iterative solvers");
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
    G3D_Region region;
    N_gradient_field_3d *field = NULL;
    N_array_3d *xcomp = NULL;
    N_array_3d *ycomp = NULL;
    N_array_3d *zcomp = NULL;
    double error, sor;
    int maxit;
    const char *solver;
    int x, y, z, stat;
    char *buff = NULL;

    /* Initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster3d, voxel");
    module->description =
	_("Numerical calculation program for transient, confined groundwater flow in three dimensions");

    /* Get parameters from user */
    set_params();

    /* Have GRASS get pheads */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /*Set the maximum iterations */
    sscanf(param.maxit->answer, "%i", &(maxit));
    /*Set the calculation error break criteria */
    sscanf(param.error->answer, "%lf", &(error));
    sscanf(param.sor->answer, "%lf", &(sor));
    /*Set the solver */
    solver = param.solver->answer;

    if (strcmp(solver, N_SOLVER_DIRECT_LU) == 0 && param.sparse->answer)
	G_fatal_error(_("The direct LU solver do not work with sparse matrices"));
    if (strcmp(solver, N_SOLVER_DIRECT_GAUSS) == 0 && param.sparse->answer)
	G_fatal_error(_("The direct Gauss solver do not work with sparse matrices"));
    if (strcmp(solver, N_SOLVER_DIRECT_CHOLESKY) == 0 && param.sparse->answer)
	G_fatal_error(_("The direct cholesky solver do not work with sparse matrices"));



    /*Set the defaults */
    G3d_initDefaults();

    /*get the current region */
    G3d_getWindow(&region);

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
    if (param.sparse->answer) {
	les =
	    N_assemble_les_3d(N_SPARSE_LES, geom, data->status, data->phead,
			      (void *)data, call);
    }
    else {
	les =
	    N_assemble_les_3d(N_NORMAL_LES, geom, data->status, data->phead,
			      (void *)data, call);
    }


    /*solve the equation system */
    if (strcmp(solver, N_SOLVER_ITERATIVE_JACOBI) == 0)
	N_solver_jacobi(les, maxit, sor, error);

    if (strcmp(solver, N_SOLVER_ITERATIVE_SOR) == 0)
	N_solver_SOR(les, maxit, sor, error);

    if (strcmp(solver, N_SOLVER_ITERATIVE_CG) == 0)
	N_solver_cg(les, maxit, error);

    if (strcmp(solver, N_SOLVER_ITERATIVE_PCG) == 0)
	N_solver_pcg(les, maxit, error, N_DIAGONAL_PRECONDITION);

    if (strcmp(solver, N_SOLVER_ITERATIVE_BICGSTAB) == 0)
	N_solver_bicgstab(les, maxit, error);

    if (strcmp(solver, N_SOLVER_DIRECT_LU) == 0)
	N_solver_lu(les);

    if (strcmp(solver, N_SOLVER_DIRECT_GAUSS) == 0)
	N_solver_gauss(les);

    if (strcmp(solver, N_SOLVER_DIRECT_CHOLESKY) == 0)
	N_solver_cholesky(les);

    if (les == NULL)
	G_fatal_error(_("Unable to create and solve the linear equation system"));


    /*write the result to the output file and copy the values to the data->phead array */
    write_result(data->status, data->phead_start, data->phead, les->x,
		 &region, param.output->answer);
    N_free_les(les);

    /*Compute the the velocity field if required and write the result into three rast3d maps */
    if (param.vector->answer) {
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

	G_asprintf(&buff, "%s_x", param.vector->answer);
	N_write_array_3d_to_rast3d(xcomp, buff, 1);
	G_asprintf(&buff, "%s_y", param.vector->answer);
	N_write_array_3d_to_rast3d(ycomp, buff, 1);
	G_asprintf(&buff, "%s_z", param.vector->answer);
	N_write_array_3d_to_rast3d(zcomp, buff, 1);
	if (buff)
	    G_free(buff);

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
	     N_array_3d * phead, double *result, G3D_Region * region,
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
    map = G3d_openCellNew(name, DCELL_TYPE, G3D_USE_CACHE_DEFAULT, region);

    if (map == NULL)
	G3d_fatalError(_("Error opening g3d map <%s>"), name);

    G_message(_("Write the result to g3d map <%s>"), name);

    /*if requested set the Mask on */
    if (param.mask->answer) {
	if (G3d_maskFileExists()) {
	    changemask = 0;
	    if (G3d_maskIsOff(map)) {
		G3d_maskOn(map);
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
		    G3d_setNullValue(&d1, 1, DCELL_TYPE);
		}
		G3d_putDouble(map, x, y, z, d1);
	    }
	}
    }

    /*We set the Mask off, if it was off before */
    if (param.mask->answer) {
	if (G3d_maskFileExists())
	    if (G3d_maskIsOn(map) && changemask)
		G3d_maskOff(map);
    }

    if (!G3d_closeCell(map))
	G3d_fatalError(map, NULL, 0, _("Error closing g3d file"));

    return;
}
