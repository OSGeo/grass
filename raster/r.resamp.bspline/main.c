
/**********************************************************************
 *
 * MODULE:       r.resamp.bspline
 *
 * AUTHOR(S):    Markus Metz
 *
 * PURPOSE:      Spline Interpolation
 *
 * COPYRIGHT:    (C) 2010, 2012 by GRASS development team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 *
 **********************************************************************/

/* INCLUDES */
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include "bspline.h"

#define SEGSIZE 	64

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
    double *TN, *Q, *parVect;	/* Interpolating and least-square vectors */
    double **N, **obsVect;	/* Interpolation and least-square matrix */

    SEGMENT in_seg, out_seg, mask_seg;
    const char *in_file, *out_file, *mask_file;
    int in_fd, out_fd, mask_fd;
    double seg_size;
    int seg_mb, segments_in_memory;
    int have_mask;

    int inrastfd, outrastfd;
    DCELL *drastbuf, dval;
    struct History history;

    struct Map_info Grid;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int cat = 1;

    struct GModule *module;
    struct Option *in_opt, *out_opt, *grid_opt, *stepE_opt, *stepN_opt,
		  *lambda_f_opt, *method_opt, *mask_opt, *memory_opt;
    struct Flag *null_flag, *cross_corr_flag;

    struct Reg_dimens dims;
    struct Cell_head elaboration_reg, src_reg, dest_reg;
    struct bound_box general_box, overlap_box, dest_box;
    struct bound_box last_overlap_box, last_general_box;

    struct Point *observ;
    struct Point *observ_marked;

    /*----------------------------------------------------------------*/
    /* Options declarations */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("surface"));
    G_add_keyword(_("resample"));
    G_add_keyword(_("interpolation"));
    module->description =
	_("Performs bicubic or bilinear spline interpolation with Tykhonov regularization.");

    in_opt = G_define_standard_option(G_OPT_R_INPUT);

    out_opt = G_define_standard_option(G_OPT_R_OUTPUT);

    grid_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    grid_opt->key = "grid";
    grid_opt->description = _("Name for output vector map with interpolation grid");
    grid_opt->required = NO;
    
    mask_opt = G_define_standard_option(G_OPT_R_INPUT);
    mask_opt->key = "mask";
    mask_opt->label = _("Name of raster map to use for masking");
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

    method_opt = G_define_standard_option(G_OPT_R_INTERP_TYPE);
    method_opt->description = _("Spline interpolation algorithm");
    method_opt->options = "linear,cubic";
    method_opt->answer = "cubic";
    method_opt->guisection = _("Settings");

    lambda_f_opt = G_define_option();
    lambda_f_opt->key = "lambda";
    lambda_f_opt->type = TYPE_DOUBLE;
    lambda_f_opt->required = NO;
    lambda_f_opt->description = _("Tykhonov regularization parameter (affects smoothing)");
    lambda_f_opt->answer = "0.01";
    lambda_f_opt->guisection = _("Settings");

    null_flag = G_define_flag();
    null_flag->key = 'n';
    null_flag->label = _("Only interpolate null cells in input raster map");
    null_flag->guisection = _("Settings");

    cross_corr_flag = G_define_flag();
    cross_corr_flag->key = 'c';
    cross_corr_flag->description =
	_("Find the best Tykhonov regularizing parameter using a \"leave-one-out\" cross validation method");

    memory_opt = G_define_option();
    memory_opt->key = "memory";
    memory_opt->type = TYPE_INTEGER;
    memory_opt->required = NO;
    memory_opt->answer = "300";
    memory_opt->description = _("Maximum memory to be used (in MB)");

    /*----------------------------------------------------------------*/
    /* Parsing */
    G_gisinit(argv[0]);
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    inrast = in_opt->answer;
    outrast = out_opt->answer;

    if (!strcmp(method_opt->answer, "linear"))
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
    if (interp_method == P_BICUBIC) {
	nsplx_adj = 100;
	nsply_adj = 100;
    }

    dims.overlap = OVERLAP_SIZE * (stepN > stepE ? stepN : stepE);
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
    G_set_window(&src_reg);
    Rast_set_window(&src_reg);

    G_debug(1, "new source north %f", src_reg.north);
    G_debug(1, "new source south %f", src_reg.south);
    G_debug(1, "new source west %f", src_reg.west);
    G_debug(1, "new source east %f", src_reg.east);
    G_debug(1, "-------------------------------------");

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    G_debug(1, "%d new rows, %d new cols", nrows, ncols);

    seg_mb = atoi(memory_opt->answer);
    if (seg_mb < 3)
	G_fatal_error(_("Memory in MB must be >= 3"));

    if (mask_opt->answer || null_flag->answer) {
	seg_size = sizeof(double) * 2 + sizeof(char);
    }
    else {
	seg_size = sizeof(double) * 2;
    }
    seg_size = (seg_size * SEGSIZE * SEGSIZE) / (1 << 20);
    segments_in_memory = seg_mb / seg_size + 0.5;
    if (segments_in_memory < 1)
	segments_in_memory = 1;

    in_file = G_tempfile();
    in_fd = creat(in_file, 0666);
    if (segment_format(in_fd, nrows, ncols, SEGSIZE, SEGSIZE, sizeof(double)) != 1)
	G_fatal_error(_("Can not create temporary file"));
    close(in_fd);

    in_fd = open(in_file, 2);
    if (segment_init(&in_seg, in_fd, segments_in_memory) != 1)
    	G_fatal_error(_("Can not initialize temporary file"));

    /* read raster input */
    inrastfd = Rast_open_old(in_opt->answer, "");
    drastbuf = Rast_allocate_buf(DCELL_TYPE);

    G_message(_("Loading input raster <%s>"), in_opt->answer);
    if (1) {
	int got_one = 0;
	for (row = 0; row < nrows; row++) {
	    DCELL dval;
	    
	    G_percent(row, nrows, 9);

	    Rast_get_d_row_nomask(inrastfd, drastbuf, row);

	    for (col = 0; col < ncols; col++) {
		dval = drastbuf[col];
		if (!Rast_is_d_null_value(&dval)) {
		    got_one++;
		}
	    }
	    segment_put_row(&in_seg, drastbuf, row);
	    
	}
	if (!got_one)
	    G_fatal_error(_("Only NULL cells in input raster"));
    }
    G_percent(row, nrows, 2);
    G_free(drastbuf);
    Rast_close(inrastfd);

    /* switch back to destination = current window */
    G_set_window(&dest_reg);
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

	if (cross_correlation(&in_seg, &src_reg, stepE, stepN) != TRUE)
	    G_fatal_error(_("Cross validation didn't finish correctly"));
	else {
	    G_debug(1, "Cross validation finished correctly");

	    G_done_msg(_("Cross validation finished for se = %f and sn = %f"), stepE, stepN);

	    segment_release(&in_seg);	/* release memory  */
	    close(in_fd);
	    unlink(in_file);

	    exit(EXIT_SUCCESS);
	}
    }

    /* Alloc and load masking matrix */
    /* encoding: 0 = do not interpolate, 1 = interpolate */
    have_mask = mask_opt->answer || null_flag->answer;
    if (have_mask) {
	int maskfd;
	int null_count = 0;
	CELL cval;
	CELL *maskbuf;
	char mask_val;
	
	G_message(_("Mark cells for interpolation"));

	/* use destination window */

	mask_file = G_tempfile();
	mask_fd = creat(mask_file, 0666);
	if (segment_format(mask_fd, nrows, ncols, SEGSIZE, SEGSIZE, sizeof(char)) != 1)
	    G_fatal_error(_("Can not create temporary file"));
	close(mask_fd);

	mask_fd = open(mask_file, 2);
	if (segment_init(&mask_seg, mask_fd, segments_in_memory) != 1)
	    G_fatal_error(_("Can not initialize temporary file"));

	if (mask_opt->answer) {
	    maskfd = Rast_open_old(mask_opt->answer, "");
	    maskbuf = Rast_allocate_buf(CELL_TYPE);
	}
	else {
	    maskfd = -1;
	    maskbuf = NULL;
	}

	if (null_flag->answer) {
	    inrastfd = Rast_open_old(in_opt->answer, "");
	    drastbuf = Rast_allocate_buf(DCELL_TYPE);
	}
	else {
	    inrastfd = -1;
	    drastbuf = NULL;
	}

	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 9);
	    
	    if (mask_opt->answer)
		Rast_get_c_row(maskfd, maskbuf, row);

	    if (null_flag->answer)
		Rast_get_d_row(inrastfd, drastbuf, row);

	    for (col = 0; col < ncols; col++) {
		mask_val = 1;

		if (mask_opt->answer) {
		    cval = maskbuf[col];
		    if (Rast_is_c_null_value(&cval) || cval == 0)
			mask_val = 0;
		}

		if (null_flag->answer && mask_val == 1) {
		    dval = drastbuf[col];
		    if (!Rast_is_d_null_value(&dval))
			mask_val = 0;
		    else
			null_count++;
		}
		segment_put(&mask_seg, &mask_val, row, col);
	    }
	}

	G_percent(row, nrows, 2);
	if (null_flag->answer) {
	    G_free(drastbuf);
	    Rast_close(inrastfd);
	}
	if (mask_opt->answer) {
	    G_free(maskbuf);
	    Rast_close(maskfd);
	}
	if (null_flag->answer && null_count == 0 && !mask_opt->answer) {
	    G_fatal_error(_("No NULL cells found in input raster."));
	}
    }

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
	G_percent(row, nrows, 9);
	segment_put_row(&out_seg, drastbuf, row);
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
	    int npoints = 0;
	    int npoints_marked = 1;   /* default: all points in output */

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
		P_Read_Raster_Region_Map(&in_seg, &elaboration_reg,
		                         &src_reg, &npoints, dim_vect);

	    G_debug(1, "%d valid points", npoints);

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

	    observ_marked = NULL;
	    
	    if (have_mask) {
		/* collect unmasked output cells */

		G_debug(1, "collect unmasked output cells");

		observ_marked =
		    P_Read_Raster_Region_masked(&mask_seg, &dest_reg,
					     dest_box, general_box,
					     &npoints_marked, dim_vect, mean);

		G_debug(1, "%d cells marked in general", npoints_marked);
		if (npoints_marked == 0) {
		    G_free(observ_marked);
		    observ_marked = NULL;
		    npoints = 1; /* disable warning below */
		}
	    }

	    if (npoints > 0 && npoints_marked > 0) {	/*  */
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

		if (!observ_marked) {	/* interpolate full output raster */
		    G_debug(1, "Interpolation: (%d,%d): Regular_Points...",
			    subregion_row, subregion_col);

		    P_Regular_Points(&elaboration_reg, &dest_reg, general_box,
				     overlap_box, &out_seg,
				     parVect, stepN, stepE, dims.overlap, mean,
				     nsplx, nsply, nrows, ncols, interp_method);
		}
		else {		/* only interpolate selected cells */

		    G_debug(1, "Interpolation of %d selected cells...",
			    npoints_marked);

		    P_Sparse_Raster_Points(&out_seg,
				    &elaboration_reg, &dest_reg,
				    general_box, overlap_box,
				    observ_marked, parVect,
				    stepE, stepN,
				    dims.overlap, nsplx, nsply,
				    npoints_marked, interp_method, mean);

		    G_free(observ_marked);
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

    segment_release(&in_seg);	/* release memory  */
    close(in_fd);
    unlink(in_file);
    
    if (have_mask) {
	segment_release(&mask_seg);	/* release memory  */
	close(mask_fd);
	unlink(mask_file);
    }

    G_message(_("Writing output..."));
    /* Writing the output raster map */
    Rast_set_fp_type(DCELL_TYPE);
    outrastfd = Rast_open_fp_new(out_opt->answer);

    /* check */
    {
	int nonulls = 0;

	segment_flush(&out_seg);
	drastbuf = Rast_allocate_d_buf();

	for (row = 0; row < dest_reg.rows; row++) {
	    G_percent(row, dest_reg.rows, 9);
	    segment_get_row(&out_seg, drastbuf, row);
	    for (col = 0; col < dest_reg.cols; col++) {
		dval = drastbuf[col];
		if (!Rast_is_d_null_value(&dval))
		    nonulls++;
	    }
	    Rast_put_d_row(outrastfd, drastbuf);
	}
	G_percent(1, 1, 1);
	if (!nonulls)
	    G_warning("only NULL cells in output raster");
    }

    segment_release(&out_seg);	/* release memory  */
    close(out_fd);
    unlink(out_file);

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
