
/**********************************************************************
 *
 * MODULE:       v.outlier
 *
 * AUTHOR(S):    Roberto Antolin
 *               
 * PURPOSE:      Removal of data outliers
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
#include "outlier.h"

/* GLOBAL VARIABLES */
int nsply, nsplx;
double stepN, stepE, Thres_Outlier;

/*--------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    /* Variables declarations */
    int nsplx_adj, nsply_adj;
    int nsubregion_col, nsubregion_row;
    int subregion = 0, nsubregions = 0;
    double N_extension, E_extension, edgeE, edgeN;
    int dim_vect, nparameters, BW, npoints;
    double mean, lambda;
    const char *dvr, *db, *mapset;
    char table_name[GNAME_MAX];
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];

    int last_row, last_column, flag_auxiliar = FALSE;
    int filter_mode;

    int *lineVect;
    double *TN, *Q, *parVect;	/* Interpolating and least-square vectors */
    double **N, **obsVect;	/* Interpolation and least-square matrix */

    /* Structs declarations */
    struct Map_info In, Out, Outlier, Qgis;
    struct Option *in_opt, *out_opt, *outlier_opt, *qgis_opt, *stepE_opt,
	*stepN_opt, *lambda_f_opt, *Thres_O_opt, *filter_opt;
    struct Flag *spline_step_flag;
    struct GModule *module;

    struct Reg_dimens dims;
    struct Cell_head elaboration_reg, original_reg;
    struct bound_box general_box, overlap_box;

    struct Point *observ;

    dbDriver *driver;

    /*----------------------------------------------------------------*/
    /* Options declaration */
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("extract"));
    G_add_keyword(_("select"));
    G_add_keyword(_("filter"));
    G_add_keyword(_("LIDAR"));
    module->description = _("Removes outliers from vector point data.");

    spline_step_flag = G_define_flag();
    spline_step_flag->key = 'e';
    spline_step_flag->label = _("Estimate point density and distance");
    spline_step_flag->description =
	_("Estimate point density and distance for the input vector points within the current region extends and quit");
    spline_step_flag->suppress_required = YES;

    in_opt = G_define_standard_option(G_OPT_V_INPUT);

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);

    outlier_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    outlier_opt->key = "outlier";
    outlier_opt->description = _("Name for output outlier vector map");

    qgis_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    qgis_opt->key = "qgis";
    qgis_opt->required = NO;
    qgis_opt->description = _("Name for vector map for visualization in QGIS");

    stepE_opt = G_define_option();
    stepE_opt->key = "ew_step";
    stepE_opt->type = TYPE_DOUBLE;
    stepE_opt->required = NO;
    stepE_opt->label =
	_("Length of each spline step in the east-west direction");
    stepE_opt->description = _("Default: 10 * east-west resolution");
    stepE_opt->guisection = _("Settings");

    stepN_opt = G_define_option();
    stepN_opt->key = "ns_step";
    stepN_opt->type = TYPE_DOUBLE;
    stepN_opt->required = NO;
    stepN_opt->label =
	_("Length of each spline step in the north-south direction");
    stepN_opt->description = _("Default: 10 * north-south resolution");
    stepN_opt->guisection = _("Settings");

    lambda_f_opt = G_define_option();
    lambda_f_opt->key = "lambda";
    lambda_f_opt->type = TYPE_DOUBLE;
    lambda_f_opt->required = NO;
    lambda_f_opt->description = _("Tykhonov regularization weight");
    lambda_f_opt->answer = "0.1";
    lambda_f_opt->guisection = _("Settings");

    Thres_O_opt = G_define_option();
    Thres_O_opt->key = "threshold";
    Thres_O_opt->type = TYPE_DOUBLE;
    Thres_O_opt->required = NO;
    Thres_O_opt->description = _("Threshold for the outliers");
    Thres_O_opt->answer = "50";

    filter_opt = G_define_option();
    filter_opt->key = "filter";
    filter_opt->type = TYPE_STRING;
    filter_opt->required = NO;
    filter_opt->description = _("Filtering option");
    filter_opt->options = "both,positive,negative";
    filter_opt->answer = "both";

    G_gisinit(argv[0]);
    G_option_requires(spline_step_flag, in_opt, NULL);

    /* Parsing */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (!(db = G_getenv_nofatal2("DB_DATABASE", G_VAR_MAPSET)))
	G_fatal_error(_("Unable to read name of database"));

    if (!(dvr = G_getenv_nofatal2("DB_DRIVER", G_VAR_MAPSET)))
	G_fatal_error(_("Unable to read name of driver"));

    G_get_set_window(&original_reg);
    stepN = 10 * original_reg.ns_res;
    if (stepN_opt->answer)
	stepN = atof(stepN_opt->answer);
    stepE = 10 * original_reg.ew_res;
    if (stepE_opt->answer)
	stepE = atof(stepE_opt->answer);
    lambda = atof(lambda_f_opt->answer);
    Thres_Outlier = atof(Thres_O_opt->answer);

    filter_mode = 0;
    if (strcmp(filter_opt->answer, "positive") == 0)
	filter_mode = 1;
    else if (strcmp(filter_opt->answer, "negative") == 0)
	filter_mode = -1;
    P_set_outlier_fn(filter_mode);

    flag_auxiliar = FALSE;

    /* Checking vector names */
    if (out_opt->answer) 
        Vect_check_input_output_name(in_opt->answer, out_opt->answer,
                                     G_FATAL_EXIT);

    if ((mapset = G_find_vector2(in_opt->answer, "")) == NULL) {
	G_fatal_error(_("Vector map <%s> not found"), in_opt->answer);
    }

    /* Setting auxiliary table's name */
    if (out_opt->answer) {
        if (G_name_is_fully_qualified(out_opt->answer, xname, xmapset)) {
            sprintf(table_name, "%s_aux", xname);
        }
        else
            sprintf(table_name, "%s_aux", out_opt->answer);
    }

    /* Something went wrong in a previous v.outlier execution */
    if (db_table_exists(dvr, db, table_name)) {
	/* Start driver and open db */
	driver = db_start_driver_open_database(dvr, db);
	if (driver == NULL)
	    G_fatal_error(_("No database connection for driver <%s> is defined. Run db.connect."),
			  dvr);
        db_set_error_handler_driver(driver);

	if (P_Drop_Aux_Table(driver, table_name) != DB_OK)
	    G_fatal_error(_("Old auxiliary table could not be dropped"));
	db_close_database_shutdown_driver(driver);
    }

    /* Open input vector */
    Vect_set_open_level(1);	/* WITHOUT TOPOLOGY */
    if (1 > Vect_open_old(&In, in_opt->answer, mapset))
	G_fatal_error(_("Unable to open vector map <%s> at the topological level"),
		      in_opt->answer);

    /* Input vector must be 3D */
    if (!Vect_is_3d(&In))
	G_fatal_error(_("Input vector map <%s> is not 3D!"), in_opt->answer);

    /* Estimate point density and mean distance for current region */
    if (spline_step_flag->answer) {
	double dens, dist;
	if (P_estimate_splinestep(&In, &dens, &dist) == 0) {
	    G_message("Estimated point density: %.4g", dens);
	    G_message("Estimated mean distance between points: %.4g", dist);
	}
	else
	    G_warning(_("No points in current region!"));
	
	Vect_close(&In);
	exit(EXIT_SUCCESS);
    }

    /* Open output vector */
    if (qgis_opt->answer)
	if (0 > Vect_open_new(&Qgis, qgis_opt->answer, WITHOUT_Z))
	    G_fatal_error(_("Unable to create vector map <%s>"),
			  qgis_opt->answer);

    if (0 > Vect_open_new(&Out, out_opt->answer, WITH_Z)) {
	Vect_close(&Qgis);
	G_fatal_error(_("Unable to create vector map <%s>"), out_opt->answer);
    }

    if (0 > Vect_open_new(&Outlier, outlier_opt->answer, WITH_Z)) {
	Vect_close(&Out);
	Vect_close(&Qgis);
	G_fatal_error(_("Unable to create vector map <%s>"), out_opt->answer);
    }

    /* Copy vector Head File */
    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    Vect_copy_head_data(&In, &Outlier);
    Vect_hist_copy(&In, &Outlier);
    Vect_hist_command(&Outlier);

    if (qgis_opt->answer) {
	Vect_copy_head_data(&In, &Qgis);
	Vect_hist_copy(&In, &Qgis);
	Vect_hist_command(&Qgis);
    }

    /* Open driver and database */
    driver = db_start_driver_open_database(dvr, db);
    if (driver == NULL)
	G_fatal_error(_("No database connection for driver <%s> is defined. Run db.connect."),
		      dvr);
    db_set_error_handler_driver(driver);

    /* Create auxiliary table */
    if ((flag_auxiliar =
	 P_Create_Aux2_Table(driver, table_name)) == FALSE)
	G_fatal_error(_("It was impossible to create <%s> table."), table_name);

    db_create_index2(driver, table_name, "ID");
    /* sqlite likes that ??? */
    db_close_database_shutdown_driver(driver);
    driver = db_start_driver_open_database(dvr, db);

    /* Setting regions and boxes */
    G_get_set_window(&elaboration_reg);
    Vect_region_box(&elaboration_reg, &overlap_box);
    Vect_region_box(&elaboration_reg, &general_box);

    /*------------------------------------------------------------------
      | Subdividing and working with tiles: 									
      | Each original region will be divided into several subregions. 
      | Each one will be overlapped by its neighbouring subregions. 
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
    P_get_edge(P_BILINEAR, &dims, stepE, stepN);
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

    elaboration_reg.south = original_reg.north;
    last_row = FALSE;

    while (last_row == FALSE) {	/* For each row */

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
	/*
	if (nsply > NSPLY_MAX)
	    nsply = NSPLY_MAX;
	*/
	G_debug(1, "nsply = %d", nsply);

	elaboration_reg.east = original_reg.west;
	last_column = FALSE;

	while (last_column == FALSE) {	/* For each column */

	    subregion++;
	    if (nsubregions > 1)
		G_message(_("Processing subregion %d of %d..."), subregion, nsubregions);

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
	    /*
	    if (nsplx > NSPLX_MAX)
		nsplx = NSPLX_MAX;
	    */
	    G_debug(1, "nsplx = %d", nsplx);

	    /*Setting the active region */
	    dim_vect = nsplx * nsply;
	    observ =
		P_Read_Vector_Region_Map(&In, &elaboration_reg, &npoints,
					 dim_vect, 1);

	    if (npoints > 0) {	/* If there is any point falling into elaboration_reg... */
		int i;

		nparameters = nsplx * nsply;

		/* Mean calculation */
		mean = P_Mean_Calc(&elaboration_reg, observ, npoints);

		/* Least Squares system */
		G_debug(1, "Allocation memory for bilinear interpolation");
		BW = P_get_BandWidth(P_BILINEAR, nsply);	/* Bilinear interpolation */
		N = G_alloc_matrix(nparameters, BW);	/* Normal matrix */
		TN = G_alloc_vector(nparameters);	/* vector */
		parVect = G_alloc_vector(nparameters);	/* Bicubic parameters vector */
		obsVect = G_alloc_matrix(npoints, 3);	/* Observation vector */
		Q = G_alloc_vector(npoints);	/* "a priori" var-cov matrix */
		lineVect = G_alloc_ivector(npoints);

		/* Setting obsVect vector & Q matrix */
		for (i = 0; i < npoints; i++) {
		    obsVect[i][0] = observ[i].coordX;
		    obsVect[i][1] = observ[i].coordY;
		    obsVect[i][2] = observ[i].coordZ - mean;
		    lineVect[i] = observ[i].lineID;
		    Q[i] = 1;	/* Q=I */
		}

		G_free(observ);

		G_verbose_message(_("Bilinear interpolation"));
		normalDefBilin(N, TN, Q, obsVect, stepE, stepN, nsplx,
			       nsply, elaboration_reg.west,
			       elaboration_reg.south, npoints, nparameters,
			       BW);
		nCorrectGrad(N, lambda, nsplx, nsply, stepE, stepN);
		G_math_solver_cholesky_sband(N, parVect, TN, nparameters, BW);

		G_free_matrix(N);
		G_free_vector(TN);
		G_free_vector(Q);

		G_verbose_message(_("Outlier detection"));
		if (qgis_opt->answer)
		    P_Outlier(&Out, &Outlier, &Qgis, elaboration_reg,
			      general_box, overlap_box, obsVect, parVect,
			      mean, dims.overlap, lineVect, npoints,
			      driver, table_name);
		else
		    P_Outlier(&Out, &Outlier, NULL, elaboration_reg,
			      general_box, overlap_box, obsVect, parVect,
			      mean, dims.overlap, lineVect, npoints,
			      driver, table_name);


		G_free_vector(parVect);
		G_free_matrix(obsVect);
		G_free_ivector(lineVect);

	    }			/*! END IF; npoints > 0 */
	    else {
		G_free(observ);
		G_warning(_("No data within this subregion. "
			    "Consider increasing spline step values."));
	    }
	}			/*! END WHILE; last_column = TRUE */
    }				/*! END WHILE; last_row = TRUE */

    /* Drop auxiliary table */
    if (npoints > 0) {
	G_debug(1, "%s: Dropping <%s>", argv[0], table_name);
	if (P_Drop_Aux_Table(driver, table_name) != DB_OK)
	    G_fatal_error(_("Auxiliary table could not be dropped"));
    }

    db_close_database_shutdown_driver(driver);

    Vect_close(&In);
    Vect_close(&Out);
    Vect_close(&Outlier);
    if (qgis_opt->answer) {
	Vect_build(&Qgis);
	Vect_close(&Qgis);
    }

    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}				/*END MAIN */
