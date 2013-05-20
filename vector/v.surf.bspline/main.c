
/**********************************************************************
 *
 * MODULE:       v.surf.bspline
 *
 * AUTHOR(S):    Roberto Antolin & Gonzalo Moreno
 *               update for grass7 by Markus Metz
 *
 * PURPOSE:      Spline Interpolation
 *
 * COPYRIGHT:    (C) 2006 by Politecnico di Milano -
 *			     Polo Regionale di Como
 *
 *               This program is free software under the
 *               GNU General Public License (>=v2).
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **********************************************************************/

/* INCLUDES */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "bspline.h"
#include <grass/N_pde.h>

#define SEGSIZE 64

/* GLOBAL VARIABLES */
int bspline_field;
char *bspline_column;

/*--------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    /* Variable declarations */
    int nsply, nsplx, nrows, ncols, nsplx_adj, nsply_adj;
    int nsubregion_col, nsubregion_row, subregion_row, subregion_col;
    int subregion = 0, nsubregions = 0;
    int last_row, last_column, grid, bilin, ext, flag_auxiliar, cross;	/* booleans */
    double stepN, stepE, lambda, mean;
    double N_extension, E_extension, edgeE, edgeN;

    const char *mapset, *drv, *db, *vector, *map;
    char table_name[GNAME_MAX], title[64];
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];

    int dim_vect, nparameters, BW;
    int *lineVect;		/* Vector restoring primitive's ID */
    double *TN, *Q, *parVect;	/* Interpolating and least-square vectors */
    double **N, **obsVect;	/* Interpolation and least-square matrix */

    SEGMENT out_seg, mask_seg;
    const char *out_file, *mask_file;
    int out_fd, mask_fd;
    double seg_size;
    int seg_mb, segments_in_memory;
    int have_mask;

    /* Structs declarations */
    int raster;
    struct Map_info In, In_ext, Out;
    struct History history;

    struct GModule *module;
    struct Option *in_opt, *in_ext_opt, *out_opt, *out_map_opt, *stepE_opt,
	*stepN_opt, *lambda_f_opt, *type_opt, *dfield_opt, *col_opt, *mask_opt,
	*memory_opt, *solver, *error, *iter;
    struct Flag *cross_corr_flag, *spline_step_flag, *withz_flag;

    struct Reg_dimens dims;
    struct Cell_head elaboration_reg, original_reg;
    struct bound_box general_box, overlap_box, original_box;

    struct Point *observ;
    struct line_cats *Cats;
    dbCatValArray cvarr;

    int nrec, ctype = 0;
    struct field_info *Fi;
    dbDriver *driver, *driver_cats;

    /*----------------------------------------------------------------*/
    /* Options declarations */
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("surface"));
    G_add_keyword(_("interpolation"));
    G_add_keyword(_("LIDAR"));
    module->description =
	_("Performs bicubic or bilinear spline interpolation with Tykhonov regularization.");

    cross_corr_flag = G_define_flag();
    cross_corr_flag->key = 'c';
    cross_corr_flag->description =
	_("Find the best Tykhonov regularizing parameter using a \"leave-one-out\" cross validation method");

    spline_step_flag = G_define_flag();
    spline_step_flag->key = 'e';
    spline_step_flag->label = _("Estimate point density and distance");
    spline_step_flag->description =
	_("Estimate point density and distance for the input vector points within the current region extends and quit");

    withz_flag = G_define_flag();
    withz_flag->key = 'z';
    withz_flag->description = _("Use z coordinates for approximation (3D vector maps only)");
    withz_flag->guisection = _("Settings");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);
    in_opt->label = _("Name of input vector point map");
    
    dfield_opt = G_define_standard_option(G_OPT_V_FIELD);

    in_ext_opt = G_define_standard_option(G_OPT_V_INPUT);
    in_ext_opt->key = "sparse_input";
    in_ext_opt->required = NO;
    in_ext_opt->label =
	_("Name of input vector map with sparse points");

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    out_opt->required = NO;

    out_map_opt = G_define_standard_option(G_OPT_R_OUTPUT);
    out_map_opt->key = "raster_output";
    out_map_opt->required = NO;

    mask_opt = G_define_standard_option(G_OPT_R_INPUT);
    mask_opt->key = "mask";
    mask_opt->label = _("Raster map to use for masking (applies to raster output only)");
    mask_opt->description = _("Only cells that are not NULL and not zero are interpolated");
    mask_opt->required = NO;

    stepE_opt = G_define_option();
    stepE_opt->key = "sie";
    stepE_opt->type = TYPE_DOUBLE;
    stepE_opt->required = NO;
    stepE_opt->answer = "4";
    stepE_opt->description =
	_("Length of each spline step in the east-west direction");
    stepE_opt->guisection = _("Settings");

    stepN_opt = G_define_option();
    stepN_opt->key = "sin";
    stepN_opt->type = TYPE_DOUBLE;
    stepN_opt->required = NO;
    stepN_opt->answer = "4";
    stepN_opt->description =
	_("Length of each spline step in the north-south direction");
    stepN_opt->guisection = _("Settings");

    type_opt = G_define_standard_option(G_OPT_R_INTERP_TYPE);
    type_opt->description = _("Spline interpolation algorithm");
    type_opt->options = "linear,cubic";
    type_opt->answer = "linear";
    type_opt->guisection = _("Settings");

    lambda_f_opt = G_define_option();
    lambda_f_opt->key = "lambda_i";
    lambda_f_opt->type = TYPE_DOUBLE;
    lambda_f_opt->required = NO;
    lambda_f_opt->description = _("Tykhonov regularization parameter (affects smoothing)");
    lambda_f_opt->answer = "0.01";
    lambda_f_opt->guisection = _("Settings");

    col_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    col_opt->key = "column";
    col_opt->required = NO;
    col_opt->description =
	_("Name of attribute column with values to approximate");
    col_opt->guisection = _("Settings");

    solver = N_define_standard_option(N_OPT_SOLVER_SYMM);
    solver->options = "cholesky,cg";
    solver->answer = "cholesky";

    iter = N_define_standard_option(N_OPT_MAX_ITERATIONS);

    error = N_define_standard_option(N_OPT_ITERATION_ERROR);

    memory_opt = G_define_option();
    memory_opt->key = "memory";
    memory_opt->type = TYPE_INTEGER;
    memory_opt->required = NO;
    memory_opt->answer = "300";
    memory_opt->description = _("Maximum memory to be used for raster output (in MB)");

    /*----------------------------------------------------------------*/
    /* Parsing */
    G_gisinit(argv[0]);
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    vector = out_opt->answer;
    map = out_map_opt->answer;

    if (vector && map)
	G_fatal_error(_("Choose either vector or raster output, not both"));

    if (!vector && !map && !cross_corr_flag->answer)
	G_fatal_error(_("No raster or vector or cross-validation output"));

    if (!strcmp(type_opt->answer, "linear"))
	bilin = P_BILINEAR;
    else
	bilin = P_BICUBIC;

    stepN = atof(stepN_opt->answer);
    stepE = atof(stepE_opt->answer);
    lambda = atof(lambda_f_opt->answer);

    flag_auxiliar = FALSE;

    drv = db_get_default_driver_name();
    if (!drv)
        G_fatal_error(_("No default DB driver defined"));
    db = db_get_default_database_name();
    if (!db)
        G_fatal_error(_("No default DB defined"));
    
    /* Set auxiliary table's name */
    if (vector) {
	if (G_name_is_fully_qualified(out_opt->answer, xname, xmapset)) {
	    sprintf(table_name, "%s_aux", xname);
	}
	else
	    sprintf(table_name, "%s_aux", out_opt->answer);
    }

    /* Something went wrong in a previous v.surf.bspline execution */
    if (db_table_exists(drv, db, table_name)) {
	/* Start driver and open db */
	driver = db_start_driver_open_database(drv, db);
	if (driver == NULL)
	    G_fatal_error(_("No database connection for driver <%s> is defined. Run db.connect."),
			  drv);
	if (P_Drop_Aux_Table(driver, table_name) != DB_OK)
	    G_fatal_error(_("Old auxiliary table could not be dropped"));
	db_close_database_shutdown_driver(driver);
    }

    /* Open input vector */
    if ((mapset = G_find_vector2(in_opt->answer, "")) == NULL)
	G_fatal_error(_("Vector map <%s> not found"), in_opt->answer);

    Vect_set_open_level(1);	/* WITHOUT TOPOLOGY */
    if (1 > Vect_open_old(&In, in_opt->answer, mapset))
	G_fatal_error(_("Unable to open vector map <%s> at the topological level"),
		      in_opt->answer);
    
    bspline_field = Vect_get_field_number(&In, dfield_opt->answer);
    bspline_column = col_opt->answer;

    /* check availability of z values */
    if (withz_flag->answer && !Vect_is_3d(&In)) {
	G_fatal_error(_("Input vector is not 3D, can not use z coordinates"));
    }
    else if (!withz_flag->answer && (bspline_field <= 0 || bspline_column == NULL))
	G_fatal_error(_("Option '%s' with z values or '-%c' flag must be given"),
                      col_opt->key, withz_flag->key);

    if (withz_flag->answer)
	bspline_field = 0;

    /* Estimate point density and mean distance for current region */
    if (spline_step_flag->answer) {
	double dens, dist;
	if (P_estimate_splinestep(&In, &dens, &dist) == 0) {
	    fprintf(stdout, _("Estimated point density: %.4g"), dens);
            fprintf(stdout, _("Estimated mean distance between points: %.4g"), dist);
	}
	else {
	    fprintf(stdout, _("No points in current region"));
	}
        
	Vect_close(&In);
	exit(EXIT_SUCCESS);
    }

    /*----------------------------------------------------------------*/
    /* Cross-correlation begins */
    if (cross_corr_flag->answer) {
	G_debug(1, "CrossCorrelation()");
	cross = cross_correlation(&In, stepE, stepN);

	if (cross != TRUE)
	    G_fatal_error(_("Cross validation didn't finish correctly"));
	else {
	    G_debug(1, "Cross validation finished correctly");

	    Vect_close(&In);

	    G_done_msg(_("Cross validation finished for sie = %f and sin = %f"), stepE, stepN);
	    exit(EXIT_SUCCESS);
	}
    }

    /* Open input ext vector */
    ext = FALSE;
    if (in_ext_opt->answer) {
	ext = TRUE;
	G_message(_("Vector map <%s> of sparse points will be interpolated"),
		  in_ext_opt->answer);

	if ((mapset = G_find_vector2(in_ext_opt->answer, "")) == NULL)
	    G_fatal_error(_("Vector map <%s> not found"), in_ext_opt->answer);

	Vect_set_open_level(1);	/* WITHOUT TOPOLOGY */
	if (1 > Vect_open_old(&In_ext, in_ext_opt->answer, mapset))
	    G_fatal_error(_("Unable to open vector map <%s> at the topological level"),
			  in_opt->answer);
    }

    /* Open output map */
    /* vector output */
    if (vector && !map) {
	if (strcmp(drv, "dbf") == 0)
	    G_fatal_error(_("Sorry, the <%s> driver is not compatible with "
			  "the vector output of this module. "
			  "Try with raster output or another driver."), drv);

	Vect_check_input_output_name(in_opt->answer, out_opt->answer,
				     G_FATAL_EXIT);
	grid = FALSE;

	if (0 > Vect_open_new(&Out, out_opt->answer, WITH_Z))
	    G_fatal_error(_("Unable to create vector map <%s>"),
			  out_opt->answer);

	/* Copy vector Head File */
	if (ext == FALSE) {
	    Vect_copy_head_data(&In, &Out);
	    Vect_hist_copy(&In, &Out);
	}
	else {
	    Vect_copy_head_data(&In_ext, &Out);
	    Vect_hist_copy(&In_ext, &Out);
	}
	Vect_hist_command(&Out);

	G_verbose_message(_("Points in input vector map <%s> will be interpolated"),
                          vector);
    }

    /* raster output */
    raster = -1;
    Rast_set_fp_type(DCELL_TYPE);
    if (!vector && map) {
	grid = TRUE;
	raster = Rast_open_fp_new(out_map_opt->answer);

	G_verbose_message(_("Cells for raster map <%s> will be interpolated"),
                          map);
    }

    /* read z values from attribute table */
    if (bspline_field > 0) {
	db_CatValArray_init(&cvarr);
	Fi = Vect_get_field(&In, bspline_field);
	if (Fi == NULL)
	    G_fatal_error(_("Cannot read layer info"));

	driver_cats = db_start_driver_open_database(Fi->driver, Fi->database);
	/*G_debug (0, _("driver=%s db=%s"), Fi->driver, Fi->database); */

	if (driver_cats == NULL)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, Fi->driver);

	nrec =
	    db_select_CatValArray(driver_cats, Fi->table, Fi->key,
				  col_opt->answer, NULL, &cvarr);
	G_debug(3, "nrec = %d", nrec);

	ctype = cvarr.ctype;
	if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE)
	    G_fatal_error(_("Column type not supported"));

	if (nrec < 0)
	    G_fatal_error(_("Unable to select data from table"));

	G_message(_("[%d] records selected from table"), nrec);

	db_close_database_shutdown_driver(driver_cats);
    }

    /*----------------------------------------------------------------*/
    /* Interpolation begins */
    G_debug(1, "Interpolation()");

    /* Open driver and database */
    driver = db_start_driver_open_database(drv, db);
    if (driver == NULL)
	G_fatal_error(_("No database connection for driver <%s> is defined. "
			"Run db.connect."), drv);

    /* Create auxiliary table */
    if (vector) {
	if ((flag_auxiliar = P_Create_Aux4_Table(driver, table_name)) == FALSE) {
	    P_Drop_Aux_Table(driver, table_name);
	    G_fatal_error(_("Interpolation: Creating table: "
			    "It was impossible to create table <%s>."),
			  table_name);
	}
	/* db_create_index2(driver, table_name, "ID"); */
	/* sqlite likes that */
	db_close_database_shutdown_driver(driver);
	driver = db_start_driver_open_database(drv, db);
    }

    /* Setting regions and boxes */
    G_debug(1, "Interpolation: Setting regions and boxes");
    G_get_window(&original_reg);
    G_get_window(&elaboration_reg);
    Vect_region_box(&original_reg, &original_box);
    Vect_region_box(&elaboration_reg, &overlap_box);
    Vect_region_box(&elaboration_reg, &general_box);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* Alloc raster matrix */
    have_mask = 0;
    out_file = mask_file = NULL;
    out_fd = mask_fd = -1;
    if (grid == TRUE) {
	int row;
	DCELL *drastbuf;

	seg_mb = atoi(memory_opt->answer);
	if (seg_mb < 3)
	    G_fatal_error(_("Memory in MB must be >= 3"));

	if (mask_opt->answer)
	    seg_size = sizeof(double) + sizeof(char);
	else
	    seg_size = sizeof(double);

	seg_size = (seg_size * SEGSIZE * SEGSIZE) / (1 << 20);
	segments_in_memory = seg_mb / seg_size + 0.5;
	G_debug(1, "%d %dx%d segments held in memory", segments_in_memory, SEGSIZE, SEGSIZE);

	out_file = G_tempfile();
	out_fd = creat(out_file, 0666);
	if (segment_format(out_fd, nrows, ncols, SEGSIZE, SEGSIZE, sizeof(double)) != 1)
	    G_fatal_error(_("Can not create temporary file"));
	close(out_fd);

	out_fd = open(out_file, 2);
	if (segment_init(&out_seg, out_fd, segments_in_memory) != 1)
	    G_fatal_error(_("Can not initialize temporary file"));

	/* initialize output */
	G_message(_("Initializing output..."));

	drastbuf = Rast_allocate_buf(DCELL_TYPE);
	Rast_set_d_null_value(drastbuf, ncols);
	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);
	    segment_put_row(&out_seg, drastbuf, row);
	}
	G_percent(row, nrows, 2);

	if (mask_opt->answer) {
	    int row, col, maskfd;
	    DCELL dval, *drastbuf;
	    char mask_val;
	    
	    G_message(_("Load masking map"));

	    mask_file = G_tempfile();
	    mask_fd = creat(mask_file, 0666);
	    if (segment_format(mask_fd, nrows, ncols, SEGSIZE, SEGSIZE, sizeof(char)) != 1)
		G_fatal_error(_("Can not create temporary file"));
	    close(mask_fd);

	    mask_fd = open(mask_file, 2);
	    if (segment_init(&mask_seg, mask_fd, segments_in_memory) != 1)
		G_fatal_error(_("Can not initialize temporary file"));

	    maskfd = Rast_open_old(mask_opt->answer, "");
	    drastbuf = Rast_allocate_buf(DCELL_TYPE);

	    for (row = 0; row < nrows; row++) {
		G_percent(row, nrows, 2);
		Rast_get_d_row(maskfd, drastbuf, row);
		for (col = 0; col < ncols; col++) {
		    dval = drastbuf[col];
		    if (Rast_is_d_null_value(&dval) || dval == 0)
			mask_val = 0;
		    else
			mask_val = 1;
			
		    segment_put(&mask_seg, &mask_val, row, col);
		}
	    }

	    G_percent(row, nrows, 2);
	    G_free(drastbuf);
	    Rast_close(maskfd);
	    
	    have_mask = 1;
	}
    }

    /*------------------------------------------------------------------
      | Subdividing and working with tiles: 									
      | Each original region will be divided into several subregions. 
      | Each one will be overlaped by its neighbouring subregions. 
      | The overlapping is calculated as a fixed OVERLAP_SIZE times
      | the largest spline step plus 2 * edge
      ----------------------------------------------------------------*/

    /* Fixing parameters of the elaboration region */
    P_zero_dim(&dims);		/* Set dim struct to zero */

    nsplx_adj = NSPLX_MAX;
    nsply_adj = NSPLY_MAX;
    if (stepN > stepE)
	dims.overlap = OVERLAP_SIZE * stepN;
    else
	dims.overlap = OVERLAP_SIZE * stepE;
    P_get_edge(bilin, &dims, stepE, stepN);
    P_set_dim(&dims, stepE, stepN, &nsplx_adj, &nsply_adj);

    G_verbose_message(_("Adjusted EW splines %d"), nsplx_adj);
    G_verbose_message(_("Adjusted NS splines %d"), nsply_adj);

    /* calculate number of subregions */
    edgeE = dims.ew_size - dims.overlap - 2 * dims.edge_v;
    edgeN = dims.sn_size - dims.overlap - 2 * dims.edge_h;

    N_extension = original_reg.north - original_reg.south;
    E_extension = original_reg.east - original_reg.west;

    nsubregion_col = ceil(E_extension / edgeE) + 0.5;
    nsubregion_row = ceil(N_extension / edgeN) + 0.5;

    if (nsubregion_col < 0)
	nsubregion_col = 0;
    if (nsubregion_row < 0)
	nsubregion_row = 0;

    nsubregions = nsubregion_row * nsubregion_col;

    /* Creating line and categories structs */
    Cats = Vect_new_cats_struct();
    Vect_cat_set(Cats, 1, 0);

    subregion_row = 0;
    elaboration_reg.south = original_reg.north;
    last_row = FALSE;

    while (last_row == FALSE) {	/* For each subregion row */
	subregion_row++;
	P_set_regions(&elaboration_reg, &general_box, &overlap_box, dims,
		      GENERAL_ROW);

	if (elaboration_reg.north > original_reg.north) {	/* First row */

	    P_set_regions(&elaboration_reg, &general_box, &overlap_box, dims,
			  FIRST_ROW);
	}

	if (elaboration_reg.south <= original_reg.south) {	/* Last row */

	    P_set_regions(&elaboration_reg, &general_box, &overlap_box, dims,
			  LAST_ROW);
	    last_row = TRUE;
	}

	nsply =
	    ceil((elaboration_reg.north -
		  elaboration_reg.south) / stepN) + 0.5;
	G_debug(1, "Interpolation: nsply = %d", nsply);
	/*
	if (nsply > NSPLY_MAX)
	    nsply = NSPLY_MAX;
	*/
	elaboration_reg.east = original_reg.west;
	last_column = FALSE;
	subregion_col = 0;

	/* TODO: process each subregion using its own thread (via OpenMP or pthreads) */
	/*     I'm not sure about pthreads, but you can tell OpenMP to start all at the
		same time and it will keep num_workers supplied with the next job as free
		cpus become available */
	while (last_column == FALSE) {	/* For each subregion column */
	    int npoints = 0;
	    /* needed for sparse points interpolation */
	    int npoints_ext, *lineVect_ext = NULL;
	    double **obsVect_ext;	/*, mean_ext = .0; */
	    struct Point *observ_ext;

	    subregion_col++;
	    subregion++;
	    if (nsubregions > 1)
		G_message(_("Subregion %d of %d..."), subregion, nsubregions);
            
	    P_set_regions(&elaboration_reg, &general_box, &overlap_box, dims,
			  GENERAL_COLUMN);

	    if (elaboration_reg.west < original_reg.west) {	/* First column */

		P_set_regions(&elaboration_reg, &general_box, &overlap_box,
			      dims, FIRST_COLUMN);
	    }

	    if (elaboration_reg.east >= original_reg.east) {	/* Last column */

		P_set_regions(&elaboration_reg, &general_box, &overlap_box,
			      dims, LAST_COLUMN);
		last_column = TRUE;
	    }
	    nsplx =
		ceil((elaboration_reg.east -
		      elaboration_reg.west) / stepE) + 0.5;
	    G_debug(1, "Interpolation: nsplx = %d", nsplx);
	    /*
	    if (nsplx > NSPLX_MAX)
		nsplx = NSPLX_MAX;
	    */
	    G_debug(1, "Interpolation: (%d,%d): subregion bounds",
		    subregion_row, subregion_col);
	    G_debug(1, "Interpolation: \t\tNORTH:%.2f\t",
		    elaboration_reg.north);
	    G_debug(1, "Interpolation: WEST:%.2f\t\tEAST:%.2f",
		    elaboration_reg.west, elaboration_reg.east);
	    G_debug(1, "Interpolation: \t\tSOUTH:%.2f",
		    elaboration_reg.south);

#ifdef DEBUG_SUBREGIONS
	    fprintf(stdout, "B 5\n");
	    fprintf(stdout, " %.11g %.11g\n", elaboration_reg.east, elaboration_reg.north);
	    fprintf(stdout, " %.11g %.11g\n", elaboration_reg.west, elaboration_reg.north);
	    fprintf(stdout, " %.11g %.11g\n", elaboration_reg.west, elaboration_reg.south);
	    fprintf(stdout, " %.11g %.11g\n", elaboration_reg.east, elaboration_reg.south);
	    fprintf(stdout, " %.11g %.11g\n", elaboration_reg.east, elaboration_reg.north);
	    fprintf(stdout, "C 1 1\n");
	    fprintf(stdout, " %.11g %.11g\n", (elaboration_reg.west + elaboration_reg.east) / 2,
					      (elaboration_reg.south + elaboration_reg.north) / 2);
	    fprintf(stdout, " 1 %d\n", subregion);
#endif



	    /* reading points in interpolation region */
	    dim_vect = nsplx * nsply;
	    observ_ext = NULL;
	    if (grid == FALSE && ext == TRUE) {
		observ_ext =
		    P_Read_Vector_Region_Map(&In_ext,
					     &elaboration_reg,
					     &npoints_ext, dim_vect,
					     1);
	    }
	    else
		npoints_ext = 1;
		
	    if (grid == TRUE && have_mask) {
		/* any unmasked cells in general region ? */
		mean = 0;
		observ_ext =
		    P_Read_Raster_Region_masked(&mask_seg, &original_reg,
					     original_box, general_box,
					     &npoints_ext, dim_vect, mean);
	    }

	    observ = NULL;
	    if (npoints_ext > 0) {
		observ =
		    P_Read_Vector_Region_Map(&In, &elaboration_reg, &npoints,
					     dim_vect, bspline_field);
	    }
	    else
		npoints = 1;

	    G_debug(1,
		    "Interpolation: (%d,%d): Number of points in <elaboration_box> is %d",
		    subregion_row, subregion_col, npoints);
            if (npoints > 0)
                G_verbose_message(_("%d points found in this subregion"), npoints);
	    /* only interpolate if there are any points in current subregion */
	    if (npoints > 0 && npoints_ext > 0) {
		int i;

		nparameters = nsplx * nsply;
		BW = P_get_BandWidth(bilin, nsply);

		/* Least Squares system */
		N = G_alloc_matrix(nparameters, BW);	/* Normal matrix */
		TN = G_alloc_vector(nparameters);	/* vector */
		parVect = G_alloc_vector(nparameters);	/* Parameters vector */
		obsVect = G_alloc_matrix(npoints, 3);	/* Observation vector */
		Q = G_alloc_vector(npoints);	/* "a priori" var-cov matrix */
		lineVect = G_alloc_ivector(npoints);	/*  */

		for (i = 0; i < npoints; i++) {	/* Setting obsVect vector & Q matrix */
		    double dval;

		    Q[i] = 1;	/* Q=I */
		    lineVect[i] = observ[i].lineID;
		    obsVect[i][0] = observ[i].coordX;
		    obsVect[i][1] = observ[i].coordY;

		    /* read z coordinates from attribute table */
		    if (bspline_field > 0) {
			int cat, ival, ret;

			cat = observ[i].cat;
			if (cat < 0)
			    continue;

			if (ctype == DB_C_TYPE_INT) {
			    ret =
				db_CatValArray_get_value_int(&cvarr, cat,
							     &ival);
			    obsVect[i][2] = ival;
			    observ[i].coordZ = ival;
			}
			else {	/* DB_C_TYPE_DOUBLE */
			    ret =
				db_CatValArray_get_value_double(&cvarr, cat,
								&dval);
			    obsVect[i][2] = dval;
			    observ[i].coordZ = dval;
			}
			if (ret != DB_OK) {
			    G_warning(_("Interpolation: (%d,%d): No record for point (cat = %d)"),
				      subregion_row, subregion_col, cat);
			    continue;
			}
		    }
		    /* use z coordinates of 3D vector */
		    else {
			obsVect[i][2] = observ[i].coordZ;
		    }
		}

		/* Mean calculation for every point */
		mean = P_Mean_Calc(&elaboration_reg, observ, npoints);

		G_debug(1, "Interpolation: (%d,%d): mean=%lf",
			subregion_row, subregion_col, mean);

		G_free(observ);

		for (i = 0; i < npoints; i++)
		    obsVect[i][2] -= mean;

		/* Bilinear interpolation */
		if (bilin) {
		    G_debug(1,
			    "Interpolation: (%d,%d): Bilinear interpolation...",
			    subregion_row, subregion_col);
		    normalDefBilin(N, TN, Q, obsVect, stepE, stepN, nsplx,
				   nsply, elaboration_reg.west,
				   elaboration_reg.south, npoints,
				   nparameters, BW);
		    nCorrectGrad(N, lambda, nsplx, nsply, stepE, stepN);
		}
		/* Bicubic interpolation */
		else {
		    G_debug(1,
			    "Interpolation: (%d,%d): Bicubic interpolation...",
			    subregion_row, subregion_col);
		    normalDefBicubic(N, TN, Q, obsVect, stepE, stepN, nsplx,
				     nsply, elaboration_reg.west,
				     elaboration_reg.south, npoints,
				     nparameters, BW);
		    nCorrectGrad(N, lambda, nsplx, nsply, stepE, stepN);
		}

                if(G_strncasecmp(solver->answer, "cg", 2) == 0)
                    G_math_solver_cg_sband(N, parVect, TN, nparameters, BW, atoi(iter->answer), atof(error->answer));
		else
		    G_math_solver_cholesky_sband(N, parVect, TN, nparameters, BW);


		G_free_matrix(N);
		G_free_vector(TN);
		G_free_vector(Q);

		if (grid == TRUE) {	/* GRID INTERPOLATION ==> INTERPOLATION INTO A RASTER */
		    G_debug(1, "Interpolation: (%d,%d): Regular_Points...",
			    subregion_row, subregion_col);

		    if (!have_mask) {
			P_Regular_Points(&elaboration_reg, &original_reg, general_box,
				     overlap_box, &out_seg, parVect,
				     stepN, stepE, dims.overlap, mean,
				     nsplx, nsply, nrows, ncols, bilin);
		    }
		    else {
			P_Sparse_Raster_Points(&out_seg,
					&elaboration_reg, &original_reg,
					general_box, overlap_box,
					observ_ext, parVect,
					stepE, stepN,
					dims.overlap, nsplx, nsply,
					npoints_ext, bilin, mean);
		    }
		}
		else {		/* OBSERVATION POINTS INTERPOLATION */
		    if (ext == FALSE) {
			G_debug(1, "Interpolation: (%d,%d): Sparse_Points...",
				subregion_row, subregion_col);
			P_Sparse_Points(&Out, &elaboration_reg, general_box,
					overlap_box, obsVect, parVect,
					lineVect, stepE, stepN,
					dims.overlap, nsplx, nsply, npoints,
					bilin, Cats, driver, mean,
					table_name);
		    }
		    else {	/* FLAG_EXT == TRUE */

			/* done that earlier */
			/*
			int npoints_ext, *lineVect_ext = NULL;
			double **obsVect_ext;
			struct Point *observ_ext;

			observ_ext =
			    P_Read_Vector_Region_Map(&In_ext,
						     &elaboration_reg,
						     &npoints_ext, dim_vect,
						     1);
			*/

			obsVect_ext = G_alloc_matrix(npoints_ext, 3);	/* Observation vector_ext */
			lineVect_ext = G_alloc_ivector(npoints_ext);

			for (i = 0; i < npoints_ext; i++) {	/* Setting obsVect_ext vector & Q matrix */
			    obsVect_ext[i][0] = observ_ext[i].coordX;
			    obsVect_ext[i][1] = observ_ext[i].coordY;
			    obsVect_ext[i][2] = observ_ext[i].coordZ - mean;
			    lineVect_ext[i] = observ_ext[i].lineID;
			}

			G_free(observ_ext);

			G_debug(1, "Interpolation: (%d,%d): Sparse_Points...",
				subregion_row, subregion_col);
			P_Sparse_Points(&Out, &elaboration_reg, general_box,
					overlap_box, obsVect_ext, parVect,
					lineVect_ext, stepE, stepN,
					dims.overlap, nsplx, nsply,
					npoints_ext, bilin, Cats, driver,
					mean, table_name);

			G_free_matrix(obsVect_ext);
			G_free_ivector(lineVect_ext);
		    }		/* END FLAG_EXT == TRUE */
		}		/* END GRID == FALSE */
		G_free_vector(parVect);
		G_free_matrix(obsVect);
		G_free_ivector(lineVect);
	    }
	    else {
		if (observ)
		    G_free(observ);
		if (observ_ext)
		    G_free(observ_ext);
		if (npoints == 0)
		    G_warning(_("No data within this subregion. "
				"Consider increasing spline step values."));
	    }
	}			/*! END WHILE; last_column = TRUE */
    }				/*! END WHILE; last_row = TRUE */

    G_verbose_message(_("Writing output..."));
    /* Writing the output raster map */
    if (grid == TRUE) {
	int row, col;
	DCELL *drastbuf, dval;


	if (have_mask) {
	    segment_release(&mask_seg);	/* release memory  */
	    close(mask_fd);
	    unlink(mask_file);
	}

	drastbuf = Rast_allocate_buf(DCELL_TYPE);
	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);
	    for (col = 0; col < ncols; col++) {
		segment_get(&out_seg, &dval, row, col);
		drastbuf[col] = dval;
	    }
	    Rast_put_d_row(raster, drastbuf);
	}

	Rast_close(raster);

	segment_release(&out_seg);	/* release memory  */
	close(out_fd);
	unlink(out_file);
	/* set map title */
	sprintf(title, "%s interpolation with Tykhonov regularization",
		type_opt->answer);
	Rast_put_cell_title(out_map_opt->answer, title);
	/* write map history */
	Rast_short_history(out_map_opt->answer, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(out_map_opt->answer, &history);
    }
    /* Writing to the output vector map the points from the overlapping zones */
    else if (flag_auxiliar == TRUE) {
	if (ext == FALSE)
	    P_Aux_to_Vector(&In, &Out, driver, table_name);
	else
	    P_Aux_to_Vector(&In_ext, &Out, driver, table_name);

	/* Drop auxiliary table */
	G_debug(1, "%s: Dropping <%s>", argv[0], table_name);
	if (P_Drop_Aux_Table(driver, table_name) != DB_OK)
	    G_fatal_error(_("Auxiliary table could not be dropped"));
    }

    db_close_database_shutdown_driver(driver);

    Vect_close(&In);
    if (ext != FALSE)
	Vect_close(&In_ext);
    if (vector)
	Vect_close(&Out);

    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}				/*END MAIN */
