/**************************************************************
 *									*
 * MODULE:       v.lidar.correction					*
 * 									*
 * AUTHOR(S):    Roberto Antolin & Gonzalo Moreno			*
 *               							*
 * PURPOSE:      Correction of the v.growing output			*
 *               							*
 * COPYRIGHT:    (C) 2005 by Politecnico di Milano - 			*
 *			     Polo Regionale di Como			*
 *									*
 *               This program is free software under the 		*
 *               GNU General Public License (>=v2). 			*
 *               Read the file COPYING that comes with GRASS		*
 *               for details.						*
 *									*
 **************************************************************/

/*INCLUDES*/
#include <stdlib.h> 
#include <string.h> 
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include <grass/PolimiFunct.h>
#include "correction.h"

/*----------------------------------------------------------------------------------------------------------*/
int
main (int argc,char *argv[])
{
/* Declarations */
    int dim_vect, nparameters, BW, npoints, nrows, ncols, nsply, nsplx;
    char *dvr, *db, *mapset, table_name[1024]; 
    double lambda, ew_resol, ns_resol, mean, passoN, passoE, HighThresh, LowThresh;

    int i, nterrain, count_terrain;

    int last_row, last_column, flag_auxiliar = FALSE;

    int *lineVect;
    double *TN, *Q, *parVect;			/* Interpolating and least-square vectors */
    double **N, **obsVect;			/* Interpolation and least-square matrix */

    struct Map_info In, Out, Terrain;
    struct Option *in_opt, *out_opt, *out_terrain_opt, *passoE_opt, *passoN_opt, \
    	*lambda_f_opt, *Thresh_A_opt, *Thresh_B_opt; 
    struct GModule *module;

    struct Cell_head elaboration_reg, original_reg;
    struct Reg_dimens dims;
    BOUND_BOX general_box, overlap_box;

    struct Point *observ;

    dbDriver *driver;

/*----------------------------------------------------------------------------------------------------------*/
/* Options' declaration */
    module = G_define_module();
    module->keywords = _("vector, LIDAR");
    module->description = _("Correction of the v.lidar.growing output. It is the last of the three algorithms for LIDAR filtering.");

    in_opt = G_define_standard_option (G_OPT_V_INPUT);
	in_opt->description = _("Input observation vector map name (v.lidar.growing output)");

    out_opt = G_define_standard_option (G_OPT_V_OUTPUT);
	out_opt->description = _("Output classified vector map name");

    out_terrain_opt = G_define_option () ;
    	out_terrain_opt->key = "terrain";
    	out_terrain_opt->type = TYPE_STRING;
    	out_terrain_opt->key_desc     = "name";
	out_terrain_opt->required     = YES;
	out_terrain_opt->gisprompt    = "new,vector,vector";
	out_terrain_opt->description = _("Only 'terrain' points output vector map");
	
    passoE_opt = G_define_option ();
    	passoE_opt->key = "sce";
    	passoE_opt->type = TYPE_DOUBLE;
    	passoE_opt->required = NO;
    	passoE_opt->answer = "25";
    	passoE_opt->description = _("Interpolation spline step value in east direction");

    passoN_opt = G_define_option ();
    	passoN_opt->key = "scn";
    	passoN_opt->type = TYPE_DOUBLE;
    	passoN_opt->required = NO;
    	passoN_opt->answer = "25";
    	passoN_opt->description = _("Interpolation spline step value in north direction");

    lambda_f_opt = G_define_option();
    	lambda_f_opt->key = "lambda_c";
    	lambda_f_opt->type = TYPE_DOUBLE;
    	lambda_f_opt->required = NO;
    	lambda_f_opt->description = _("Regularization weight in reclassification evaluation");
    	lambda_f_opt->answer = "1";

    Thresh_A_opt = G_define_option();
    	Thresh_A_opt->key = "tch";
    	Thresh_A_opt->type = TYPE_DOUBLE;
    	Thresh_A_opt->required = NO;
    	Thresh_A_opt->description = _("High threshold for object to terrain reclassification");
    	Thresh_A_opt->answer = "2";

    Thresh_B_opt = G_define_option();
    	Thresh_B_opt->key = "tcl";
    	Thresh_B_opt->type = TYPE_DOUBLE;
    	Thresh_B_opt->required = NO;
    	Thresh_B_opt->description = _("Low threshold for terrain to object reclassification");
    	Thresh_B_opt->answer = "1";

/* Parsing */
    G_gisinit(argv[0]);

    if (G_parser (argc, argv))
	exit(EXIT_FAILURE); 

    passoN = atof (passoN_opt->answer);
    passoE = atof (passoE_opt->answer);
    lambda = atof (lambda_f_opt->answer);
    HighThresh = atof (Thresh_A_opt->answer);
    LowThresh = atof (Thresh_B_opt->answer);
    dvr = G__getenv2 ("DB_DRIVER",G_VAR_MAPSET);
    db = G__getenv2 ("DB_DATABASE",G_VAR_MAPSET);

/* Setting auxiliar table's name */
    sprintf (table_name, "%s_aux", out_opt->answer);

/* Checking vector names */
  Vect_check_input_output_name ( in_opt->answer, out_opt->answer, GV_FATAL_EXIT );

/* Open input vector */
    if ((mapset = G_find_vector2 (in_opt->answer, "")) == NULL) 
	G_fatal_error ( _("Vector map <%s> not found"), in_opt->answer);

    Vect_set_open_level (1); /*without topology*/
    if (1 > Vect_open_old (&In, in_opt->answer, mapset)) 
	G_fatal_error (_("Unable to open vector map <%s>"), in_opt->answer);

/* Open output vector */
    if (0 > Vect_open_new (&Out, out_opt->answer, WITH_Z)) {
	Vect_close (&In);
	G_fatal_error (_("Unable to create vector map <%s>"), out_opt->answer);
    }

    if (0 > Vect_open_new (&Terrain, out_terrain_opt->answer, WITH_Z)) {
	Vect_close (&In);
	Vect_close (&Out);
	G_fatal_error (_("Unable to create vector map <%s>"), out_opt->answer);
    }

/* Copy vector Head File */
    Vect_copy_head_data (&In, &Out);
    Vect_hist_copy (&In, &Out);
    Vect_hist_command (&Out);
    Vect_copy_head_data (&In, &Terrain);
    Vect_hist_copy (&In, &Terrain);
    Vect_hist_command (&Terrain);

/* Start driver and open db*/
    driver = db_start_driver_open_database (dvr, db);
    if (driver == NULL)
	G_fatal_error( _("No database connection for driver <%s> is defined. Run db.connect."), dvr);

/* Setting regions and boxes */    
    G_get_set_window (&original_reg);
    G_get_set_window (&elaboration_reg);
    Vect_region_box (&elaboration_reg, &overlap_box);
    Vect_region_box (&elaboration_reg, &general_box);

/* Fixxing parameters of the elaboration region */
/*! Each original_region will be divided into several subregions. These
 *  subregion will be overlaped by its neibourgh subregions. This overlaping
 *  is calculated as OVERLAP_PASS times the east-west resolution. */

    nrows = G_window_rows ();
    ncols = G_window_cols ();

    ew_resol = original_reg.ew_res;
    ns_resol = original_reg.ns_res;

    P_zero_dim (&dims);
    dims.latoE = NSPLX_MAX * passoE;
    dims.latoN = NSPLY_MAX * passoN;
    dims.overlap = OVERLAP_SIZE * ew_resol;
    P_get_orlo (P_BILINEAR, &dims, passoE, passoN);

/* Subdividing and working with tiles */
    elaboration_reg.south = original_reg.north;
    last_row = FALSE;
    while (last_row == FALSE){		/* For each row */

	P_set_regions(&elaboration_reg, &general_box, &overlap_box, dims, GENERAL_ROW);

	if (elaboration_reg.north > original_reg.north) {		/* First row */
	    P_set_regions (&elaboration_reg, &general_box, &overlap_box, dims, FIRST_ROW);
	}

	if (elaboration_reg.south <= original_reg.south) {		/* Last row*/
	    P_set_regions (&elaboration_reg, &general_box, &overlap_box, dims, LAST_ROW);
	    last_row = TRUE;
	}
	
	nsply = ceil((elaboration_reg.north - elaboration_reg.south)/passoN)+1;
	if (nsply > NSPLY_MAX) {
	        nsply = NSPLY_MAX;
	}
	G_debug (1, _("nsply = %d"), nsply);

	elaboration_reg.east = original_reg.west;
	last_column = FALSE;

	while (last_column == FALSE){	/* For each column*/

	    P_set_regions (&elaboration_reg, &general_box, &overlap_box, dims, GENERAL_COLUMN);

	    if (elaboration_reg.west < original_reg.west)  {		/* First column */
		P_set_regions (&elaboration_reg, &general_box, &overlap_box, dims, FIRST_COLUMN);
	    }

	    if (elaboration_reg.east >= original_reg.east) {		/* Last column */
		P_set_regions (&elaboration_reg, &general_box, &overlap_box, dims, LAST_COLUMN);
		last_column = TRUE;
	    }

	    nsplx = ceil ((elaboration_reg.east - elaboration_reg.west)/passoE)+1;
	    if (nsplx > NSPLX_MAX) {
		    nsplx = NSPLX_MAX;
	    }
	    G_debug (1, _("nsplx = %d"), nsplx);

	    dim_vect = nsplx * nsply;
	    G_debug(1, _("read vector region map"));
	    observ = P_Read_Vector_Correction (&In, &elaboration_reg, &npoints, &nterrain, dim_vect);

	    G_debug (5, _("npoints = %d, nterrain = %d"), npoints, nterrain);
	    if (npoints > 0) {				/* If there is any point falling into elaboration_reg. */
		count_terrain = 0;
		nparameters = nsplx * nsply;

	/* Mean's calculation */
		G_debug (3, _("Mean's calculation"));
		mean = P_Mean_Calc (&elaboration_reg, observ, npoints); 
		
	/*Least Squares system*/
		BW = P_get_BandWidth (P_BILINEAR, nsply);		/* Bilinear interpolation */
		N = G_alloc_matrix (nparameters, BW);		/* Normal matrix */
		TN = G_alloc_vector (nparameters);			/* vector */
		parVect = G_alloc_vector (nparameters);		/* Bicubic parameters vector */
		obsVect = G_alloc_matrix (nterrain+1, 3);		/* Observation vector */
		Q = G_alloc_vector (nterrain+1);			/* "a priori" var-cov matrix */
		lineVect = G_alloc_ivector (npoints+1);

	/* Setting obsVect vector & Q matrix */
		G_debug (3, _("Only TERRAIN points"));
		for (i=0; i<npoints; i++) {
		    if (observ[i].cat == TERRAIN_SINGLE) {
		    	obsVect[count_terrain][0] = observ[i].coordX;
		    	obsVect[count_terrain][1] = observ[i].coordY;
		    	obsVect[count_terrain][2] = observ[i].coordZ - mean;
		    	Q[count_terrain] = 1;					/* Q=I */
			count_terrain ++;
		    }
		    lineVect[i] = observ[i].lineID;
		}
		
		G_debug (3, _("M.Q. solution"));
		normalDefBilin (N, TN, Q, obsVect, passoE, passoN, nsplx, nsply, elaboration_reg.west, elaboration_reg.south, \
					nterrain, nparameters, BW);
		nCorrectGrad (N, lambda, nsplx, nsply, passoE, passoN);
		tcholSolve(N, TN, parVect, nparameters, BW);
		
	   	G_free_matrix (N);
	   	G_free_vector (TN);
	   	G_free_vector (Q);
		
		if (flag_auxiliar == FALSE) {
		    if ((flag_auxiliar = P_Create_Aux_Table (driver, table_name)) == FALSE) {
			Vect_close (&In);
			Vect_close (&Out);
			Vect_close (&Terrain);
			exit (EXIT_FAILURE);
		    }
		}

		G_debug (3, _("Correction and creation of terrain vector"));
		P_Sparse_Correction (&In, &Out, &Terrain, &elaboration_reg, general_box, overlap_box, obsVect, parVect, lineVect,\
				passoN, passoE, dims.overlap, HighThresh, LowThresh, nsplx, nsply, npoints, driver, mean);	

		G_free_matrix (obsVect);
		G_free_ivector (lineVect);

		G_free_vector (parVect);
	    }
	    G_free (observ);
	}	/*! END WHILE; last_column = TRUE*/
    }	/*! END WHILE; last_row = TRUE*/

/* Dropping auxiliar table */
    if (npoints > 0) {
       G_debug (1, _("Dropping <%s>"), table_name);
       if (P_Drop_Aux_Table (driver, table_name) != DB_OK)
	  G_fatal_error(_("Auxiliar table could not be dropped"));
    }

    db_close_database_shutdown_driver (driver);

    Vect_close (&In);
    Vect_close (&Out);
    Vect_close (&Terrain);

    G_done_msg (" ");

    exit(EXIT_SUCCESS);
}	/*! END MAIN*/
