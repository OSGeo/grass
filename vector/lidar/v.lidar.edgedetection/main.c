/**************************************************************
 *									
 * MODULE:       v.lidar.edgedetection				
 * 								
 * AUTHOR(S):    Original version in GRASS 5.4 (s.edgedetection):
 * 		 Maria Antonia Brovelli, Massimiliano Cannata, 
 *		 Ulisse Longoni and Mirko Reguzzoni
 *
 *		 Update for GRASS 6.X and improvements:
 * 		 Roberto Antolin and Gonzalo Moreno
 *               							
 * PURPOSE:      Detection of object's edges on a LIDAR data set	
 *               							
 * COPYRIGHT:    (C) 2006 by Politecnico di Milano - 			
 *			     Polo Regionale di Como			
 *									
 *               This program is free software under the 		
 *               GNU General Public License (>=v2). 			
 *               Read the file COPYING that comes with GRASS		
 *               for details.					
 *							
 **************************************************************/

/*INCLUDES*/
#include <stdlib.h> 
#include <string.h> 

#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include <grass/config.h>

#include <grass/PolimiFunct.h>
#include "edgedetection.h"

int nsply, nsplx, line_out_counter, first_it;
double passoN, passoE; 


/**************************************************************************************
**************************************************************************************/
int
main (int argc,char *argv[])
{
/* Variables' declarations */
    int dim_vect, nparameters, BW, npoints;
    double lambda_B, lambda_F, grad_H, grad_L, alpha, ew_resol, ns_resol, mean;
    char *dvr, *db, *mapset, table_interpolation[1024], table_name[1024]; 

    int last_row, last_column, flag_auxiliar = FALSE; 	 

    int *lineVect;
    double *TN, *Q, *parVect_bilin, *parVect_bicub;		/* Interpolating and least-square vectors*/
    double **N, **obsVect;					/* Interpolation and least-square matrix*/

    /* Structs' declarations */
    struct Map_info In, Out;
    struct Option *in_opt, *out_opt, *passoE_opt, *passoN_opt, \
    	*lambdaF_opt, *lambdaB_opt, *gradH_opt, *gradL_opt, *alfa_opt; 
    struct GModule *module;

    struct Cell_head elaboration_reg, original_reg;
    struct Reg_dimens dims;
    BOUND_BOX general_box, overlap_box;

    struct Point *observ;

    dbDriver *driver;

/*------------------------------------------------------------------------------------------*/
/* Options' declaration */
    module = G_define_module ();
    module->keywords = _("vector, LIDAR, edges");
    module->description = _("Detects the object's edges from a LIDAR data set.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);

    passoE_opt = G_define_option ();
    	passoE_opt->key = "see";
    	passoE_opt->type = TYPE_DOUBLE;
    	passoE_opt->required = NO;
    	passoE_opt->answer = "4";
    	passoE_opt->description = _("Interpolation spline step value in east direction");
	passoE_opt->guisection = _("Settings");

    passoN_opt = G_define_option ();
    	passoN_opt->key = "sen";
    	passoN_opt->type = TYPE_DOUBLE;
    	passoN_opt->required = NO;
    	passoN_opt->answer = "4";
    	passoN_opt->description = _("Interpolation spline step value in north direction");
	passoN_opt->guisection = _("Settings");

    lambdaB_opt = G_define_option ();
    	lambdaB_opt->key = "lambda_g";
    	lambdaB_opt->type = TYPE_DOUBLE;
    	lambdaB_opt->required = NO;
    	lambdaB_opt->description = _("Regularization weight in gradient evaluation");
    	lambdaB_opt->answer = "0.01";
	lambdaB_opt->guisection = _("Settings");

    gradH_opt = G_define_option ();
    	gradH_opt->key = "tgh";
    	gradH_opt->type = TYPE_DOUBLE;
    	gradH_opt->required = NO;
    	gradH_opt->description = _("High gradient threshold for edge classification");
    	gradH_opt->answer = "6";
	gradH_opt->guisection = _("Settings");

    gradL_opt = G_define_option ();
    	gradL_opt->key = "tgl";
    	gradL_opt->type = TYPE_DOUBLE;
    	gradL_opt->required = NO;
    	gradL_opt->description = _("Low gradient threshold for edge classification");
    	gradL_opt->answer = "3";
	gradL_opt->guisection = _("Settings");

    alfa_opt = G_define_option ();
    	alfa_opt->key = "theta_g";
    	alfa_opt->type = TYPE_DOUBLE;
    	alfa_opt->required = NO;
    	alfa_opt->description = _("Angle range for same direction detection");
    	alfa_opt->answer = "0.26";
	alfa_opt->guisection = _("Settings");

    lambdaF_opt = G_define_option ();
    	lambdaF_opt->key = "lambda_r";
    	lambdaF_opt->type = TYPE_DOUBLE;
    	lambdaF_opt->required = NO;
    	lambdaF_opt->description = _("Regularization weight in residual evaluation");
    	lambdaF_opt->answer = "2";
	lambdaF_opt->guisection = _("Settings");

/* Parsing */	
    G_gisinit (argv[0]);

    if (G_parser (argc, argv))
	exit(EXIT_FAILURE); 

    line_out_counter = 1;
    passoN = atof (passoN_opt->answer);
    passoE = atof (passoE_opt->answer);
    lambda_F = atof (lambdaF_opt->answer);
    lambda_B = atof (lambdaB_opt->answer);
    grad_H = atof (gradH_opt->answer);
    grad_L = atof (gradL_opt->answer);
    alpha = atof (alfa_opt->answer);
    if ( !(db=G__getenv2 ("DB_DATABASE",G_VAR_MAPSET)) )
	G_fatal_error (_("Unable to read name of database"));

    if ( !(dvr = G__getenv2 ("DB_DRIVER",G_VAR_MAPSET)) )
	G_fatal_error (_("Unable to read name of driver"));

/* Setting auxiliar table's name */
    sprintf (table_name, "%s_aux", out_opt->answer);

/* Checking vector names */
   Vect_check_input_output_name ( in_opt->answer, out_opt->answer, GV_FATAL_EXIT );

    if ((mapset = G_find_vector2 (in_opt->answer, "")) == NULL) {
	 G_fatal_error ( _("Vector map <%s> not found"), in_opt->answer);
    }

/* Open output vector */
    if (0 > Vect_open_new (&Out, out_opt->answer, WITH_Z)) 
	G_fatal_error (_("Unable to create vector map <%s>"), out_opt->answer);

    Vect_set_open_level (1);
/* Open input vector */
    if (1 > Vect_open_old (&In, in_opt->answer, mapset))
    	G_fatal_error ( _("Unable to open vector map <%s>"), in_opt->answer);

/* Copy vector Head File */
    Vect_copy_head_data (&In, &Out);
    Vect_hist_copy (&In, &Out);
    Vect_hist_command (&Out);
    
/* Start driver and open db*/
    driver = db_start_driver_open_database (dvr, db);
    if (driver == NULL) 
	G_fatal_error (_("No database connection for driver <%s> is defined. Run db.connect."), dvr);

    if (Create_Interpolation_Table (out_opt->answer, driver) != DB_OK) 
	G_fatal_error (_("It was impossible to create <%s> interpolation table in database."), out_opt->answer);
    
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
	G_debug (1, _("nsply = %d"), nsply);
	
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
	    G_debug (1, _("nsplx = %d"), nsplx);
	    
	/*Setting the active region*/
	    dim_vect = nsplx * nsply;
	    G_debug (1, _("read vector region map"));
	    observ = P_Read_Vector_Region_Map (&In, &elaboration_reg, &npoints, dim_vect, 1);

	    if (npoints > 0) {				/* If there is any point falling into elaboration_reg... */
	        int i, tn;

		nparameters = nsplx * nsply;

	/* Mean's calculation */
		mean = P_Mean_Calc (&elaboration_reg, observ, npoints);

	/* Least Squares system */
		G_debug (1, _("Allocation memory for bilinear interpolation"));
		BW = P_get_BandWidth (P_BILINEAR, nsply);		/* Bilinear interpolation */
		N = G_alloc_matrix (nparameters, BW);		/* Normal matrix */
		TN = G_alloc_vector (nparameters);		/* vector */
		parVect_bilin = G_alloc_vector (nparameters);	/* Bicubic parameters vector */
		obsVect = G_alloc_matrix (npoints+1, 3);	/* Observation vector */
		Q = G_alloc_vector (npoints+1);			/* "a priori" var-cov matrix */

		lineVect = G_alloc_ivector (npoints+1);

	/* Setting obsVect vector & Q matrix */
		for (i=0; i<npoints; i++) {
		    obsVect[i][0] = observ[i].coordX;
		    obsVect[i][1] = observ[i].coordY;
		    obsVect[i][2] = observ[i].coordZ - mean;
		    lineVect[i] = observ[i].lineID;
		    Q[i] = 1;					/* Q=I */
		}
		
		/*G_free (observ);*/

		G_debug (1, _("Bilinear interpolation"));
		normalDefBilin (N, TN, Q, obsVect, passoE, passoN, nsplx, nsply, elaboration_reg.west, elaboration_reg.south, \
					npoints, nparameters, BW);
		nCorrectGrad (N, lambda_B, nsplx, nsply, passoE, passoN);
		tcholSolve (N, TN, parVect_bilin, nparameters, BW);

		G_free_matrix (N);
		for (tn = 0; tn < nparameters; tn++) TN[tn] = 0;

		G_debug (1, _("Allocation memory for bicubic interpolation"));
		BW = P_get_BandWidth (P_BICUBIC, nsply);
		N = G_alloc_matrix (nparameters, BW);			/* Normal matrix */
		parVect_bicub = G_alloc_vector (nparameters);		/* Bicubic parameters vector */

		G_debug (1, _("Bicubic interpolation"));
		normalDefBicubic (N, TN, Q, obsVect, passoE, passoN, nsplx, nsply, elaboration_reg.west, 
				    elaboration_reg.south, npoints, nparameters, BW);
		nCorrectLapl (N, lambda_F, nsplx, nsply, passoE, passoN);
		tcholSolve (N, TN, parVect_bicub, nparameters, BW);

		G_free_matrix (N);
		G_free_vector (TN);
		G_free_vector (Q);

		if (flag_auxiliar == FALSE) {
		    G_debug (1, _("Creating auxiliar table for archiving overlaping zones"));
		    if ((flag_auxiliar = Create_AuxEdge_Table (driver)) == FALSE) 
			G_fatal_error (_("It was impossible to create <%s>."), table_name);
		}
		G_debug (1, _("Point classification"));
		classification (&Out, elaboration_reg, general_box, overlap_box, obsVect, parVect_bilin, parVect_bicub, \
				mean, alpha, grad_H, grad_L, dims.overlap, lineVect, npoints, driver, out_opt->answer);

		G_free_vector (parVect_bilin);
		G_free_vector (parVect_bicub);
		G_free_matrix (obsVect);
		G_free_ivector (lineVect);
	    }	/* IF */

	    G_free (observ);
	    first_it = FALSE;
	}	/*! END WHILE; last_column = TRUE*/
    }		/*! END WHILE; last_row = TRUE*/

/* Dropping auxiliar table */
    if (npoints > 0) {
       G_debug (1, _("Dropping <%s>"), table_name);
       if (Drop_Aux_Table (driver) != DB_OK)
	  G_warning (_("Auxiliar table could not be dropped"));
    }

    db_close_database_shutdown_driver (driver);

    Vect_close (&In);

    sprintf (table_interpolation, "%s_edge_Interpolation", out_opt->answer);
    Vect_map_add_dblink ( &Out, F_INTERPOLATION, NULL, table_interpolation, "id", db, dvr);

    Vect_close (&Out);

    G_done_msg (" ");

    exit(EXIT_SUCCESS);
}	/*!END MAIN*/
