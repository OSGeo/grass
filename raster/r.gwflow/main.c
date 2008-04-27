
/****************************************************************************
*
* MODULE:       r.gwflow 
*   	    	
* AUTHOR(S):    Original author 
*               Soeren Gebbert soerengebbert <at> gmx <dot> de
* 		27 11 2006 Berlin
* PURPOSE:      Calculates confiend and unconfined transient two dimensional groundwater flow
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
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/N_pde.h>
#include <grass/N_gwflow.h>


/*- Parameters and global variables -----------------------------------------*/
typedef struct
{
    struct Option *output, *phead, *status, *hc_x, *hc_y, *q, *s, *r, *top,
	*bottom, *vector, *type, *dt, *maxit, *error, *solver, *sor,
	*river_head, *river_bed, *river_leak, *drain_bed, *drain_leak;
    struct Flag *sparse;
} paramType;

paramType param;		/*Parameters */

/*- prototypes --------------------------------------------------------------*/
void set_params(void);		/*Fill the paramType structure */
void copy_result(N_array_2d * status, N_array_2d * phead_start, double *result,
		 struct Cell_head *region, N_array_2d * target);
N_les *create_solve_les(N_geom_data * geom, N_gwflow_data2d * data,
			N_les_callback_2d * call, const char *solver, int maxit,
			double error, double sor);

/* ************************************************************************* */
/* Set up the arguments we are expecting ********************************** */
/* ************************************************************************* */
void set_params(void)
{
    param.phead = G_define_option();
    param.phead->key = "phead";
    param.phead->type = TYPE_STRING;
    param.phead->required = YES;
    param.phead->gisprompt = "old,raster,raster";
    param.phead->description = _("The initial piezometric head in [m]");

    param.status = G_define_option();
    param.status->key = "status";
    param.status->type = TYPE_STRING;
    param.status->required = YES;
    param.status->gisprompt = "old,raster,raster";
    param.status->description = _("Boundary condition status, 0-inactive, 1-active, 2-dirichlet");

    param.hc_x = G_define_option();
    param.hc_x->key = "hc_x";
    param.hc_x->type = TYPE_STRING;
    param.hc_x->required = YES;
    param.hc_x->gisprompt = "old,raster,raster";
    param.hc_x->description =
	_("X-part of the hydraulic conductivity tensor in [m/s]");

    param.hc_y = G_define_option();
    param.hc_y->key = "hc_y";
    param.hc_y->type = TYPE_STRING;
    param.hc_y->required = YES;
    param.hc_y->gisprompt = "old,raster,raster";
    param.hc_y->description =
	_("Y-part of the hydraulic conductivity tensor in [m/s]");

    param.q = G_define_option();
    param.q->key = "q";
    param.q->type = TYPE_STRING;
    param.q->required = NO;
    param.q->gisprompt = "old,raster,raster";
    param.q->description = _("Water sources and sinks in [m^3/s]");

    param.s = G_define_option();
    param.s->key = "s";
    param.s->type = TYPE_STRING;
    param.s->required = YES;
    param.s->gisprompt = "old,raster,raster";
    param.s->description = _("Specific yield in [1/m]");

    param.r = G_define_option();
    param.r->key = "r";
    param.r->type = TYPE_STRING;
    param.r->required = NO;
    param.r->gisprompt = "old,raster,raster";
    param.r->description =
	_("Recharge map e.g: 6*10^-9 per cell in [m^3/s*m^2]");

    param.top = G_define_option();
    param.top->key = "top";
    param.top->type = TYPE_STRING;
    param.top->required = YES;
    param.top->gisprompt = "old,raster,raster";
    param.top->description = _("Top surface of the aquifer in [m]");

    param.bottom = G_define_option();
    param.bottom->key = "bottom";
    param.bottom->type = TYPE_STRING;
    param.bottom->required = YES;
    param.bottom->gisprompt = "old,raster,raster";
    param.bottom->description = _("Bottom surface of the aquifer in [m]");

    param.output = G_define_option();
    param.output->key = "output";
    param.output->type = TYPE_STRING;
    param.output->required = YES;
    param.output->gisprompt = "new,raster,raster";
    param.output->description =	_("The map storing the numerical result [m]");

    param.vector = G_define_option();
    param.vector->key = "velocity";
    param.vector->type = TYPE_STRING;
    param.vector->required = NO;
    param.vector->gisprompt = "new,raster,raster";
    param.vector->description =	_("Calculate the groundwater filter velocity vector field [m/s]\n"
         "and write the x, and y components to maps named name_[xy]");

    param.type = G_define_option();
    param.type->key = "type";
    param.type->type = TYPE_STRING;
    param.type->required = NO;
    param.type->answer = "confined";
    param.type->options = "confined,unconfined";
    param.type->description = _("The type of groundwater flow");

    /*Variants of the cauchy boundary condition */
    param.river_bed = G_define_option();
    param.river_bed->key = "river_bed";
    param.river_bed->type = TYPE_STRING;
    param.river_bed->required = NO;
    param.river_bed->gisprompt = "old,raster,raster";
    param.river_bed->description = _("The hight of the river bed in [m]");

    param.river_head = G_define_option();
    param.river_head->key = "river_head";
    param.river_head->type = TYPE_STRING;
    param.river_head->required = NO;
    param.river_head->gisprompt = "old,raster,raster";
    param.river_head->description =
	_("Water level (head) of the river with leakage connection in [m]");

    param.river_leak = G_define_option();
    param.river_leak->key = "river_leak";
    param.river_leak->type = TYPE_STRING;
    param.river_leak->required = NO;
    param.river_leak->gisprompt = "old,raster,raster";
    param.river_leak->description =
	_("The leakage coefficient of the river bed in [1/s].");

    param.drain_bed = G_define_option();
    param.drain_bed->key = "drain_bed";
    param.drain_bed->type = TYPE_STRING;
    param.drain_bed->required = NO;
    param.drain_bed->gisprompt = "old,raster,raster";
    param.drain_bed->description = _("The hight of the drainage bed in [m]");

    param.drain_leak = G_define_option();
    param.drain_leak->key = "drain_leak";
    param.drain_leak->type = TYPE_STRING;
    param.drain_leak->required = NO;
    param.drain_leak->gisprompt = "old,raster,raster";
    param.drain_leak->description =
	_("The leakage coefficient of the drainage bed in [1/s]");
 
    param.dt = N_define_standard_option(N_OPT_CALC_TIME);
    param.maxit = N_define_standard_option(N_OPT_MAX_ITERATIONS);
    param.error = N_define_standard_option(N_OPT_ITERATION_ERROR);
    param.solver = N_define_standard_option(N_OPT_SOLVER_SYMM);
    param.sor = N_define_standard_option(N_OPT_SOR_VALUE);   

    param.sparse = G_define_flag();
    param.sparse->key = 's';
    param.sparse->description =	_("Use a sparse matrix, only available with iterative solvers");

}

