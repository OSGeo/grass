/************************************************************************
 *									*
 * MODULE:       v.outlier						*
 * 									*
 * AUTHOR(S):    Roberto Antolin					*
 *               							*
 * PURPOSE:      Removal of data outliers				*
 *               							*
 * COPYRIGHT:    (C) 2006 by Politecnico di Milano - 			*
 *			     Polo Regionale di Como			*
 *									*
 *               This program is free software under the 		*
 *               GNU General Public License (>=v2). 			*
 *               Read the file COPYING that comes with GRASS		*
 *               for details.						*
 *									*
 ************************************************************************/

/*INCLUDES*/
#include <stdlib.h> 
#include <string.h> 
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include <grass/PolimiFunct.h>
#include "outlier.h"

/* GLOBAL VARIABLES DEFINITIONS */
int nsply, nsplx, first_it;
double passoN, passoE, Thres_Outlier; 

/*--------------------------------------------------------------------------------------*/
    int
main (int argc,char *argv[])
{
    /* Variables' declarations */
    int dim_vect, nparameters, BW, npoints;
    double ew_resol, ns_resol, mean, lambda;
    char *dvr, *db, *mapset, table_name[1024];

    int last_row, last_column, flag_auxiliar = FALSE; 	 

    int *lineVect;
    double *TN, *Q, *parVect;				/* Interpolating and least-square vectors*/
    double **N, **obsVect;				/* Interpolation and least-square matrix*/

    /* Structs' declarations */
    struct Map_info In, Out, Outlier, Qgis;
    struct Option *in_opt, *out_opt, *outlier_opt, *qgis_opt, *passoE_opt, *passoN_opt, \
	*lambda_f_opt, *Thres_O_opt;
    /*struct Flag *qgis_flag;*/ 
    struct GModule *module;

    struct Cell_head elaboration_reg, original_reg;
    struct Reg_dimens dims;
    BOUND_BOX general_box, overlap_box;

    struct Point *observ;

    dbDriver *driver;

    /*------------------------------------------------------------------------------------------*/
    /* Options' declaration */
    module = G_define_module ();
    	module->keywords = _("vector, statistics");
    	module->description = _("Removes outliers from vector point data.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);

    outlier_opt = G_define_option () ;
    	outlier_opt->key = "outlier";
    	outlier_opt->type = TYPE_STRING;
    	outlier_opt->key_desc     = "name";
    	outlier_opt->required     = YES;
    	outlier_opt->gisprompt    = "new,vector,vector";
    	outlier_opt->description  = _("Name of output outlier vector map");

    qgis_opt = G_define_option () ;
    	qgis_opt->key = "qgis";
    	qgis_opt->type = TYPE_STRING;
    	qgis_opt->key_desc     = "name";
    	qgis_opt->required     = NO; 
    	qgis_opt->gisprompt    = "new,vector,vector";
    	qgis_opt->description  = _("Name of vector map for visualization in QGIS");

    passoE_opt = G_define_option ();
    	passoE_opt->key = "soe";
    	passoE_opt->type = TYPE_DOUBLE;
    	passoE_opt->required = NO;
    	passoE_opt->answer = "10";
    	passoE_opt->description = _("Interpolation spline step value in east direction");

    passoN_opt = G_define_option ();
    	passoN_opt->key = "son";
    	passoN_opt->type = TYPE_DOUBLE;
    	passoN_opt->required = NO;
    	passoN_opt->answer = "10";
    	passoN_opt->description = _("Interpolation spline step value in north direction");

    lambda_f_opt = G_define_option();
    	lambda_f_opt->key         = "lambda_i";
    	lambda_f_opt->type        = TYPE_DOUBLE;
    	lambda_f_opt->required    = NO;
    	lambda_f_opt->description = _("Thychonov regularization weigth");
    	lambda_f_opt->answer      = "0.1";

    Thres_O_opt = G_define_option();
    	Thres_O_opt->key         = "thres_o";
    	Thres_O_opt->type        = TYPE_DOUBLE;
    	Thres_O_opt->required    = NO;
    	Thres_O_opt->description = _("Threshold for the outliers");
    	Thres_O_opt->answer      = "50";

    /* Parsing */	
    G_gisinit (argv[0]);

    if (G_parser (argc, argv))
	exit (EXIT_FAILURE); 

   if ( !(db=G__getenv2("DB_DATABASE",G_VAR_MAPSET)) )
    	G_fatal_error (_("Unable to read name of database"));
	
    if ( !(dvr = G__getenv2("DB_DRIVER",G_VAR_MAPSET)) )
	G_fatal_error (_("Unable to read name of driver")); 

    passoN = atof (passoN_opt->answer);
    passoE = atof (passoE_opt->answer);
    lambda = atof (lambda_f_opt->answer);
    Thres_Outlier = atof (Thres_O_opt->answer);

    /* Setting auxiliar table's name */
    sprintf (table_name, "%s_aux", out_opt->answer);

    /* Checking vector names */
    Vect_check_input_output_name ( in_opt->answer, out_opt->answer, GV_FATAL_EXIT );

    if ((mapset = G_find_vector2 (in_opt->answer, "")) == NULL) {
	G_fatal_error ( _("Vector map <%s> not found"), in_opt->answer);
    }

    /* Open output vector */
    if (qgis_opt->answer)  
	if (0 > Vect_open_new (&Qgis, qgis_opt->answer, WITHOUT_Z))
	    G_fatal_error (_("Unable to create vector map <%s>"), qgis_opt->answer);

    if (0 > Vect_open_new (&Out, out_opt->answer, WITH_Z)) {
	Vect_close (&Qgis);
	G_fatal_error (_("Unable to create vector map <%s>"), out_opt->answer);
    }

    if (0 > Vect_open_new (&Outlier, outlier_opt->answer, WITH_Z)) {
	Vect_close (&Out);
	Vect_close (&Qgis);
	G_fatal_error (_("Unable to create vector map <%s>"), out_opt->answer);
    }

    Vect_set_open_level (1);
    /* Open input vector */
    if (1 > Vect_open_old (&In, in_opt->answer, mapset))
	G_fatal_error ( _("Unable to open vector map <%s> at the topological level"), in_opt->answer);

    /* Copy vector Head File */
    Vect_copy_head_data (&In, &Out);
    Vect_hist_copy (&In, &Out);
    Vect_hist_command (&Out);

    Vect_copy_head_data (&In, &Outlier);
    Vect_hist_copy (&In, &Outlier);
    Vect_hist_command (&Outlier);

    if (qgis_opt->answer) {
	Vect_copy_head_data (&In, &Qgis);
	Vect_hist_copy (&In, &Qgis);
	Vect_hist_command (&Qgis);
    }

    /* Start driver and open db*/
    driver = db_start_driver_open_database (dvr, db);
    if (driver == NULL) 
	G_fatal_error (_("No database connection for driver <%s> is defined. Run db.connect."), dvr);

    /* Setting regions and boxes */
    G_get_set_window (&original_reg);
    G_get_set_window (&elaboration_reg);
    Vect_region_box (&elaboration_reg, &overlap_box);
    Vect_region_box (&elaboration_reg, &general_box);

    /* Fixxing parameters of the elaboration region */
    /*! Each original_region will be divided into several subregions. These
     *  subregion will be overlaped by its neibourgh subregions. This overlaping
     *  is calculated as OVERLAP_PASS times the east-west resolution. */

    ew_resol = original_reg.ew_res;
    ns_resol = original_reg.ns_res;

    P_zero_dim (&dims);

    dims.latoE = NSPLX_MAX * passoE;
    dims.latoN = NSPLY_MAX * passoN;
    dims.overlap = OVERLAP_SIZE * ew_resol;
    P_get_orlo (P_BICUBIC, &dims, passoE, passoN);

    elaboration_reg.south = original_reg.north;

    last_row = FALSE;
    first_it = TRUE;

    while (last_row == FALSE){		/* For each row */

	P_set_regions(&elaboration_reg, &general_box, &overlap_box, dims, GENERAL_ROW);

	if (elaboration_reg.north > original_reg.north) {		/* First row */
	    P_set_regions (&elaboration_reg, &general_box, &overlap_box, dims, FIRST_ROW);
	}

	if (elaboration_reg.south <= original_reg.south) {		/* Last row */
	    P_set_regions (&elaboration_reg, &general_box, &overlap_box, dims, LAST_ROW);
	    last_row = TRUE;
	}

	nsply = ceil((elaboration_reg.north - elaboration_reg.south)/passoN)+1;
	if (nsply > NSPLY_MAX) {
	    nsply = NSPLY_MAX;
	}
	G_debug (1, "nsply = %d", nsply);

	elaboration_reg.east = original_reg.west;
	last_column = FALSE;

	while (last_column == FALSE){	/* For each column */

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
	    G_debug (1, "nsplx = %d", nsplx);

	    /*Setting the active region*/
	    dim_vect = nsplx * nsply;
	    observ = P_Read_Vector_Region_Map (&In, &elaboration_reg, &npoints, dim_vect, 1);

	    if (npoints > 0) {				/* If there is any point falling into elaboration_reg... */
		int i;

		nparameters = nsplx * nsply;

		/* Mean's calculation */
		mean = P_Mean_Calc (&elaboration_reg, observ, npoints);

		/* Least Squares system */
		G_debug (1, "Allocation memory for bilinear interpolation");
		BW = P_get_BandWidth (P_BILINEAR, nsply);		/* Bilinear interpolation */
		N = G_alloc_matrix (nparameters, BW);		/* Normal matrix */
		TN = G_alloc_vector (nparameters);		/* vector */
		parVect = G_alloc_vector (nparameters);	/* Bicubic parameters vector */
		obsVect = G_alloc_matrix (npoints, 3);		/* Observation vector */
		Q = G_alloc_vector (npoints);			/* "a priori" var-cov matrix */
		lineVect = G_alloc_ivector (npoints);

		/* Setting obsVect vector & Q matrix */
		for (i=0; i<npoints; i++) {
		    obsVect[i][0] = observ[i].coordX;
		    obsVect[i][1] = observ[i].coordY;
		    obsVect[i][2] = observ[i].coordZ - mean;
		    lineVect[i] = observ[i].lineID;
		    Q[i] = 1;					/* Q=I */
		}

		G_debug (1, "Bilinear interpolation");
		normalDefBilin (N, TN, Q, obsVect, passoE, passoN, nsplx, nsply, elaboration_reg.west, elaboration_reg.south, \
			npoints, nparameters, BW);
		nCorrectGrad (N, lambda, nsplx, nsply, passoE, passoN);
		tcholSolve (N, TN, parVect, nparameters, BW);

		G_free_matrix (N);
		G_free_vector (TN);
		G_free_vector (Q);

		if (flag_auxiliar == FALSE) {
		    G_debug (1, "Creating auxiliar table for archiving overlaping zones");
		    if ((flag_auxiliar = P_Create_Aux_Table (driver, table_name)) == FALSE)
			G_fatal_error (_("It was impossible to create <Auxiliar_outlier_table>."));
		}

		if (qgis_opt->answer)
		    P_Outlier (&Out, &Outlier, &Qgis, elaboration_reg, general_box, overlap_box, obsVect, \
			    parVect, mean, dims.overlap, lineVect, npoints, driver);
		else
		    P_Outlier (&Out, &Outlier, NULL, elaboration_reg, general_box, overlap_box, obsVect, \
			    parVect, mean, dims.overlap, lineVect, npoints, driver);


		G_free_vector (parVect);
		G_free_matrix (obsVect);
		G_free_ivector (lineVect);

	    }	/*! END IF; npoints > 0*/
	    G_free (observ);
	    first_it = FALSE;
	}	/*! END WHILE; last_column = TRUE*/
    }		/*! END WHILE; last_row = TRUE*/

    /* Dropping auxiliar table */
    if (npoints > 0) {
	G_debug (1, "Dropping <%s>", table_name);
	if (P_Drop_Aux_Table (driver, table_name) != DB_OK)
	    G_fatal_error(_("Auxiliar table could not be dropped"));
    }

    db_close_database_shutdown_driver (driver);

    Vect_close (&In);
    Vect_close (&Out);
    Vect_close (&Outlier);
    if (qgis_opt->answer) {
	Vect_build (&Qgis, stderr);
	Vect_close (&Qgis);
    }

    G_done_msg("");

    exit(EXIT_SUCCESS);
}	/*!END MAIN*/
