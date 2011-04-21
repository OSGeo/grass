
/****************************************************************************
*
* MODULE:       r.solute.transport
*
* AUTHOR(S):    Original author 
*               Soeren Gebbert soerengebbert <at> gmx <dot> de
* 		27 11 2006 Berlin
* PURPOSE:      Calculates transient two dimensional solute transport
* 		in porous media
*
* COPYRIGHT:    (C) 2006-2009 by Soeren Gebbert, and the GRASS Development Team
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
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/gmath.h>
#include <grass/N_pde.h>
#include <grass/N_solute_transport.h>


/*- Parameters and global variables -----------------------------------------*/
typedef struct
{
    struct Option *output, *phead, *hc_x, *hc_y,
	*c, *status, *diff_x, *diff_y, *q, *cs, *r, *top, *nf, *cin,
	*bottom, *vector_x, *vector_y, *type, *dt, *maxit, *error, *solver, *sor,
	*al, *at, *loops, *stab;
    struct Flag *full_les;
    struct Flag *cfl;
} paramType;

paramType param;		/*Parameters */

/*- prototypes --------------------------------------------------------------*/
void set_params();		/*Fill the paramType structure */
void copy_result(N_array_2d * status, N_array_2d * c_start, double *result,
		 struct Cell_head *region, N_array_2d * target, int tflag);
N_les *create_solve_les(N_geom_data * geom, N_solute_transport_data2d * data,
			N_les_callback_2d * call, const char *solver, int maxit,
			double error, double sor);