/* ************************************************************************* */
/* Main function *********************************************************** */
/* ************************************************************************* */
int main(int argc, char *argv[])
{
    struct GModule *module = NULL;
    N_gwflow_data2d *data = NULL;
    N_geom_data *geom = NULL;
    N_les *les = NULL;
    N_les_callback_2d *call = NULL;
    double *tmp_vect = NULL;
    struct Cell_head region;
    double error, sor, max_norm = 0, tmp;
    int maxit, i, inner_count = 0;
    char *solver;
    int x, y, stat;
    N_gradient_field_2d *field = NULL;
    N_array_2d *xcomp = NULL;
    N_array_2d *ycomp = NULL;
    char *buff = NULL;
    int with_river = 0, with_drain = 0;


    /* Initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster");
    module->description = _("Numerical calculation program for transient, confined and unconfined groundwater flow in two dimensions.");

    /* Get parameters from user */
    set_params();

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    /*Check the river  parameters */
    if (param.river_leak->answer == NULL && param.river_bed->answer == NULL &&
	param.river_head->answer == NULL) {
	with_river = 0;
    }
    else if (param.river_leak->answer != NULL && param.river_bed->answer != NULL
	     && param.river_head->answer != NULL) {
	with_river = 1;
    }
    else {
	G_fatal_error
	    (_("Please provide river_head, river_leak and river_bed maps"));
    }

    /*Check the drainage parameters */
    if (param.drain_leak->answer == NULL && param.drain_bed->answer == NULL) {
	with_drain = 0;
    }
    else if (param.drain_leak->answer != NULL &&
	     param.drain_bed->answer != NULL) {
	with_drain = 1;
    }
    else {
	G_fatal_error(_("Please provide drain_head and drain_leak maps"));
    }


    /*Set the maximum iterations */
    sscanf(param.maxit->answer, "%i", &(maxit));
    /*Set the calculation error break criteria */
    sscanf(param.error->answer, "%lf", &(error));
    sscanf(param.sor->answer, "%lf", &(sor));
    /*set the solver */
    solver = param.solver->answer;

    if (strcmp(solver, N_SOLVER_DIRECT_LU) == 0 && param.sparse->answer)
	G_fatal_error(_
		      ("The direct LU solver do not work with sparse matrices"));
    if (strcmp(solver, N_SOLVER_DIRECT_GAUSS) == 0 && param.sparse->answer)
	G_fatal_error(_
		      ("The direct Gauss solver do not work with sparse matrices"));
    if (strcmp(solver, N_SOLVER_DIRECT_CHOLESKY) == 0 && param.sparse->answer)
	G_fatal_error(_
		      ("The direct cholesky solver do not work with sparse matrices"));


    /*get the current region */
    G_get_set_window(&region);

    /*allocate the geometry structure  for geometry and area calculation */
    geom = N_init_geom_data_2d(&region, geom);

    /*Set the function callback to the groundwater flow function */
    call = N_alloc_les_callback_2d();
    N_set_les_callback_2d_func(call, (*N_callback_gwflow_2d));	/*gwflow 2d */

    /*Allocate the groundwater flow data structure */
    data =
	N_alloc_gwflow_data2d(geom->cols, geom->rows, with_river, with_drain);

    /* set the groundwater type */
    if (param.type->answer) {
	if (strncmp("unconfined", param.type->answer, 10) == 0) {
	    data->gwtype = N_GW_UNCONFINED;
	}
	else {
	    data->gwtype = N_GW_CONFINED;
	}
    }

    /*Set the calculation time */
    sscanf(param.dt->answer, "%lf", &(data->dt));
    G_message("Calculation time: %g", data->dt);

    /*read all input maps into the memory and take care of the
     * null values.*/
    N_read_rast_to_array_2d(param.phead->answer, data->phead);
    N_convert_array_2d_null_to_zero(data->phead);
    N_read_rast_to_array_2d(param.phead->answer, data->phead_start);
    N_convert_array_2d_null_to_zero(data->phead_start);
    N_read_rast_to_array_2d(param.status->answer, data->status);
    N_convert_array_2d_null_to_zero(data->status);
    N_read_rast_to_array_2d(param.hc_x->answer, data->hc_x);
    N_convert_array_2d_null_to_zero(data->hc_x);
    N_read_rast_to_array_2d(param.hc_y->answer, data->hc_y);
    N_convert_array_2d_null_to_zero(data->hc_y);
    N_read_rast_to_array_2d(param.q->answer, data->q);
    N_convert_array_2d_null_to_zero(data->q);
    N_read_rast_to_array_2d(param.s->answer, data->s);
    N_convert_array_2d_null_to_zero(data->s);
    N_read_rast_to_array_2d(param.top->answer, data->top);
    N_convert_array_2d_null_to_zero(data->top);
    N_read_rast_to_array_2d(param.bottom->answer, data->bottom);
    N_convert_array_2d_null_to_zero(data->bottom);

    /*river is optional */
    if (with_river) {
	N_read_rast_to_array_2d(param.river_bed->answer, data->river_bed);
	N_read_rast_to_array_2d(param.river_head->answer, data->river_head);
	N_read_rast_to_array_2d(param.river_leak->answer, data->river_leak);
	N_convert_array_2d_null_to_zero(data->river_bed);
	N_convert_array_2d_null_to_zero(data->river_head);
	N_convert_array_2d_null_to_zero(data->river_leak);
    }

    /*drainage is optional */
    if (with_drain) {
	N_read_rast_to_array_2d(param.drain_bed->answer, data->drain_bed);
	N_read_rast_to_array_2d(param.drain_leak->answer, data->drain_leak);
	N_convert_array_2d_null_to_zero(data->drain_bed);
	N_convert_array_2d_null_to_zero(data->drain_leak);
    }

    /*Recharge is optional */
    if (param.r->answer) {
	N_read_rast_to_array_2d(param.r->answer, data->r);
	N_convert_array_2d_null_to_zero(data->r);
    }

    /* Set the inactive values to zero, to assure a no flow boundary */
    for (y = 0; y < geom->rows; y++) {
	for (x = 0; x < geom->cols; x++) {
	    stat = (int)N_get_array_2d_d_value(data->status, x, y);
	    if (stat == N_CELL_INACTIVE) {	/*only inactive cells */
		N_put_array_2d_d_value(data->hc_x, x, y, 0);
		N_put_array_2d_d_value(data->hc_y, x, y, 0);
		N_put_array_2d_d_value(data->s, x, y, 0);
		N_put_array_2d_d_value(data->q, x, y, 0);
	    }
	}
    }


    /*assemble the linear equation system  and solve it */
    les = create_solve_les(geom, data, call, solver, maxit, error, sor);

    /* copy the result into the phead array for output or unconfined calculation */
    copy_result(data->status, data->phead_start, les->x, &region, data->phead);
    N_convert_array_2d_null_to_zero(data->phead);

  /****************************************************/
    /*explicite calculation of free groundwater surface */

  /****************************************************/
    if (data->gwtype == N_GW_UNCONFINED) {
	/* allocate memory and copy the result into a new temporal vector */
	if (!(tmp_vect = (double *)calloc(les->rows, sizeof(double))))
	    G_fatal_error(_("Out of memory"));

	/*copy data */
	for (i = 0; i < les->rows; i++)
	    tmp_vect[i] = les->x[i];

	/*count the number of inner iterations */
	inner_count = 0;

	do {
	    G_message(_("Calculation of unconfined groundwater flow loop %i\n"),
		      inner_count + 1);

	    /* we will allocate a new les for each loop */
	    if (les)
		N_free_les(les);

	    /*assemble the linear equation system  and solve it */
	    les = create_solve_les(geom, data, call, solver, maxit, error, sor);

	    /*calculate the maximum norm of the groundwater height difference */
	    tmp = 0;
	    max_norm = 0;
	    for (i = 0; i < les->rows; i++) {
		tmp = fabs(les->x[i] - tmp_vect[i]);
		if (max_norm < tmp)
		    max_norm = tmp;

		/*copy the result */
		tmp_vect[i] = les->x[i];
	    }

	    G_message(_("Maximum difference between this and last increment: %g"),
		      max_norm);

	    /* copy the result into the phead array */
	    copy_result(data->status, data->phead_start, les->x, &region,
			data->phead);
	    N_convert_array_2d_null_to_zero(data->phead);
	     /**/ inner_count++;
	}
	while (max_norm > 0.01 && inner_count < 50);

	if (tmp_vect)
	    free(tmp_vect);
    }

    /*write the result to the output file */
    N_write_array_2d_to_rast(data->phead, param.output->answer);

    /*release the memory */
    if (les)
	N_free_les(les);


    /*Compute the the velocity field if required and write the result into three rast maps */
    if (param.vector->answer) {
	field =
	    N_compute_gradient_field_2d(data->phead, data->hc_x, data->hc_y,
					geom, NULL);

	xcomp = N_alloc_array_2d(geom->cols, geom->rows, 1, DCELL_TYPE);
	ycomp = N_alloc_array_2d(geom->cols, geom->rows, 1, DCELL_TYPE);

	N_compute_gradient_field_components_2d(field, xcomp, ycomp);

	G_asprintf(&buff, "%s_x", param.vector->answer);
	N_write_array_2d_to_rast(xcomp, buff);
	G_asprintf(&buff, "%s_y", param.vector->answer);
	N_write_array_2d_to_rast(ycomp, buff);
	if (buff)
	    G_free(buff);

	if (xcomp)
	    N_free_array_2d(xcomp);
	if (ycomp)
	    N_free_array_2d(ycomp);
	if (field)
	    N_free_gradient_field_2d(field);
    }


    if (data)
	N_free_gwflow_data2d(data);
    if (geom)
	N_free_geom_data(geom);
    if (call)
	G_free(call);

    return (EXIT_SUCCESS);
}


