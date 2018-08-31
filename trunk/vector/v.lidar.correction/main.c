
/********************************************************************
 *
 * MODULE:       v.lidar.correction
 *
 * AUTHOR(S):    Roberto Antolin & Gonzalo Moreno
 *               general update Markus Metz
 *
 * PURPOSE:      Correction of the v.growing output
 *
 * COPYRIGHT:    (C) 2005 by Politecnico di Milano -
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
#include "correction.h"

/*----------------------------------------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    /* Declarations */
    int dim_vect, nparameters, BW, npoints;
    int nsply, nsplx, nsplx_adj, nsply_adj;
    int nsubregion_col, nsubregion_row;
    int subregion = 0, nsubregions = 0;
    const char *dvr, *db, *mapset;
    char table_name[GNAME_MAX];
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    double lambda, mean, stepN, stepE, HighThresh,
	LowThresh;
    double N_extension, E_extension, edgeE, edgeN;

    int i, nterrain, count_terrain;

    int last_row, last_column, flag_auxiliar = FALSE;

    int *lineVect;
    double *TN, *Q, *parVect;	/* Interpolating and least-square vectors */
    double **N, **obsVect, **obsVect_all;	/* Interpolation and least-square matrix */

    struct Map_info In, Out, Terrain;
    struct Option *in_opt, *out_opt, *out_terrain_opt, *stepE_opt,
	*stepN_opt, *lambda_f_opt, *Thresh_A_opt, *Thresh_B_opt;
    struct Flag *spline_step_flag;
    struct GModule *module;

    struct Cell_head elaboration_reg, original_reg;
    struct Reg_dimens dims;
    struct bound_box general_box, overlap_box;

    struct Point *observ;
    struct lidar_cat *lcat;

    dbDriver *driver;