/* ************************************************************************* */
/* Set up the arguments we are expecting ********************************** */
/* ************************************************************************* */
void set_params()
{
    param.c = G_define_standard_option(G_OPT_R_INPUT);
    param.c->key = "c";
    param.c->description = _("The initial concentration in [kg/m^3]");

    param.phead = G_define_standard_option(G_OPT_R_INPUT);
    param.phead->key = "phead";
    param.phead->description = _("The piezometric head in [m]");

    param.hc_x = G_define_standard_option(G_OPT_R_INPUT);
    param.hc_x->key = "hc_x";
    param.hc_x->description =
	_("The x-part of the hydraulic conductivity tensor in [m/s]");

    param.hc_y = G_define_standard_option(G_OPT_R_INPUT);
    param.hc_y->key = "hc_y";
    param.hc_y->description =
	_("The y-part of the hydraulic conductivity tensor in [m/s]");


    param.status = G_define_standard_option(G_OPT_R_INPUT);
    param.status->key = "status";
    param.status->description =
	_("The status for each cell, = 0 - inactive cell, 1 - active cell, "
	  "2 - dirichlet- and 3 - transfer boundary condition");

    param.diff_x = G_define_standard_option(G_OPT_R_INPUT);
    param.diff_x->key = "diff_x";
    param.diff_x->description =
	_("The x-part of the diffusion tensor in [m^2/s]");

    param.diff_y = G_define_standard_option(G_OPT_R_INPUT);
    param.diff_y->key = "diff_y";
    param.diff_y->description =
	_("The y-part of the diffusion tensor in [m^2/s]");

    param.q = G_define_standard_option(G_OPT_R_INPUT);
    param.q->key = "q";
    param.q->guisection = _("Water flow");
    param.q->required = NO;
    param.q->description = _("Groundwater sources and sinks in [m^3/s]");

    param.cin = G_define_standard_option(G_OPT_R_INPUT);
    param.cin->key = "cin";
    param.cin->required = NO;
    param.cin->gisprompt = "old,raster,raster";
    param.cin->guisection = _("Water flow");
    param.cin->description = _("Concentration sources and sinks bounded to a "
            "water source or sink in [kg/s]");


    param.cs = G_define_standard_option(G_OPT_R_INPUT);
    param.cs->key = "cs";
    param.cs->type = TYPE_STRING;
    param.cs->required = YES;
    param.cs->gisprompt = "old,raster,raster";
    param.cs->description = _("Concentration of inner sources and inner sinks in [kg/s] "
            "(i.e. a chemical reaction)");

    param.r = G_define_standard_option(G_OPT_R_INPUT);
    param.r->key = "rd";
    param.r->description = _("Retardation factor [-]");

    param.nf = G_define_standard_option(G_OPT_R_INPUT);
    param.nf->key = "nf";
    param.nf->description = _("Effective porosity [-]");

    param.top = G_define_standard_option(G_OPT_R_INPUT);
    param.top->key = "top";
    param.top->description = _("Top surface of the aquifer in [m]");

    param.bottom = G_define_standard_option(G_OPT_R_INPUT);
    param.bottom->key = "bottom";
    param.bottom->description = _("Bottom surface of the aquifer in [m]");

    param.output = G_define_standard_option(G_OPT_R_OUTPUT);
    param.output->description =	_("The resulting concentration of the numerical solute "
            "transport calculation will be written to this map. [kg/m^3]");

    param.vector_x = G_define_standard_option(G_OPT_R_OUTPUT);
    param.vector_x->key = "vx";
    param.vector_x->required = NO;
    param.vector_x->guisection = _("Water flow");
    param.vector_x->description =
	_("Calculate and store the groundwater filter velocity vector part in x direction [m/s]\n");

    param.vector_y = G_define_standard_option(G_OPT_R_OUTPUT);
    param.vector_y->key = "vy";
    param.vector_y->required = NO;
    param.vector_y->guisection = _("Water flow");
    param.vector_y->description =
	_("Calculate and store the groundwater filter velocity vector part in y direction [m/s]\n");

    param.dt = N_define_standard_option(N_OPT_CALC_TIME);
    param.maxit = N_define_standard_option(N_OPT_MAX_ITERATIONS);
    param.error = N_define_standard_option(N_OPT_ITERATION_ERROR);
    param.solver = N_define_standard_option(N_OPT_SOLVER_UNSYMM);
    param.sor = N_define_standard_option(N_OPT_SOR_VALUE);

    param.al = G_define_option();
    param.al->key = "al";
    param.al->type = TYPE_DOUBLE;
    param.al->required = NO;
    param.al->answer = "0.0";
    param.al->description =
	_("The longditudinal dispersivity length. [m]");

    param.at = G_define_option();
    param.at->key = "at";
    param.at->type = TYPE_DOUBLE;
    param.at->required = NO;
    param.at->answer = "0.0";
    param.at->description =
	_("The transversal dispersivity length. [m]");

    param.loops = G_define_option();
    param.loops->key = "loops";
    param.loops->type = TYPE_DOUBLE;
    param.loops->required = NO;
    param.loops->answer = "1";
    param.loops->description =
	_("Use this number of time loops if the CFL flag is off. The timestep will become dt/loops.");

    param.stab = G_define_option();
    param.stab->key = "stab";
    param.stab->type = TYPE_STRING;
    param.stab->required = NO;
    param.stab->answer = "full";
    param.stab->options = "full,exp";
    param.stab->guisection = "Stabelization";
    param.stab->description =
	_("Set the flow stabilizing scheme (full or exponential upwinding).");

    param.full_les = G_define_flag();
    param.full_les->key = 'f';
    param.full_les->guisection = "Solver";
    param.full_les->description = _("Use a full filled quadratic linear equation system,"
            " default is a sparse linear equation system.");

    param.cfl = G_define_flag();
    param.cfl->key = 'c';
    param.cfl->guisection = "Stabelization";
    param.cfl->description =
	_("Use the Courant-Friedrichs-Lewy criteria for time step calculation");
}