/* ************************************************************************* */
/* this function copies the result from the x vector to a N_array_2d struct  */
/* ************************************************************************* */
void
copy_result(N_array_2d * status, N_array_2d * phead_start, double *result,
	    struct Cell_head *region, N_array_2d * target)
{
    int y, x, rows, cols, count, stat;
    double d1 = 0;
    DCELL val;

    rows = region->rows;
    cols = region->cols;

    count = 0;
    for (y = 0; y < rows; y++) {
	G_percent(y, rows - 1, 10);
	for (x = 0; x < cols; x++) {
	    stat = (int)N_get_array_2d_d_value(status, x, y);
	    if (stat == N_CELL_ACTIVE) {	/*only active cells */
		d1 = result[count];
		val = (DCELL) d1;
		count++;
	    }
	    else if (stat == N_CELL_DIRICHLET) {	/*dirichlet cells */
		d1 = N_get_array_2d_d_value(phead_start, x, y);
		val = (DCELL) d1;
		count++;
	    }
	    else {
		G_set_null_value(&val, 1, DCELL_TYPE);
	    }
	    N_put_array_2d_d_value(target, x, y, val);
	}
    }

    return;
}

/* *************************************************************** */
/* ***** create and solve the linear equation system ************* */
/* *************************************************************** */
N_les *create_solve_les(N_geom_data * geom, N_gwflow_data2d * data,
			N_les_callback_2d * call, const char *solver, int maxit,
			double error, double sor)
{

    N_les *les;

    /*assemble the linear equation system */
    if (param.sparse->answer)
	les =
	    N_assemble_les_2d_dirichlet(N_SPARSE_LES, geom, data->status,
					data->phead, (void *)data, call);
    else
	les =
	    N_assemble_les_2d_dirichlet(N_NORMAL_LES, geom, data->status,
					data->phead, (void *)data, call);

    N_les_integrate_dirichlet_2d(les, geom, data->status, data->phead);

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

    if (strcmp(solver, N_SOLVER_DIRECT_CHOLESKY) == 0)
	N_solver_cholesky(les);

    if (strcmp(solver, N_SOLVER_DIRECT_GAUSS) == 0)
	N_solver_gauss(les);

    if (les == NULL)
	G_fatal_error(_("Unable to create and solve the linear equation system"));

    return les;
}
