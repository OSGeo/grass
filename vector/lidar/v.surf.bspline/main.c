/***********************************************************************
 *
 * MODULE:       v.surf.bspline
 *
 * AUTHOR(S):    Roberto Antolin & Gonzalo Moreno
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
 **************************************************************************/

/*INCLUDES*/
#include <stdlib.h> 
#include <string.h> 

#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include <grass/config.h>

#include <grass/PolimiFunct.h>
#include "bspline.h"

/*GLOBAL VARIABLES*/
int bspline_field;
char *bspline_column;

/*-------------------------------------------------------------------------------------------*/
    int
main (int argc,char *argv[])
{

    /* Variables' declarations */
    int  nsply, nsplx, nlines, nrows, ncols, subregion_row, subregion_col;
    int last_row, last_column, grid, bilin, ext, flag_auxiliar, cross; 	/* booleans */
    double passoN, passoE, lambda, mean;		

    char *mapset, *dvr, *db, *vector, *map, table_name[1024], title[64];		

    int dim_vect, nparameters, BW;
    int *lineVect;				/* Vector restoring primitive's ID*/
    double **raster_matrix;			/* Matrix to store the auxiliar raster values*/
    double *TN, *Q, *parVect;			/* Interpolating and least-square vectors*/
    double **N, **obsVect;			/* Interpolation and least-square matrix*/

    /* Structs' declarations */
    int raster;
    struct Map_info In, In_ext, Out;
    struct History history;
    
    struct GModule *module;
    struct Option *in_opt, *in_ext_opt, *out_opt, *out_map_opt, *passoE_opt, 
		  *passoN_opt, *lambda_f_opt, *type, *dfield_opt, *col_opt; 
    struct Flag *cross_corr_flag;

    struct Reg_dimens dims;
    struct Cell_head elaboration_reg, original_reg;
    BOUND_BOX general_box, overlap_box;

    struct Point *observ;
    struct line_pnts *points;
    struct line_cats *Cats;
    dbCatValArray cvarr;

    int nrec, ctype = 0;
    struct field_info *Fi;
    dbDriver *driver, *driver_cats;

    /*-------------------------------------------------------------------------------------------*/
    /* Options' declaration */
    module = G_define_module(); {
	module->keywords = _("vector, interpolation");
	module->description = 
	   _("Bicubic or bilinear spline interpolation with Tykhonov regularization.");
    }

    cross_corr_flag = G_define_flag (); {
	cross_corr_flag->key = 'c';
	cross_corr_flag->description = 
	    _("Find best parameters using a cross validation method");
    }
    
    in_opt = G_define_standard_option(G_OPT_V_INPUT);
    
    in_ext_opt = G_define_standard_option (G_OPT_V_INPUT); {
	in_ext_opt->key 	= "sparse";
	in_ext_opt->required 	= NO;
	in_ext_opt->description = _("Name of input vector map of sparse points");
    }

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
	out_opt->required       = NO;

    out_map_opt = G_define_standard_option(G_OPT_R_OUTPUT);
	out_map_opt->key 	  = "raster";
	out_map_opt->required 	  = NO;

    passoE_opt = G_define_option(); {
	passoE_opt->key		= "sie";
	passoE_opt->type	= TYPE_DOUBLE;
	passoE_opt->required	= NO;
	passoE_opt->answer 	= "4";
	passoE_opt->description	= 
	    _("Interpolation spline step value in east direction");
	passoE_opt->guisection	= _("Settings");
    }
    
    passoN_opt = G_define_option(); {
	passoN_opt->key		= "sin";	
	passoN_opt->type	= TYPE_DOUBLE;
	passoN_opt->required	= NO;
	passoN_opt->answer 	= "4";
	passoN_opt->description	= 
	    _("Interpolation spline step value in north direction");		
	passoN_opt->guisection	= _("Settings");
    }

    type = G_define_option(); {
	type->key	  = "type";	
	type->type	  = TYPE_STRING;	
	type->required	  = NO;	
	type->description = _("Spline type of interpolation");	
	type->options	  = "bilinear,bicubic";
	type->answer	  = "bilinear";
	type->guisection  = _("Settings");
    }

    lambda_f_opt = G_define_option(); {
	lambda_f_opt->key    	  = "lambda_i";
	lambda_f_opt->type	  = TYPE_DOUBLE;
	lambda_f_opt->required	  = NO;
	lambda_f_opt->description =_("Thychonov regularization weigth");
	lambda_f_opt->answer	  = "1";
	lambda_f_opt->guisection  = _("Settings");	
    }

    dfield_opt = G_define_standard_option(G_OPT_V_FIELD); {
	dfield_opt->description =
	    _("Field value. If set to 0, z coordinates are used. (3D vector only)");
	dfield_opt->answer = "0";
	dfield_opt->guisection = _("Settings");
    }

    col_opt = G_define_option() ; {
	col_opt->key        = "column" ;
	col_opt->type       = TYPE_STRING ;
	col_opt->required   = NO ;
	col_opt->description= _("Attribute table column with values to interpolate (if layer>0)");
	col_opt->guisection = _("Settings") ;
    }

/*-------------------------------------------------------------------------------------------*/
    /* Parsing */	
    G_gisinit(argv[0]);
    if (G_parser (argc, argv))
	exit(EXIT_FAILURE); 

    if (!strcmp(type->answer,"bilinear"))
	bilin = P_BILINEAR;
    else
	bilin = P_BICUBIC;

    passoN = atof (passoN_opt->answer);
    passoE = atof (passoE_opt->answer);
    lambda = atof (lambda_f_opt->answer);
    bspline_field = atoi (dfield_opt->answer);
    bspline_column = col_opt->answer;

    flag_auxiliar = FALSE;
    if (cross_corr_flag->answer) {
    }
    vector = out_opt->answer;
    map = out_map_opt->answer;

    if ( !(db=G__getenv2("DB_DATABASE",G_VAR_MAPSET)) )
	G_fatal_error (_("Unable to read name of database"));

    if ( !(dvr = G__getenv2("DB_DRIVER",G_VAR_MAPSET)) )
	G_fatal_error (_("Unable to read name of driver"));

    /* Setting auxiliar table's name */
    if (vector)
	sprintf (table_name, "%s_aux", out_opt->answer);
 
    /* Open driver and database */
    if (db_table_exists (dvr, db, &table_name)){  /*Something went wrong in a 
						    previous v.surf.bspline execution*/
	dbString sql;
	char buf[1024];


	driver = db_start_driver_open_database (dvr, db);
	if (driver == NULL)
	    G_fatal_error( _("No database connection for driver <%s> is defined. " 
			"Run db.connect."), dvr);

	db_init_string (&sql);
	db_zero_string (&sql);	
	sprintf (buf, "drop table %s", table_name);
	db_append_string (&sql, buf);
	if (db_execute_immediate (driver, &sql) != DB_OK) 
	    G_fatal_error (_("It was not possible to drop <%s> table. "
			"Nothing will be done. Try to drop it manually."), table_name);

	db_close_database_shutdown_driver (driver);
    }

    if (vector && map) 
	G_fatal_error (_("Choose either vector or raster output, not both"));
#ifdef nodef
    if (!vector && !map && !cross_corr_flag->answer)
	G_fatal_error (_("No raster nor vector output"));
#endif
    /* Open input vector */
    if ((mapset = G_find_vector2 (in_opt->answer, "")) == NULL) 
	G_fatal_error ( _("Vector map <%s> not found"), in_opt->answer);

    Vect_set_open_level (1); 		/* WITHOUT TOPOLOGY */
    if (1 > Vect_open_old (&In, in_opt->answer, mapset)) 
	G_fatal_error (_("Unable to open vector map <%s> at the topological level"),
		       in_opt->answer);

    /* Open input ext vector */
    if (!in_ext_opt->answer){
	ext = FALSE;
	G_warning ( _("No vector map to interpolate. "
		    "Interpolation will be done with <%s> vector map"), in_opt->answer);
    } else {
	ext = TRUE;
	G_warning (_("<%s> vector map will be interpolated"), in_ext_opt->answer);

	if ((mapset = G_find_vector2 (in_ext_opt->answer, "")) == NULL) 
	    G_fatal_error ( _("Vector map <%s> not found"), in_ext_opt->answer);

	Vect_set_open_level (1); 		/* WITHOUT TOPOLOGY */
	if (1 > Vect_open_old (&In_ext, in_ext_opt->answer, mapset)) 
	    G_fatal_error (_("Unable to open vector map <%s> at the topological level"),
			   in_opt->answer);
    }

    /* Open output map */
    /* vector output */
    if (vector && !map) {
	if ( strcmp(dvr, "dbf") == 0)
	    G_fatal_error (_("Sorry, <%s> driver is not allowed for vector output in this module. " \
			"Try with a raster output or other driver."), dvr); 

	Vect_check_input_output_name (in_opt->answer, out_opt->answer, GV_FATAL_EXIT);
	grid = FALSE;

	if (0 > Vect_open_new (&Out, out_opt->answer, WITH_Z)) 
	    G_fatal_error (_("Unable to create vector map <%s>"), out_opt->answer);

	/* Copy vector Head File */
	if (ext == FALSE) {
	    Vect_copy_head_data (&In, &Out);
	    Vect_hist_copy (&In, &Out);
	} else {
	    Vect_copy_head_data (&In_ext, &Out);
	    Vect_hist_copy (&In_ext, &Out);
	} 
	Vect_hist_command (&Out);
    }

    /* raster output */
    raster = -1;
    G_set_fp_type (DCELL_TYPE);
    if (!vector && map) {
	grid = TRUE;
	/*
	if (G_find_cell (out_map_opt->answer, G_mapset()) != NULL) 
	    G_fatal_error (_("Raster <%s> already exist."), out_map_opt->answer);
	    */

	if ((raster = G_open_fp_cell_new (out_map_opt->answer)) < 0) 
	    G_fatal_error (_("Unable to create raster map <%s>"), out_map_opt->answer);
    }

    if (bspline_field > 0) {
        db_CatValArray_init ( &cvarr );
	Fi = Vect_get_field (&In, bspline_field);
	if ( Fi == NULL )
	    G_fatal_error (_("Cannot read field info"));	

	driver_cats = db_start_driver_open_database ( Fi->driver, Fi->database );
	/*G_debug (0, _("driver=%s db=%s"), Fi->driver, Fi->database);*/
	
	if ( driver_cats == NULL )
    	G_fatal_error (_("Unable to open database <%s> by driver <%s>"), Fi->database, Fi->driver);
    
        nrec = db_select_CatValArray ( driver_cats, Fi->table, Fi->key, col_opt->answer, NULL, &cvarr );
        G_debug (3, "nrec = %d", nrec );
    
        ctype = cvarr.ctype;
        if ( ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE )
	    G_fatal_error ( _("Column type not supported") );
    
        if ( nrec < 0 )
	    G_fatal_error (_("Unable to select data from table"));
    
        G_message ( _("[%d] records selected from table"), nrec);

	db_close_database_shutdown_driver (driver_cats);
    }

/*-------------------------------------------------------------------------------------------*/
    /*Cross-correlation begins*/
    if (cross_corr_flag->answer) {		/* CROSS-CORRELATION WILL BE DONE*/
	G_debug (1, "CrossCorrelation()");
	/*cross = cross_correlation (&In, &passoE, &passoN, &lambda);*/
	cross = cross_correlation (&In, passoE, passoN);
	
	if (cross != TRUE)
	    G_fatal_error (_("Cross validation didn't finish correctly"));
	else { 
	    G_debug (1, "Cross validation finished correctly");

	    Vect_close (&In);
	    if (ext != FALSE) Vect_close (&In_ext);
	    if (vector) Vect_close (&Out);

	    if (map) G_close_cell (raster);

	    G_done_msg (_("Cross Validation was success!"));
	    exit (EXIT_SUCCESS);
	}

	/*G_debug (0, _("passoE = %f, passoN=%f, lambda_i=%f"), passoE, passoN, lambda);*/
    }

/*-------------------------------------------------------------------------------------------*/
    /* Interpolation begins */
    G_debug (1, "Interpolation()");

    /* Open driver and database */
    driver = db_start_driver_open_database (dvr, db);
    if (driver == NULL)
	G_fatal_error( _("No database connection for driver <%s> is defined. "
		    "Run db.connect."), dvr);

    /* Setting regions and boxes */    
    G_debug (1, "Interpolation: Setting regions and boxes");
    G_get_window (&original_reg);
    G_get_window (&elaboration_reg);
    Vect_region_box (&elaboration_reg, &overlap_box);
    Vect_region_box (&elaboration_reg, &general_box);

    /* Alloc raster matrix */
    if (raster) {
	double nrows_ncols;
	
	nrows = G_window_rows ();
	ncols = G_window_cols ();
	nrows_ncols = (double) nrows*ncols;
	
	if ((nrows_ncols) > 30000000) /* about 5500x5500 cells */
	    G_fatal_error (_("Interpolation: The region resolution is too high: %d cells. " 
			"Consider to change it."), nrows_ncols);

	/*raster_matrix = G_alloc_fmatrix (nrows, ncols);  Is it neccesary a double precision??*/
	raster_matrix = G_alloc_matrix (nrows, ncols);
    }

    /* Fixxing parameters of the elaboration region */
    P_zero_dim (&dims);					/* Set to zero the dim struct*/
    dims.latoE = NSPLX_MAX * passoE;
    dims.latoN = NSPLY_MAX * passoN;
    dims.overlap = OVERLAP_SIZE * passoE;
    P_get_orlo (bilin, &dims, passoE, passoN);		/* Set the last two dim elements*/

    /* Creating line and categories structs */   
    points = Vect_new_line_struct ();
    Cats = Vect_new_cats_struct ();
    nlines = Vect_get_num_lines (&In);

    /*------------------------------------------------------------------------------------------
      | Subdividing and working with tiles: 									
      | Each original_region will be divided into several subregions. 
      | Each one will be overlaped by its neibourgh subregions. 
      | The overlaping was calculated as a fixed OVERLAP_SIZE times the east-west resolution
      ------------------------------------------------------------------------------------------*/

    elaboration_reg.south = original_reg.north;

    subregion_row = 0;
    last_row = FALSE;
    while (last_row == FALSE){		/* For each row */
	subregion_row++;	
	P_set_regions(&elaboration_reg, &general_box, &overlap_box, dims, GENERAL_ROW);

	if (elaboration_reg.north > original_reg.north) {		/* First row */

	    P_set_regions(&elaboration_reg, &general_box, &overlap_box, dims, FIRST_ROW);
	    nsply = ceil((elaboration_reg.north - elaboration_reg.south)/passoN);
	    G_debug (1, "Interpolation: nsply = %d", nsply);
	    if (nsply > NSPLY_MAX) 
		nsply = NSPLY_MAX;
	}

	if (elaboration_reg.south <= original_reg.south) {		/* Last row */

	    P_set_regions(&elaboration_reg, &general_box, &overlap_box, dims, LAST_ROW);
	    nsply=ceil((elaboration_reg.north - elaboration_reg.south)/passoN);
	    last_row = TRUE;
	    G_debug (1, "Interpolation: nsply = %d", nsply);
	    if (nsply > NSPLY_MAX) 
		nsply = NSPLY_MAX;
	}

	elaboration_reg.east = original_reg.west;
	last_column = FALSE;

	subregion_col = 0;
	while (last_column == FALSE){	/* For each column */
	    int npoints = 0;
	
	    subregion_col++;
	    P_set_regions(&elaboration_reg, &general_box, &overlap_box, dims, GENERAL_COLUMN);

	    if (elaboration_reg.west < original_reg.west)  {		/* First column */

		P_set_regions(&elaboration_reg, &general_box, &overlap_box, dims, FIRST_COLUMN);
		nsplx=ceil((elaboration_reg.east - elaboration_reg.west)/passoE);
		G_debug (1, "Interpolation: nsply = %d", nsply);
		if (nsplx > NSPLX_MAX) 
		    nsplx = NSPLX_MAX;
	    }

	    if (elaboration_reg.east >= original_reg.east) {		/* Last column */

		P_set_regions(&elaboration_reg, &general_box, &overlap_box, dims, LAST_COLUMN);
		last_column = TRUE;
		nsplx=ceil((elaboration_reg.east - elaboration_reg.west)/passoE);
		G_debug (1, "Interpolation: nsply = %d", nsply);
		if (nsplx > NSPLX_MAX) 
		    nsplx = NSPLX_MAX;
	    }
	    G_debug (1, "Interpolation: (%d,%d): subregion bounds",
		     subregion_row, subregion_col);
	    G_debug (1, "Interpolation: \t\tNORTH:%.2f\t",
		     elaboration_reg.north);
	    G_debug (1, "Interpolation: WEST:%.2f\t\tEAST:%.2f",
		     elaboration_reg.west, elaboration_reg.east);
	    G_debug (1, "Interpolation: \t\tSOUTH:%.2f",
		     elaboration_reg.south);

	    /*Setting the active region*/
	    dim_vect = nsplx * nsply;
	    observ = P_Read_Vector_Region_Map (&In, &elaboration_reg, &npoints, dim_vect, bspline_field);
	    G_debug (1, "Interpolation: (%d,%d): Number of points in <elaboration_box> is %d", 
		     subregion_row, subregion_col, npoints);
	
	    if (npoints > 0) {				/*  */
		int i;
		double *obs_mean;
		nparameters = nsplx * nsply;
		BW = P_get_BandWidth (bilin, nsply);

		/*Least Squares system*/
		N = G_alloc_matrix (nparameters, BW);		/* Normal matrix */
		TN = G_alloc_vector (nparameters);		/* vector */
		parVect = G_alloc_vector (nparameters);		/* Parameters vector */
		obsVect = G_alloc_matrix (npoints, 3);	/* Observation vector */
		Q = G_alloc_vector (npoints);		/* "a priori" var-cov matrix */
		lineVect = G_alloc_ivector (npoints);	/*  */
		obs_mean = G_alloc_vector (npoints);

		if (bspline_field <= 0)
		    mean = P_Mean_Calc (&elaboration_reg, observ, npoints);

		for (i=0; i<npoints; i++) {		/* Setting obsVect vector & Q matrix */
		    double dval;
	    
		    Q[i] = 1;					/* Q=I */
		    lineVect[i] = observ[i].lineID;
		    obsVect[i][0] = observ[i].coordX;
		    obsVect[i][1] = observ[i].coordY;
		    
		    if (bspline_field > 0) {
			int cat, ival, ret, type;
			
			cat = observ[i].cat;
			if ( cat < 0 ) continue;

			if ( ctype == DB_C_TYPE_INT ) {
			    ret = db_CatValArray_get_value_int ( &cvarr, cat, &ival );
			    obsVect[i][2] = ival;
			    obs_mean [i] = ival;
			} else {		 /* DB_C_TYPE_DOUBLE */
			    ret = db_CatValArray_get_value_double ( &cvarr, cat, &dval );
			    obsVect[i][2] = dval;
			    obs_mean [i] = dval;
			}
			if ( ret != DB_OK ) {
			    G_warning (_("Interpolation: (%d,%d): No record for point (cat = %d)"), 
				    subregion_row, subregion_col, cat);
			    continue;
			}
		    }

		    else { 
			obsVect[i][2] = observ[i].coordZ; 
			obs_mean [i] = observ[i].coordZ;
		    } /*obsVect[i][2] = observ[i].coordZ - mean; */
		}

		/* Mean calculation for every point*/
		if (bspline_field > 0)
		    mean = calc_mean (obs_mean, npoints);

		G_debug (1 ,"Interpolation: (%d,%d): mean=%lf",
			 subregion_row, subregion_col, mean);
		
		for (i=0; i<npoints; i++)
		    obsVect[i][2] -= mean;

		G_free (observ);

		if (bilin) {		/* Bilinear interpolation */
		    G_debug (1 , "Interpolation: (%d,%d): Bilinear interpolation...",
			     subregion_row, subregion_col);
		    normalDefBilin (N, TN, Q, obsVect, passoE, passoN, nsplx, nsply, elaboration_reg.west, 
			    elaboration_reg.south, npoints, nparameters, BW);
		    nCorrectGrad (N, lambda, nsplx, nsply, passoE, passoN);
		} 
		else{	
		    G_debug (1, "Interpolation: (%d,%d): Bicubic interpolation...",
			     subregion_row, subregion_col);
		    normalDefBicubic(N, TN, Q, obsVect, passoE, passoN, nsplx, nsply, elaboration_reg.west, 
			    elaboration_reg.south, npoints, nparameters, BW);
		    nCorrectGrad(N, lambda, nsplx, nsply, passoE, passoN);
		}

		tcholSolve(N, TN, parVect, nparameters, BW);

		G_free_matrix (N);
		G_free_vector (TN);
		G_free_vector (Q);

		if (grid == FALSE) {		/*OBSERVATION POINTS INTERPOLATION*/
		    /* Auxiliar table creation */
		    if (flag_auxiliar == FALSE) {
			G_debug (1, "Interpolation: Creating auxiliar table for archiving "
				 "overlapping zones");
			if ((flag_auxiliar = P_Create_Aux_Table (driver, table_name)) == FALSE) {
			    P_Drop_Aux_Table (driver, table_name);
			    G_fatal_error (_("Interpolation: Creating table: "
				"It was impossible to create table <%s>."), table_name);
			}
		    }

		    if (ext == FALSE) {
			G_debug (1 , "Interpolation: (%d,%d): Sparse_Points...",
				 subregion_row, subregion_col);
			P_Sparse_Points (&Out, &elaboration_reg, general_box, overlap_box, obsVect, 
				parVect, lineVect, passoE, passoN, dims.overlap, nsplx, nsply, npoints, 
				bilin, Cats, driver, mean, table_name);

			G_free_matrix (obsVect);
			G_free_vector (parVect);
		    }

		    else {		/* FLAG_EXT == TRUE*/
			int npoints_ext, *lineVect_ext=NULL;
			double **obsVect_ext; /*, mean_ext = .0;*/
			struct Point *observ_ext;

			observ_ext = P_Read_Vector_Region_Map (&In_ext, &elaboration_reg, &npoints_ext, dim_vect, 1);

			obsVect_ext = G_alloc_matrix (npoints_ext, 3);	/* Observation vector_ext */
			lineVect_ext = G_alloc_ivector (npoints_ext);

			for (i=0; i<npoints_ext; i++) {		/* Setting obsVect_ext vector & Q matrix */
			    obsVect_ext[i][0] = observ_ext[i].coordX;
			    obsVect_ext[i][1] = observ_ext[i].coordY;
			    obsVect_ext[i][2] = observ_ext[i].coordZ-mean;
			    lineVect_ext[i] = observ_ext[i].lineID;
			}

			G_free (observ_ext);
			
			G_debug (1, "Interpolation: (%d,%d): Sparse_Points...",
				 subregion_row, subregion_col);
			P_Sparse_Points (&Out, &elaboration_reg, general_box, overlap_box, obsVect_ext, 
				parVect, lineVect_ext, passoE, passoN, dims.overlap, nsplx, nsply, npoints_ext, 
				bilin, Cats, driver, mean, table_name);
 
			G_free_matrix (obsVect_ext);
			G_free_vector (parVect);
			G_free_ivector (lineVect_ext);
		    }		/* END FLAG_EXT == TRUE */
		}		/* END IF GRID == FLASE */

		else {			/*GRID INTERPOLATION ==> INTERPOLATION INTO A RASTER*/ 
		    G_free_matrix (obsVect);
		    flag_auxiliar = TRUE;
		    G_debug (1, "Interpolation: (%d,%d): Regular_Points...",
			     subregion_row, subregion_col);
		    raster_matrix = P_Regular_Points (&elaboration_reg, general_box, overlap_box, raster_matrix, 
			    parVect, passoN, passoE, dims.overlap, mean, nsplx, nsply, nrows, ncols, bilin);
		    G_free_vector (parVect);
		}
		G_free_ivector (lineVect);	
	    }
	} /*! END WHILE; last_column = TRUE*/
    } /*! END WHILE; last_row = TRUE*/	

    /* Writing into the output vector map the points from the overlaping zones */
    if (flag_auxiliar == TRUE) {
	if (grid == FALSE) {
	    if (ext == FALSE)
		P_Aux_to_Vector (&In, &Out, driver, table_name);
	    else
		P_Aux_to_Vector (&In_ext, &Out, driver, table_name);

	    /* Dropping auxiliar table */
	    G_debug (1, "%s: Dropping <%s>", argv[0], table_name);
	    if (P_Drop_Aux_Table (driver, table_name) != DB_OK)
		G_fatal_error(_("Auxiliar table could not be dropped"));
	}
	else {
	    P_Aux_to_Raster (raster_matrix, raster);
	    G_free_matrix (raster_matrix);
	}
    }

    db_close_database_shutdown_driver (driver);

    Vect_close (&In);
    if (ext != FALSE) Vect_close (&In_ext);
    if (vector) Vect_close (&Out);

    if (map) {
	G_close_cell(raster);

	/* set map title */
	sprintf(title, "%s interpolation with Tykhonov regularization",
	    type->answer);
	G_put_cell_title(out_map_opt->answer, title);
	/* write map history */
	G_short_history(out_map_opt->answer, "raster", &history);
	G_command_history(&history);
	G_write_history(out_map_opt->answer, &history);
    }

    G_done_msg ("");

    exit (EXIT_SUCCESS);
}	/*END MAIN*/