/* ************************************************************************* */
/* Main function *********************************************************** */
/* ************************************************************************* */
int main(int argc, char *argv[])
{
    struct GModule *module = NULL;
    N_solute_transport_data2d *data = NULL;
    N_geom_data *geom = NULL;
    N_les *les = NULL;
    N_les_callback_2d *call = NULL;
    struct Cell_head region;
    double error, sor;
    char *solver;
    int x, y, stat, i, maxit = 1;
    double loops = 1;
    N_array_2d *xcomp = NULL;
    N_array_2d *ycomp = NULL;
    N_array_2d *hc_x = NULL;
    N_array_2d *hc_y = NULL;
    N_array_2d *phead = NULL;

    double time_step, cfl, length, time_loops, time_sum;

    /* Initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    G_add_keyword(_("solute transport"));
    module->description =
	_("Numerical calculation program for transient, confined and unconfined "
            "solute transport in two dimensions");

    /* Get parameters from user */
    set_params();

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Make sure that the current projection is not lat/long */
    if ((G_projection() == PROJECTION_LL))
	G_fatal_error(_("Lat/Long location is not supported by %s. Please reproject map first."),
		      G_program_name());
    
    /*Set the maximum iterations */
    sscanf(param.maxit->answer, "%i", &(maxit));
    /*Set the calculation error break criteria */
    sscanf(param.error->answer, "%lf", &(error));
    sscanf(param.sor->answer, "%lf", &(sor));
    /*number of loops*/
    sscanf(param.loops->answer, "%lf", &(loops));    
    /*Set the solver */
    solver = param.solver->answer;

    if (strcmp(solver, G_MATH_SOLVER_DIRECT_LU) == 0 && !param.full_les->answer)
	G_fatal_error(_("The direct LU solver do not work with sparse matrices"));
    if (strcmp(solver, G_MATH_SOLVER_DIRECT_GAUSS) == 0 && !param.full_les->answer)
	G_fatal_error(_("The direct Gauss solver do not work with sparse matrices"));


    /*get the current region */
    G_get_set_window(&region);

    /*allocate the geometry structure for geometry and area calculation */
    geom = N_init_geom_data_2d(&region, geom);

    /*Set the function callback to the groundwater flow function */
    call = N_alloc_les_callback_2d();
    N_set_les_callback_2d_func(call, (*N_callback_solute_transport_2d));	/*solute_transport 2d */

    /*Allocate the groundwater flow data structure */
    data = N_alloc_solute_transport_data2d(geom->cols, geom->rows);

    /*Set the stabilizing scheme*/
    if (strncmp("full", param.stab->answer, 4) == 0) {
        data->stab = N_UPWIND_FULL;
    }
    if (strncmp("exp", param.stab->answer, 3) == 0) {
        data->stab = N_UPWIND_EXP;
    }
 
    /*the dispersivity lengths*/
    sscanf(param.al->answer, "%lf", &(data->al));
    sscanf(param.at->answer, "%lf", &(data->at));

    /*Set the calculation time */
    sscanf(param.dt->answer, "%lf", &(data->dt));

    /*read all input maps into the memory and take care of the
     * null values.*/
    N_read_rast_to_array_2d(param.c->answer, data->c);
    N_convert_array_2d_null_to_zero(data->c);
    N_read_rast_to_array_2d(param.c->answer, data->c_start);
    N_convert_array_2d_null_to_zero(data->c_start);
    N_read_rast_to_array_2d(param.status->answer, data->status);
    N_convert_array_2d_null_to_zero(data->status);
    N_read_rast_to_array_2d(param.diff_x->answer, data->diff_x);
    N_convert_array_2d_null_to_zero(data->diff_x);
    N_read_rast_to_array_2d(param.diff_y->answer, data->diff_y);
    N_convert_array_2d_null_to_zero(data->diff_y);
    N_read_rast_to_array_2d(param.q->answer, data->q);
    N_convert_array_2d_null_to_zero(data->q);
    N_read_rast_to_array_2d(param.nf->answer, data->nf);
    N_convert_array_2d_null_to_zero(data->nf);
    N_read_rast_to_array_2d(param.cs->answer, data->cs);
    N_convert_array_2d_null_to_zero(data->cs);
    N_read_rast_to_array_2d(param.top->answer, data->top);
    N_convert_array_2d_null_to_zero(data->top);
    N_read_rast_to_array_2d(param.bottom->answer, data->bottom);
    N_convert_array_2d_null_to_zero(data->bottom);
    N_read_rast_to_array_2d(param.r->answer, data->R);
    N_convert_array_2d_null_to_zero(data->R);

    if(param.cin->answer) {
      N_read_rast_to_array_2d(param.cin->answer, data->cin);
      N_convert_array_2d_null_to_zero(data->cin);
    }

    /*initiate the values for velocity calculation*/
    hc_x = N_alloc_array_2d(geom->cols, geom->rows, 1, DCELL_TYPE);
    hc_x = N_read_rast_to_array_2d(param.hc_x->answer, hc_x);
    N_convert_array_2d_null_to_zero(hc_x);
    hc_y = N_alloc_array_2d(geom->cols, geom->rows, 1, DCELL_TYPE);
    hc_y = N_read_rast_to_array_2d(param.hc_y->answer, hc_y);
    N_convert_array_2d_null_to_zero(hc_y);
    phead = N_alloc_array_2d(geom->cols, geom->rows, 1, DCELL_TYPE);
    phead = N_read_rast_to_array_2d(param.phead->answer, phead);
    N_convert_array_2d_null_to_zero(phead);

    /* Set the inactive values to zero, to assure a no flow boundary */
    for (y = 0; y < geom->rows; y++) {
	for (x = 0; x < geom->cols; x++) {
	    stat = (int)N_get_array_2d_d_value(data->status, x, y);
	    if (stat == N_CELL_INACTIVE) {	/*only inactive cells */
		N_put_array_2d_d_value(data->diff_x, x, y, 0);
		N_put_array_2d_d_value(data->diff_y, x, y, 0);
		N_put_array_2d_d_value(data->cs, x, y, 0);
		N_put_array_2d_d_value(data->q, x, y, 0);
	    }
	}
    }

    /*compute the velocities */
    N_math_array_2d(hc_x, data->nf, hc_x, N_ARRAY_DIV);
    N_math_array_2d(hc_y, data->nf, hc_y, N_ARRAY_DIV);
    N_compute_gradient_field_2d(phead, hc_x, hc_y, geom, data->grad);

    /*Now compute the dispersivity tensor*/
    N_calc_solute_transport_disptensor_2d(data);

    /***************************************/
    /*the Courant-Friedrichs-Lewy criteria */
    /*Compute the correct time step */
    if (geom->dx > geom->dy)
	length = geom->dx;
    else
	length = geom->dy;

    if (fabs(data->grad->max) > fabs(data->grad->min)) {
	cfl = (double)data->dt * fabs(data->grad->max) / length;
	time_step = 1*length / fabs(data->grad->max);
    }
    else {
	cfl = (double)data->dt * fabs(data->grad->min) / length;
	time_step = 1*length / fabs(data->grad->min);
    }

    G_message(_("The Courant-Friedrichs-Lewy criteria is %g it should be within [0:1]"), cfl);
    G_message(_("The largest stable time step is %g"), time_step);

    /*Set the number of inner loops and the time step*/
    if (data->dt > time_step && param.cfl->answer) {
	/*safe the user time step */
	time_sum = data->dt;
	time_loops = data->dt / time_step;
	time_loops = floor(time_loops) + 1;
	data->dt = data->dt / time_loops;
	G_message(_("Number of inner loops is %g"), time_loops);
	G_message(_("Time step for each loop %g"), data->dt);
    }
    else {
        if(data->dt > time_step)
	    G_warning(_("The time step is to large: %gs. The largest time step should be of size %gs."), data->dt, time_step);

	time_loops = loops;
	data->dt = data->dt / loops;
    }

    N_free_array_2d(phead);
    N_free_array_2d(hc_x);
    N_free_array_2d(hc_y);

     /*Compute for each time step*/
     for (i = 0; i < time_loops; i++) {
	 G_message(_("Time step %i with time sum %g"), i + 1, (i + 1)*data->dt);

	/*assemble the linear equation system  and solve it */
	les = create_solve_les(geom, data, call, solver, maxit, error, sor);

	/* copy the result into the c array for output */
	copy_result(data->status, data->c_start, les->x, &region, data->c, 1);
	N_convert_array_2d_null_to_zero(data->c_start);

        if (les)
	    N_free_les(les);

	/*Set the start array*/
	N_copy_array_2d(data->c, data->c_start);
	/*Set the transmission boundary*/
	N_calc_solute_transport_transmission_2d(data);

    }

    /*write the result to the output file */
    N_write_array_2d_to_rast(data->c, param.output->answer);

    /*Compute the the velocity field if required and write the result into three rast maps */
    if (param.vector_x->answer || param.vector_y->answer) {
	xcomp = N_alloc_array_2d(geom->cols, geom->rows, 1, DCELL_TYPE);
	ycomp = N_alloc_array_2d(geom->cols, geom->rows, 1, DCELL_TYPE);

	N_compute_gradient_field_components_2d(data->grad, xcomp, ycomp);

        if (param.vector_x->answer)
            N_write_array_2d_to_rast(xcomp, param.vector_x->answer);
        if (param.vector_y->answer)
            N_write_array_2d_to_rast(ycomp, param.vector_y->answer);

	if (xcomp)
	    N_free_array_2d(xcomp);
	if (ycomp)
	    N_free_array_2d(ycomp);
    }


    if (data)
	N_free_solute_transport_data2d(data);
    if (geom)
	N_free_geom_data(geom);
    if (call)
	G_free(call);

    return (EXIT_SUCCESS);
}