/*----------------------------------------------------------------------------------------------------------*/

    G_gisinit(argv[0]);

    /* Options' declaration */
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("LIDAR"));
    module->description =
	_("Corrects the v.lidar.growing output. It is the last of the three algorithms for LIDAR filtering.");

    spline_step_flag = G_define_flag();
    spline_step_flag->key = 'e';
    spline_step_flag->label = _("Estimate point density and distance and quit");
    spline_step_flag->description =
	_("Estimate point density and distance in map units for the input vector points within the current region extents and quit");
    spline_step_flag->suppress_required = YES;

    in_opt = G_define_standard_option(G_OPT_V_INPUT);
    in_opt->description =
	_("Input observation vector map name (v.lidar.growing output)");

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    out_opt->description = _("Output classified vector map name");

    out_terrain_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    out_terrain_opt->key = "terrain";
    out_terrain_opt->description =
	_("Name for output only 'terrain' points vector map");

    stepE_opt = G_define_option();
    stepE_opt->key = "ew_step";
    stepE_opt->type = TYPE_DOUBLE;
    stepE_opt->required = NO;
    stepE_opt->label =
	_("Length of each spline step in the east-west direction");
    stepE_opt->description = _("Default: 25 * east-west resolution");
    stepE_opt->guisection = _("Settings");

    stepN_opt = G_define_option();
    stepN_opt->key = "ns_step";
    stepN_opt->type = TYPE_DOUBLE;
    stepN_opt->required = NO;
    stepN_opt->label =
	_("Length of each spline step in the north-south direction");
    stepN_opt->description = _("Default: 25 * north-south resolution");
    stepN_opt->guisection = _("Settings");

    lambda_f_opt = G_define_option();
    lambda_f_opt->key = "lambda_c";
    lambda_f_opt->type = TYPE_DOUBLE;
    lambda_f_opt->required = NO;
    lambda_f_opt->description =
	_("Regularization weight in reclassification evaluation");
    lambda_f_opt->answer = "1";

    Thresh_A_opt = G_define_option();
    Thresh_A_opt->key = "tch";
    Thresh_A_opt->type = TYPE_DOUBLE;
    Thresh_A_opt->required = NO;
    Thresh_A_opt->description =
	_("High threshold for object to terrain reclassification");
    Thresh_A_opt->answer = "2";

    Thresh_B_opt = G_define_option();
    Thresh_B_opt->key = "tcl";
    Thresh_B_opt->type = TYPE_DOUBLE;
    Thresh_B_opt->required = NO;
    Thresh_B_opt->description =
	_("Low threshold for terrain to object reclassification");
    Thresh_B_opt->answer = "1";

    G_option_required(out_opt, out_terrain_opt, spline_step_flag, NULL);
    G_option_requires(spline_step_flag, in_opt, out_opt, out_terrain_opt, NULL);

    /* Parsing */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    G_get_set_window(&original_reg);
    stepN = 25 * original_reg.ns_res;
    if (stepN_opt->answer)
	stepN = atof(stepN_opt->answer);
    stepE = 25 * original_reg.ew_res;
    if (stepE_opt->answer)
	stepE = atof(stepE_opt->answer);
    lambda = atof(lambda_f_opt->answer);
    HighThresh = atof(Thresh_A_opt->answer);
    LowThresh = atof(Thresh_B_opt->answer);

    /* Open input vector */
    if ((mapset = G_find_vector2(in_opt->answer, "")) == NULL)
	G_fatal_error(_("Vector map <%s> not found"), in_opt->answer);

    Vect_set_open_level(1);	/* without topology */
    if (1 > Vect_open_old(&In, in_opt->answer, mapset))
	G_fatal_error(_("Unable to open vector map <%s>"), in_opt->answer);

    /* Input vector must be 3D */
    if (!Vect_is_3d(&In))
	G_fatal_error(_("Input vector map <%s> is not 3D!"), in_opt->answer);

    /* Estimate point density and mean distance for current region */
    if (spline_step_flag->answer) {
	double dens, dist;
	if (P_estimate_splinestep(&In, &dens, &dist) == 0) {
	    fprintf(stdout, _("Estimated point density: %.4g\n"), dens);
	    fprintf(stdout, _("Estimated mean distance between points: %.4g\n"), dist);
	}
	else
	    G_warning(_("No points in current region!"));
	
	Vect_close(&In);
	exit(EXIT_SUCCESS);
    }

    if (!(db = G_getenv_nofatal2("DB_DATABASE", G_VAR_MAPSET)))
	G_fatal_error(_("Unable to read name of database"));

    if (!(dvr = G_getenv_nofatal2("DB_DRIVER", G_VAR_MAPSET)))
	G_fatal_error(_("Unable to read name of driver"));

    /* Setting auxiliary table's name */
    if (G_name_is_fully_qualified(out_opt->answer, xname, xmapset)) {
	sprintf(table_name, "%s_aux", xname);
    }
    else
	sprintf(table_name, "%s_aux", out_opt->answer);

    /* Something went wrong in a previous v.lidar.correction execution */
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

    /* Checking vector names */
    Vect_check_input_output_name(in_opt->answer, out_opt->answer,
				 G_FATAL_EXIT);

    /* Open output vector */
    if (0 > Vect_open_new(&Out, out_opt->answer, WITH_Z)) {
	Vect_close(&In);
	G_fatal_error(_("Unable to create vector map <%s>"), out_opt->answer);
    }

    if (0 > Vect_open_new(&Terrain, out_terrain_opt->answer, WITH_Z)) {
	Vect_close(&In);
	Vect_close(&Out);
	G_fatal_error(_("Unable to create vector map <%s>"), out_opt->answer);
    }

    /* Copy vector Head File */
    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);
    Vect_copy_head_data(&In, &Terrain);
    Vect_hist_copy(&In, &Terrain);
    Vect_hist_command(&Terrain);

    /* Start driver and open db */
    driver = db_start_driver_open_database(dvr, db);
    if (driver == NULL)
	G_fatal_error(_("No database connection for driver <%s> is defined. Run db.connect."),
		      dvr);
    db_set_error_handler_driver(driver);

    /* Create auxiliary table */
    if ((flag_auxiliar =
	 P_Create_Aux2_Table(driver, table_name)) == FALSE) {
	Vect_close(&In);
	Vect_close(&Out);
	Vect_close(&Terrain);
	exit(EXIT_FAILURE);
    }

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
    P_zero_dim(&dims);

    nsplx_adj = NSPLX_MAX;
    nsply_adj = NSPLY_MAX;
    if (stepN > stepE)
	dims.overlap = OVERLAP_SIZE * stepN;
    else
	dims.overlap = OVERLAP_SIZE * stepE;
    P_get_edge(P_BILINEAR, &dims, stepE, stepN);
    P_set_dim(&dims, stepE, stepN, &nsplx_adj, &nsply_adj);

    G_verbose_message(n_("Adjusted EW spline %d",
                         "Adjusted EW splines %d",
                         nsplx_adj), nsplx_adj);
    G_verbose_message(n_("Adjusted NS spline %d",
                         "Adjusted NS splines %d",
                         nsply_adj), nsply_adj);

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
	if (nsply > NSPLY_MAX) {
	    nsply = NSPLY_MAX;
	}
	*/
	G_debug(1, "nsply = %d", nsply);

	elaboration_reg.east = original_reg.west;
	last_column = FALSE;

	while (last_column == FALSE) {	/* For each column */

	    subregion++;
	    if (nsubregions > 1)
		G_message(_("Subregion %d of %d"), subregion, nsubregions);

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
		ceil((elaboration_reg.east - elaboration_reg.west) / stepE) +
		0.5;
	    /*
	    if (nsplx > NSPLX_MAX) {
		nsplx = NSPLX_MAX;
	    }
	    */
	    G_debug(1, "nsplx = %d", nsplx);

	    dim_vect = nsplx * nsply;
	    G_debug(1, "read vector region map");
	    observ =
		P_Read_Vector_Correction(&In, &elaboration_reg, &npoints,
					 &nterrain, dim_vect, &lcat);

	    G_debug(5, "npoints = %d, nterrain = %d", npoints, nterrain);
	    if (npoints > 0) {	/* If there is any point falling into elaboration_reg. */
		count_terrain = 0;
		nparameters = nsplx * nsply;

		/* Mean calculation */
		G_important_message(_("Performing mean calculation..."));
		mean = P_Mean_Calc(&elaboration_reg, observ, npoints);

		/*Least Squares system */
		BW = P_get_BandWidth(P_BILINEAR, nsply);	/* Bilinear interpolation */
		N = G_alloc_matrix(nparameters, BW);	/* Normal matrix */
		TN = G_alloc_vector(nparameters);	/* vector */
		parVect = G_alloc_vector(nparameters);	/* Bilinear parameters vector */
		obsVect = G_alloc_matrix(nterrain + 1, 3);	/* Observation vector with terrain points */
		obsVect_all = G_alloc_matrix(npoints + 1, 3);	/* Observation vector with all points */
		Q = G_alloc_vector(nterrain + 1);	/* "a priori" var-cov matrix */
		lineVect = G_alloc_ivector(npoints + 1);

		/* Setting obsVect vector & Q matrix */
		G_debug(3, "Only TERRAIN points");
		for (i = 0; i < npoints; i++) {
		    if (observ[i].cat == TERRAIN_SINGLE) {
			obsVect[count_terrain][0] = observ[i].coordX;
			obsVect[count_terrain][1] = observ[i].coordY;
			obsVect[count_terrain][2] = observ[i].coordZ - mean;
			Q[count_terrain] = 1;	/* Q=I */
			count_terrain++;
		    }
		    lineVect[i] = observ[i].lineID;
		    obsVect_all[i][0] = observ[i].coordX;
		    obsVect_all[i][1] = observ[i].coordY;
		    obsVect_all[i][2] = observ[i].coordZ - mean;
		}

		G_free(observ);

		G_verbose_message(_("Bilinear interpolation"));
		normalDefBilin(N, TN, Q, obsVect, stepE, stepN, nsplx,
			       nsply, elaboration_reg.west,
			       elaboration_reg.south, nterrain, nparameters,
			       BW);
		nCorrectGrad(N, lambda, nsplx, nsply, stepE, stepN);
		G_math_solver_cholesky_sband(N, parVect, TN, nparameters, BW);

		G_free_matrix(N);
		G_free_vector(TN);
		G_free_vector(Q);
		G_free_matrix(obsVect);

		G_important_message( _("Correction and creation of terrain vector map..."));
		P_Sparse_Correction(&In, &Out, &Terrain, &elaboration_reg,
				    general_box, overlap_box, obsVect_all, lcat,
				    parVect, lineVect, stepN, stepE,
				    dims.overlap, HighThresh, LowThresh,
				    nsplx, nsply, npoints, driver, mean, table_name);

		G_free_vector(parVect);
		G_free_matrix(obsVect_all);
		G_free_ivector(lineVect);
	    }
	    else {
		G_free(observ);
		G_warning(_("No data within this subregion. "
			    "Consider changing the spline step."));
	    }
	    G_free(lcat);
	}			/*! END WHILE; last_column = TRUE */
    }				/*! END WHILE; last_row = TRUE */

    /* Dropping auxiliary table */
    if (npoints > 0) {
	G_debug(1, "Dropping <%s>", table_name);
	if (P_Drop_Aux_Table(driver, table_name) != DB_OK)
	    G_fatal_error(_("Auxiliary table could not be dropped"));
    }

    db_close_database_shutdown_driver(driver);

    Vect_close(&In);
    Vect_close(&Out);
    Vect_close(&Terrain);

    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}				/*! END MAIN */
