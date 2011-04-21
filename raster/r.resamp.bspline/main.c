
/**********************************************************************
 *
 * MODULE:       r.resamp.bspline
 *
 * AUTHOR(S):    Markus Metz
 *
 * PURPOSE:      Spline Interpolation
 *
 * COPYRIGHT:    (C) 2010 GRASS development team
 *
 *               This program is free software under the
 *               GNU General Public License (>=v2).
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **********************************************************************/

/* INCLUDES */
#include <grass/config.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bspline.h"

/*--------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    /* Variable declarations */
    int nsply, nsplx, row, col, nrows, ncols, nsplx_adj, nsply_adj;
    int nsubregion_col, nsubregion_row, subregion_row, subregion_col;
    int subregion = 0, nsubregions = 0;
    int last_row, last_column, interp_method;	/* booleans */
    double lambda, mean;
    double N_extension, E_extension, edgeE, edgeN;
    double stepN, stepE;

    const char *inrast, *outrast;
    char title[64];

    int dim_vect, nparameters, BW;
    double **inrast_matrix, **outrast_matrix;	/* Matrix to store the auxiliar raster values */
    double *TN, *Q, *parVect;	/* Interpolating and least-square vectors */
    double **N, **obsVect;	/* Interpolation and least-square matrix */
    char **mask_matrix;   /* matrix for masking */

    /* Structs declarations */
    int inrastfd, outrastfd;
    DCELL *drastbuf;
    struct History history;

    struct Map_info Grid;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int cat = 1;

    struct GModule *module;
    struct Option *in_opt, *out_opt, *grid_opt, *stepE_opt, *stepN_opt,
		  *lambda_f_opt, *method_opt, *mask_opt;
    struct Flag *null_flag, *cross_corr_flag;

    struct Reg_dimens dims;
    struct Cell_head elaboration_reg, src_reg, dest_reg;
    struct bound_box general_box, overlap_box, dest_box;
    struct bound_box last_overlap_box, last_general_box;

    struct Point *observ;
    struct Point *observ_null;

    /*----------------------------------------------------------------*/
    /* Options declarations */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("surface"));
    G_add_keyword(_("resample"));
    G_add_keyword(_("interpolation"));
    module->description =
	_("Bicubic or bilinear spline interpolation with Tykhonov regularization.");

    in_opt = G_define_standard_option(G_OPT_R_INPUT);

    out_opt = G_define_standard_option(G_OPT_R_OUTPUT);

    grid_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    grid_opt->key = "grid";
    grid_opt->description = _("Output vector with interpolation grid");
    grid_opt->required = NO;
    
    mask_opt = G_define_standard_option(G_OPT_R_INPUT);
    mask_opt->key = "mask";
    mask_opt->label = _("Raster map to use for masking");
    mask_opt->description = _("Only cells that are not NULL and not zero are interpolated");
    mask_opt->required = NO;

    stepE_opt = G_define_option();
    stepE_opt->key = "se";
    stepE_opt->type = TYPE_DOUBLE;
    stepE_opt->required = NO;
    stepE_opt->description =
	_("Length of each spline step in the east-west direction. Default: 1.5 * ewres.");
    stepE_opt->guisection = _("Settings");

    stepN_opt = G_define_option();
    stepN_opt->key = "sn";
    stepN_opt->type = TYPE_DOUBLE;
    stepN_opt->required = NO;
    stepN_opt->description =
	_("Length of each spline step in the north-south direction. Default: 1.5 * nsres.");
    stepN_opt->guisection = _("Settings");

    method_opt = G_define_option();
    method_opt->key = "method";
    method_opt->type = TYPE_STRING;
    method_opt->required = NO;
    method_opt->description = _("Spline interpolation algorithm");
    method_opt->options = "bilinear,bicubic";
    method_opt->answer = "bicubic";
    method_opt->guisection = _("Settings");

    lambda_f_opt = G_define_option();
    lambda_f_opt->key = "lambda";
    lambda_f_opt->type = TYPE_DOUBLE;
    lambda_f_opt->required = NO;
    lambda_f_opt->description = _("Tykhonov regularization parameter (affects smoothing)");
    lambda_f_opt->answer = "0.005";
    lambda_f_opt->guisection = _("Settings");

    null_flag = G_define_flag();
    null_flag->key = 'n';
    null_flag->label = _("Only interpolate null cells in input raster map");
    null_flag->guisection = _("Settings");

    cross_corr_flag = G_define_flag();
    cross_corr_flag->key = 'c';
    cross_corr_flag->description =
	_("Find the best Tykhonov regularizing parameter using a \"leave-one-out\" cross validation method");

    /*----------------------------------------------------------------*/
    /* Parsing */
    G_gisinit(argv[0]);
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    inrast = in_opt->answer;
    outrast = out_opt->answer;

    if (!strcmp(method_opt->answer, "bilinear"))
	interp_method = P_BILINEAR;
    else
	interp_method = P_BICUBIC;

    lambda = atof(lambda_f_opt->answer);

    /* Setting regions and boxes */
    G_debug(1, "Interpolation: Setting regions and boxes");
    G_get_set_window(&dest_reg);
    G_get_set_window(&elaboration_reg);
    Vect_region_box(&dest_reg, &dest_box);
    Vect_region_box(&elaboration_reg, &overlap_box);
    Vect_region_box(&elaboration_reg, &general_box);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* get window of input map */
    Rast_get_cellhd(in_opt->answer, "", &src_reg);

    if (stepE_opt->answer) {
	stepE = atof(stepE_opt->answer);
	if (stepE <= .0)
	    G_fatal_error(_("se must be positive"));
    }
    else
        stepE = src_reg.ew_res * 1.5;

    if (stepN_opt->answer) {
	stepN = atof(stepN_opt->answer);
	if (stepN <= .0)
	    G_fatal_error(_("sn must be positive"));
    }
    else
        stepN = src_reg.ns_res * 1.5;

    /*------------------------------------------------------------------
      | Subdividing and working with tiles: 									
      | Each original region will be divided into several subregions. 
      | Each one will be overlaped by its neighbouring subregions. 
      | The overlapping is calculated as a fixed OVERLAP_SIZE times
      | the largest spline step plus 2 * orlo
      ----------------------------------------------------------------*/

    /* Fixing parameters of the elaboration region */
    P_zero_dim(&dims);		/* Set dim struct to zero */

    nsplx_adj = NSPLX_MAX;
    nsply_adj = NSPLY_MAX;
    if (stepN > stepE)
	dims.overlap = OVERLAP_SIZE * stepN;
    else
	dims.overlap = OVERLAP_SIZE * stepE;
    P_get_edge(interp_method, &dims, stepE, stepN);
    P_set_dim(&dims, stepE, stepN, &nsplx_adj, &nsply_adj);

    G_verbose_message(_("spline step in ew direction %g"), stepE);
    G_verbose_message(_("spline step in ns direction %g"), stepN);
    G_verbose_message(_("adjusted EW splines %d"), nsplx_adj);
    G_verbose_message(_("adjusted NS splines %d"), nsply_adj);

    /* calculate number of subregions */
    edgeE = dims.ew_size - dims.overlap - 2 * dims.edge_v;
    edgeN = dims.sn_size - dims.overlap - 2 * dims.edge_h;

    N_extension = dest_reg.north - dest_reg.south;
    E_extension = dest_reg.east - dest_reg.west;

    nsubregion_col = ceil(E_extension / edgeE) + 0.5;
    nsubregion_row = ceil(N_extension / edgeN) + 0.5;

    if (nsubregion_col < 0)
	nsubregion_col = 0;
    if (nsubregion_row < 0)
	nsubregion_row = 0;

    nsubregions = nsubregion_row * nsubregion_col;

    G_debug(1, "-------------------------------------");
    G_debug(1, "source north %f", src_reg.north);
    G_debug(1, "source south %f", src_reg.south);
    G_debug(1, "source west %f", src_reg.west);
    G_debug(1, "source east %f", src_reg.east);
    G_debug(1, "-------------------------------------");

    /* adjust source window */
    if (1) {
	double north = dest_reg.north + 2 * dims.edge_h;
	double south = dest_reg.south - 2 * dims.edge_h;
	int r0 = (int)(floor(Rast_northing_to_row(north, &src_reg)) - 0.5);
	int r1 = (int)(floor(Rast_northing_to_row(south, &src_reg)) + 0.5);
	double east = dest_reg.east + 2 * dims.edge_v;
	double west = dest_reg.west - 2 * dims.edge_v;
	/* NOTE: Rast_easting_to_col() is broken because of G_adjust_easting() */
	/*
	int c0 = (int)floor(Rast_easting_to_col(east, &src_reg) + 0.5);
	int c1 = (int)floor(Rast_easting_to_col(west, &src_reg) + 0.5);
        */
	int c0 = (int)(floor(((east - src_reg.west) / src_reg.ew_res)) + 0.5);
	int c1 = (int)(floor(((west - src_reg.west) / src_reg.ew_res)) - 0.5);

	src_reg.north -= src_reg.ns_res * (r0);
	src_reg.south -= src_reg.ns_res * (r1 - src_reg.rows);
	src_reg.east += src_reg.ew_res * (c0 - src_reg.cols);
	src_reg.west += src_reg.ew_res * (c1);
	src_reg.rows = r1 - r0;
	src_reg.cols = c0 - c1;
    }

    /* switch to buffered input raster window */
    /* G_set_window(&src_reg); */
    Rast_set_window(&src_reg);

    G_debug(1, "new source north %f", src_reg.north);
    G_debug(1, "new source south %f", src_reg.south);
    G_debug(1, "new source west %f", src_reg.west);
    G_debug(1, "new source east %f", src_reg.east);
    G_debug(1, "-------------------------------------");

    /* read raster input */
    inrastfd = Rast_open_old(in_opt->answer, "");
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    G_debug(1, "%d new rows, %d new cols", nrows, ncols);

    /* Alloc raster matrix */
    if (!(inrast_matrix = G_alloc_matrix(nrows, ncols)))
	G_fatal_error(_("Cannot allocate memory for auxiliar matrix."
			"Consider changing region (resolution)"));

    drastbuf = Rast_allocate_buf(DCELL_TYPE);

    G_message("loading input raster <%s>", in_opt->answer);
    if (1) {
	int got_one = 0;
	for (row = 0; row < nrows; row++) {
	    DCELL dval;
	    
	    G_percent(row, nrows, 2);

	    Rast_get_d_row(inrastfd, drastbuf, row);

	    for (col = 0; col < ncols; col++) {
		inrast_matrix[row][col] = drastbuf[col];
		dval = inrast_matrix[row][col];
		if (!Rast_is_d_null_value(&dval)) {
		    got_one++;
		}
	    }
	}
	if (!got_one)
	    G_fatal_error("only NULL cells in input raster");
    }
    G_percent(row, nrows, 2);
    G_free(drastbuf);
    Rast_close(inrastfd);

    /* switch back to destination = current window */
    /* G_set_window(&dest_reg); */
    Rast_set_window(&dest_reg);
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    G_debug(1, "-------------------------------------");
    G_debug(1, "dest north %f", dest_reg.north);
    G_debug(1, "dest south %f", dest_reg.south);
    G_debug(1, "dest west %f", dest_reg.west);
    G_debug(1, "dest east %f", dest_reg.east);
    G_debug(1, "-------------------------------------");

    /* cross-correlation */
    if (cross_corr_flag->answer) {
	G_debug(1, "CrossCorrelation()");

	if (cross_correlation(inrast_matrix, &src_reg, stepE, stepN) != TRUE)
	    G_fatal_error(_("Cross validation didn't finish correctly"));
	else {
	    G_debug(1, "Cross validation finished correctly");

	    G_free(inrast_matrix);

	    G_done_msg(_("Cross validation finished for se = %f and sn = %f"), stepE, stepN);
	    exit(EXIT_SUCCESS);
	}
    }

    /* Alloc raster matrix */
    if (!(outrast_matrix = G_alloc_matrix(nrows, ncols)))
	G_fatal_error(_("Cannot allocate memory for auxiliar matrix."
			"Consider changing region (resolution)"));
			
    /* Alloc and load masking matrix */
    if (mask_opt->answer) {
	int maskfd;
	DCELL dval;
	
	G_message(_("Load masking map"));
	
	mask_matrix = (char **)G_calloc(nrows, sizeof(char *));
	mask_matrix[0] = (char *)G_calloc(nrows * ncols, sizeof(char));
	for (row = 1; row < nrows; row++)
	    mask_matrix[row] = mask_matrix[row - 1] + ncols;


	maskfd = Rast_open_old(mask_opt->answer, "");
	drastbuf = Rast_allocate_buf(DCELL_TYPE);

	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);
	    Rast_get_d_row(maskfd, drastbuf, row);
	    for (col = 0; col < ncols; col++) {
		dval = drastbuf[col];
		if (Rast_is_d_null_value(&dval) || dval == 0)
		    mask_matrix[row][col] = 0;
		else
		    mask_matrix[row][col] = 1;
	    }
	}

	G_percent(row, nrows, 2);
	G_free(drastbuf);
	Rast_close(maskfd);
    }
    else
	mask_matrix = NULL;
			

    /* initialize output */
    G_message("initializing output");
    {
	DCELL dval;

	Rast_set_d_null_value(&dval, 1);
	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);
	    for (col = 0; col < ncols; col++) {
		outrast_matrix[row][col] = dval;
	    }
	}
    }
    G_percent(row, nrows, 2);

    if (grid_opt->answer) {
	if (0 > Vect_open_new(&Grid, grid_opt->answer, WITH_Z))
	    G_fatal_error(_("Unable to create vector map <%s>"),
			  grid_opt->answer);
			  
	Points = Vect_new_line_struct();
	Cats = Vect_new_cats_struct();
    }

    subregion_row = 0;
    elaboration_reg.south = dest_reg.north;
    last_row = FALSE;
    overlap_box.S = dest_box.N;
    general_box.S = dest_box.N;

    while (last_row == FALSE) {	/* For each subregion row */
	subregion_row++;
	last_overlap_box.S = overlap_box.S;
	last_general_box.S = general_box.S;
	P_set_regions(&elaboration_reg, &general_box, &overlap_box, dims,
		      GENERAL_ROW);

	/* only works if source reg = dest reg with buffer */
	/* messing with elaboration region is dangerous... */
	/* align_elaboration_box(&elaboration_reg, &src_reg, GENERAL_ROW); */
	align_interp_boxes(&general_box, &overlap_box, &dest_reg,
	                last_general_box, last_overlap_box, GENERAL_ROW);

	if (elaboration_reg.north > dest_reg.north) {	/* First row */

	    P_set_regions(&elaboration_reg, &general_box, &overlap_box, dims,
			  FIRST_ROW);
	    /* align_elaboration_box(&elaboration_reg, &src_reg, GENERAL_ROW); */
	    align_interp_boxes(&general_box, &overlap_box, &dest_reg,
			    last_general_box, last_overlap_box, FIRST_ROW);
	}

	if (elaboration_reg.south <= dest_reg.south) {	/* Last row */

	    P_set_regions(&elaboration_reg, &general_box, &overlap_box, dims,
			  LAST_ROW);
	    last_row = TRUE;
	}

	nsply =
	    ceil((elaboration_reg.north -
		  elaboration_reg.south) / stepN) + 0.5;
	G_debug(1, "Interpolation: nsply = %d", nsply);

	elaboration_reg.east = dest_reg.west;
	last_column = FALSE;
	subregion_col = 0;

	overlap_box.E = dest_box.W;
	general_box.E = dest_box.W;

	while (last_column == FALSE) {	/* For each subregion column */
	    int npoints = 0, npoints_null = 0, n_nulls = 0;

	    subregion_col++;
	    subregion++;
	    if (nsubregions > 1)
		G_message(_("subregion %d of %d"), subregion, nsubregions);

	    last_overlap_box.E = overlap_box.E;
	    last_general_box.E = general_box.E;

	    P_set_regions(&elaboration_reg, &general_box, &overlap_box, dims,
			  GENERAL_COLUMN);

	    /* only works if source reg = dest reg with buffer */
	    /* messing with elaboration region is dangerous... */
	    /* align_elaboration_box(&elaboration_reg, &src_reg, GENERAL_COLUMN); */
	    align_interp_boxes(&general_box, &overlap_box, &dest_reg,
	              last_general_box, last_overlap_box, GENERAL_COLUMN);

	    if (elaboration_reg.west < dest_reg.west) {	/* First column */

		P_set_regions(&elaboration_reg, &general_box, &overlap_box,
			      dims, FIRST_COLUMN);
		/* align_elaboration_box(&elaboration_reg, &src_reg, GENERAL_COLUMN); */
		align_interp_boxes(&general_box, &overlap_box, &dest_reg,
			  last_general_box, last_overlap_box, FIRST_COLUMN);
	    }

	    if (elaboration_reg.east >= dest_reg.east) {	/* Last column */

		P_set_regions(&elaboration_reg, &general_box, &overlap_box,
			      dims, LAST_COLUMN);
		last_column = TRUE;
	    }
	    nsplx =
		ceil((elaboration_reg.east -
		      elaboration_reg.west) / stepE) + 0.5;
	    G_debug(1, "Interpolation: nsplx = %d", nsplx);

	    if (grid_opt->answer) {
		Vect_reset_cats(Cats);
		Vect_cat_set(Cats, 1, cat++);
		Vect_reset_line(Points);
		Vect_append_point(Points, general_box.W, general_box.S, 0);
		Vect_append_point(Points, general_box.E, general_box.S, 0);
		Vect_append_point(Points, general_box.E, general_box.N, 0);
		Vect_append_point(Points, general_box.W, general_box.N, 0);
		Vect_append_point(Points, general_box.W, general_box.S, 0);
		Vect_write_line(&Grid, GV_LINE, Points, Cats);
		Vect_reset_line(Points);
		Vect_append_point(Points, (general_box.E + general_box.W) / 2,
					   (general_box.N + general_box.S) / 2, 0);
		Vect_write_line(&Grid, GV_POINT, Points, Cats);
	    }

	    /* reading points in interpolation region */
	    G_debug(1, "reading points from input raster...");
	    dim_vect = nsplx * nsply;

	    observ =
		P_Read_Raster_Region_Map(inrast_matrix, mask_matrix, &elaboration_reg,
		                         &src_reg, &npoints, &n_nulls,
					 dim_vect);

	    G_debug(1, "%d points, %d NULL cells", npoints, n_nulls);

	    G_debug(1,
		    "Interpolation: (%d,%d): Number of points in <elaboration_box> is %d",
		    subregion_row, subregion_col, npoints);

	    /* Mean calculation for observed non-NULL points */
	    if (npoints > 0)
		mean = P_Mean_Calc(&elaboration_reg, observ, npoints);
	    else
		mean = 0;
	    G_debug(1, "Interpolation: (%d,%d): mean=%lf",
		    subregion_row, subregion_col, mean);

	    observ_null = NULL;
	    if (null_flag->answer && n_nulls) {
		/* read input NULL cells */

		G_debug(1, "read input NULL cells");

		observ_null =
		    P_Read_Raster_Region_Nulls(inrast_matrix, mask_matrix, &src_reg,
					     dest_box, general_box,
					     &npoints_null, dim_vect, mean);

		G_debug(1, "%d nulls in elaboration, %d nulls in general", n_nulls, npoints_null);
		if (npoints_null == 0) {
		    G_free(observ_null);
		    n_nulls = 0;
		}
	    }
	    else if (npoints == 0 && n_nulls == 0)
		/* nothing to interpolate, disable warning below */
		npoints = 1;
	    else
		n_nulls = 1;

	    if (npoints > 0 && n_nulls > 0) {	/*  */
		int i;

		nparameters = nsplx * nsply;
		BW = P_get_BandWidth(interp_method, nsply > nsplx ? nsply : nsplx);

		/* Least Squares system */
		N = G_alloc_matrix(nparameters, BW);	/* Normal matrix */
		TN = G_alloc_vector(nparameters);	/* vector */
		parVect = G_alloc_vector(nparameters);	/* Parameters vector */
		obsVect = G_alloc_matrix(npoints, 3);	/* Observation vector */
		Q = G_alloc_vector(npoints);	/* "a priori" var-cov matrix */

		for (i = 0; i < npoints; i++) {	/* Setting obsVect vector & Q matrix */
		    Q[i] = 1;	/* Q=I */
		    obsVect[i][0] = observ[i].coordX;
		    obsVect[i][1] = observ[i].coordY;
		    obsVect[i][2] = observ[i].coordZ - mean;
		}

		G_free(observ);

		/* Bilinear interpolation */
		if (interp_method == P_BILINEAR) {
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

		G_math_solver_cholesky_sband(N, parVect, TN, nparameters, BW);

		G_free_matrix(N);
		G_free_vector(TN);
		G_free_vector(Q);

		if (!null_flag->answer) {	/* interpolate full output raster */
		    G_debug(1, "Interpolation: (%d,%d): Regular_Points...",
			    subregion_row, subregion_col);
		    outrast_matrix =
			P_Regular_Points(&elaboration_reg, &dest_reg, general_box,
					 overlap_box, outrast_matrix, mask_matrix,
					 parVect, stepN, stepE, dims.overlap, mean,
					 nsplx, nsply, nrows, ncols, interp_method);
		}
		else {		/* only interpolate NULL cells */

		    if (observ_null == NULL)
			G_fatal_error("no NULL cells loaded");

		    G_debug(1, "Interpolation of %d NULL cells...",
			    npoints_null);

		    outrast_matrix =
			P_Sparse_Raster_Points(outrast_matrix,
			                &elaboration_reg, &dest_reg,
					general_box, overlap_box,
					observ_null, parVect,
					stepE, stepN,
					dims.overlap, nsplx, nsply,
					npoints_null, interp_method, mean);

		    G_free(observ_null);
		}		/* end NULL cells */
		G_free_vector(parVect);
		G_free_matrix(obsVect);
	    }
	    else {
		if (observ)
		    G_free(observ);
		if (npoints == 0)
		    G_warning(_("No data within this subregion. "
				"Consider increasing the spline step."));
	    }
	}			/*! END WHILE; last_column = TRUE */
    }				/*! END WHILE; last_row = TRUE */

    G_verbose_message(_("Writing output..."));
    G_free_matrix(inrast_matrix);
    if (mask_opt->answer) {
	G_free(mask_matrix[0]);
	G_free(mask_matrix);
    }
    /* Writing the output raster map */
    Rast_set_fp_type(DCELL_TYPE);
    outrastfd = Rast_open_fp_new(out_opt->answer);

    /* check */
    {
	int nonulls = 0;
	DCELL dval;

	for (row = 0; row < dest_reg.rows; row++) {
	    for (col = 0; col < dest_reg.cols; col++) {
		dval = outrast_matrix[row][col];
		if (!Rast_is_d_null_value(&dval))
		    nonulls++;
	    }
	}
	if (!nonulls)
	    G_warning("only NULL cells in output raster");
    }
    P_Aux_to_Raster(outrast_matrix, outrastfd);
    G_free_matrix(outrast_matrix);

    Rast_close(outrastfd);

    /* set map title */
    sprintf(title, "%s interpolation with Tykhonov regularization",
	    method_opt->answer);
    Rast_put_cell_title(out_opt->answer, title);
    /* write map history */
    Rast_short_history(out_opt->answer, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(out_opt->answer, &history);

    if (grid_opt->answer) {
	Vect_build(&Grid);
	Vect_hist_command(&Grid);
	Vect_close(&Grid);
    }
    
    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}				/*END MAIN */