/* ************************************************************************* */
/* this function copies the result from the x vector to a N_array_2d array * */
/* ************************************************************************* */
void
copy_result(N_array_2d * status, N_array_2d * c_start, double *result,
	    struct Cell_head *region, N_array_2d * target, int tflag)
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
		d1 = N_get_array_2d_d_value(c_start, x, y);
		val = (DCELL) d1;
	    }
	    else if (tflag == 1 && stat == N_CELL_TRANSMISSION) {/*transmission cells*/
		d1 = N_get_array_2d_d_value(c_start, x, y);
		val = (DCELL) d1;
	    }
	    else {
		Rast_set_null_value(&val, 1, DCELL_TYPE);
	    }
	    N_put_array_2d_d_value(target, x, y, val);
	}
    }

    return;
}

/* *************************************************************** */
/* ***** create and solve the linear equation system ************* */
/* *************************************************************** */
N_les *create_solve_les(N_geom_data * geom, N_solute_transport_data2d * data,
			N_les_callback_2d * call, const char *solver, int maxit,
			double error, double sor)
{

    N_les *les;

    /*assemble the linear equation system */
    if (param.full_les->answer)
	les =
	    N_assemble_les_2d(N_NORMAL_LES, geom, data->status, data->c,
			      (void *)data, call);
    else
	les =
	    N_assemble_les_2d(N_SPARSE_LES, geom, data->status, data->c,
			      (void *)data, call);

    /*solve the equation system */
    if (strcmp(solver, G_MATH_SOLVER_ITERATIVE_JACOBI) == 0)
    {
        if (!param.full_les->answer)
            G_math_solver_sparse_jacobi(les->Asp, les->x, les->b, les->rows, maxit, sor, error);
        else
            G_math_solver_jacobi(les->A, les->x, les->b, les->rows, maxit, sor, error);
    }

    if (strcmp(solver, G_MATH_SOLVER_ITERATIVE_SOR) == 0)
    {
        if (!param.full_les->answer)
            G_math_solver_sparse_gs(les->Asp, les->x, les->b, les->rows, maxit, sor, error);
        else
            G_math_solver_gs(les->A, les->x, les->b, les->rows, maxit, sor, error);
    }

    if (strcmp(solver, G_MATH_SOLVER_ITERATIVE_BICGSTAB) == 0)
    {
        if (!param.full_les->answer)
            G_math_solver_sparse_bicgstab(les->Asp, les->x, les->b, les->rows,  maxit, error);
        else
            G_math_solver_bicgstab(les->A, les->x, les->b, les->rows, maxit, error);
    }

    if (strcmp(solver, G_MATH_SOLVER_DIRECT_LU) == 0)
	G_math_solver_lu(les->A, les->x, les->b, les->rows);

    if (strcmp(solver, G_MATH_SOLVER_DIRECT_GAUSS) == 0)
	G_math_solver_gauss(les->A, les->x, les->b, les->rows);

    if (les == NULL)
	G_fatal_error(_("Could not create and solve the linear equation system"));

    return les;
}

