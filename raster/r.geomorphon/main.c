
/****************************************************************************
*
* MODULE:	r.geomorphon
* AUTHOR(S):	Jarek Jasiewicz jarekj amu.edu.pl with collaboration of Tom Stepinski stepintz uc.edu based on idea of Tomasz Stepinski and Jaroslaw Jasiewicz
*
* PURPOSE:	Calculate terrain forms using machine-vison technique called geomorphons
*		This module allow to calculate standard set of terrain forms
*		using look-up table proposed by authors, calculate patterns (binary and ternary)
*		for every pixel as well as several geomorphometrical parameters
*		This technology is currently capable of "experimental" stage.
*
* COPYRIGHT:	(C) 2002,2012 by the GRASS Development Team
*		(C) Scientific idea of geomotrphon copyrighted to authors.
*
*		This program is free software under the GNU General Public
*		License (>=v2). Read the file COPYING that comes with GRASS
*		for details.
*
 *****************************************************************************/

#define MAIN
#include "local_proto.h"
typedef enum
{ i_dem, o_forms, o_ternary, o_positive, o_negative, o_intensity,
	o_exposition,
    o_range, o_variance, o_elongation, o_azimuth, o_extend, o_width, io_size
} outputs;

int main(int argc, char **argv)
{
    IO rasters[] = {		/* rasters stores output buffers */
	{"elevation", YES, "Name of input elevation raster map", "input", UNKNOWN, -1, NULL},	/* WARNING: this one map is input */
	{"forms", NO, "Most common geomorphic forms", "Patterns", CELL_TYPE,
	 -1, NULL},
	{"ternary", NO, "Code of ternary patterns", "Patterns", CELL_TYPE, -1,
	 NULL},
	{"positive", NO, "Code of binary positive patterns", "Patterns",
	 CELL_TYPE, -1, NULL},
	{"negative", NO, "Code of binary negative patterns", "Patterns",
	 CELL_TYPE, -1, NULL},
	{"intensity", NO,
	 "Rasters containing mean relative elevation of the form", "Geometry",
	 FCELL_TYPE, -1, NULL},
	{"exposition", NO,
	 "Rasters containing maximum difference between extend and central cell",
	 "Geometry", FCELL_TYPE, -1, NULL},
	{"range", NO,
	 "Rasters containing difference between max and min elevation of the form extend",
	 "Geometry", FCELL_TYPE, -1, NULL},
	{"variance", NO, "Rasters containing variance of form boundary",
	 "Geometry", FCELL_TYPE, -1, NULL},
	{"elongation", NO, "Rasters containing local elongation", "Geometry",
	 FCELL_TYPE, -1, NULL},
	{"azimuth", NO, "Rasters containing local azimuth of the elongation",
	 "Geometry", FCELL_TYPE, -1, NULL},
	{"extend", NO, "Rasters containing local extend (area) of the form",
	 "Geometry", FCELL_TYPE, -1, NULL},
	{"width", NO, "Rasters containing local width of the form",
	 "Geometry", FCELL_TYPE, -1, NULL}
    };				/* adding more maps change IOSIZE macro */

    CATCOLORS ccolors[CNT] = {	/* colors and cats for forms */
	{ZERO, 0, 0, 0, "forms"},
	{FL, 220, 220, 220, "flat"},
	{PK, 56, 0, 0, "summit"},
	{RI, 200, 0, 0, "ridge"},
	{SH, 255, 80, 20, "shoulder"},
	{CV, 250, 210, 60, "spur"},
	{SL, 255, 255, 60, "slope"},
	{CN, 180, 230, 20, "hollow"},
	{FS, 60, 250, 150, "footslope"},
	{VL, 0, 0, 255, "valley"},
	{PT, 0, 0, 56, "depression"},
	{__, 255, 0, 255, "ERROR"}
    };

    struct GModule *module;
    struct Option
	*opt_input,
	*opt_output[io_size],
	*par_search_radius,
	*par_skip_radius,
	*par_flat_threshold,
	*par_flat_distance,
	*par_multi_prefix, *par_multi_step, *par_multi_start;
    struct Flag *flag_units, *flag_extended;

    struct History history;

    int i;
    int meters = 0, multires = 0, extended = 0;	/* flags */
    int row, cur_row, col;
    int pattern_size;
    double max_resolution;
    char prefix[20];

    G_gisinit(argv[0]);

    {				/* interface  parameters */
	module = G_define_module();
	module->description =
	    _("Calculates geomorphons (terrain forms) and associated geometry using machine vision approach.");
        G_add_keyword(_("raster"));
	G_add_keyword(_("geomorphons"));
	G_add_keyword(_("terrain patterns"));
	G_add_keyword(_("machine vision geomorphometry"));

	opt_input = G_define_standard_option(G_OPT_R_ELEV);
	opt_input->key = rasters[0].name;
	opt_input->required = rasters[0].required;
	opt_input->description = _(rasters[0].description);

	for (i = 1; i < io_size; ++i) {	/* WARNING: loop starts from one, zero is for input */
	    opt_output[i] = G_define_standard_option(G_OPT_R_OUTPUT);
	    opt_output[i]->key = rasters[i].name;
	    opt_output[i]->required = NO;
	    opt_output[i]->description = _(rasters[i].description);
	    opt_output[i]->guisection = _(rasters[i].gui);
	}

	par_search_radius = G_define_option();
	par_search_radius->key = "search";
	par_search_radius->type = TYPE_INTEGER;
	par_search_radius->answer = "3";
	par_search_radius->required = YES;
	par_search_radius->description = _("Outer search radius");

	par_skip_radius = G_define_option();
	par_skip_radius->key = "skip";
	par_skip_radius->type = TYPE_INTEGER;
	par_skip_radius->answer = "0";
	par_skip_radius->required = YES;
	par_skip_radius->description = _("Inner search radius");

	par_flat_threshold = G_define_option();
	par_flat_threshold->key = "flat";
	par_flat_threshold->type = TYPE_DOUBLE;
	par_flat_threshold->answer = "1";
	par_flat_threshold->required = YES;
	par_flat_threshold->description = _("Flatenss threshold (degrees)");

	par_flat_distance = G_define_option();
	par_flat_distance->key = "dist";
	par_flat_distance->type = TYPE_DOUBLE;
	par_flat_distance->answer = "0";
	par_flat_distance->required = YES;
	par_flat_distance->description =
	    _("Flatenss distance, zero for none");

	par_multi_prefix = G_define_option();
	par_multi_prefix->key = "prefix";
	par_multi_prefix->type = TYPE_STRING;
	par_multi_prefix->description =
	    _("Prefix for maps resulting from multiresolution approach");
	par_multi_prefix->guisection = _("Multires");

	par_multi_step = G_define_option();
	par_multi_step->key = "step";
	par_multi_step->type = TYPE_DOUBLE;
	par_multi_step->answer = "0";
	par_multi_step->description =
	    _("Distance step for every iteration (zero to omit)");
	par_multi_step->guisection = _("Multires");

	par_multi_start = G_define_option();
	par_multi_start->key = "start";
	par_multi_start->type = TYPE_DOUBLE;
	par_multi_start->answer = "0";
	par_multi_start->description =
	    _("Distance where serch will start in multiple mode (zero to omit)");
	par_multi_start->guisection = _("Multires");

	flag_units = G_define_flag();
	flag_units->key = 'm';
	flag_units->description =
	    _("Use meters to define search units (default is cells)");

	flag_extended = G_define_flag();
	flag_extended->key = 'e';
	flag_extended->description = _("Use extended form correction");

	if (G_parser(argc, argv))
	    exit(EXIT_FAILURE);
    }

    {				/* calculate parameters */
	int num_outputs = 0;
	double search_radius, skip_radius, start_radius, step_radius;
	double ns_resolution;

	multires = (par_multi_prefix->answer) ? 1 : 0;
	for (i = 1; i < io_size; ++i)	/* check for outputs */
	    if (opt_output[i]->answer) {
		if (G_legal_filename(opt_output[i]->answer) < 0)
		    G_fatal_error(_("<%s> is an illegal file name"),
				  opt_output[i]->answer);
		num_outputs++;
	    }
	if (!num_outputs && !multires)
	    G_fatal_error(_("At least one output is required, e.g. %s"),
		      opt_output[o_forms]->key);

	meters = (flag_units->answer != 0);
	extended = (flag_extended->answer != 0);
	nrows = Rast_window_rows();
	ncols = Rast_window_cols();
	Rast_get_window(&window);
	G_begin_distance_calculations();

	if (G_projection() == PROJECTION_LL) {	/* for LL max_res should be NS */
	    ns_resolution =
		G_distance(0, Rast_row_to_northing(0, &window), 0,
			   Rast_row_to_northing(1, &window));
	    max_resolution = ns_resolution;
	}
	else {
	    max_resolution = MAX(window.ns_res, window.ew_res);	/* max_resolution MORE meters per cell */
	    ns_resolution = window.ns_res;
	}

	/* search distance */
	search_radius = atof(par_search_radius->answer);
	search_cells =
	    meters ? (int)(search_radius / max_resolution) : search_radius;
	if (search_cells < 1)
	    G_fatal_error(_("Search radius size must cover at least 1 cell"));
	row_radius_size =
	    meters ? ceil(search_radius / ns_resolution) : search_radius;
	row_buffer_size = row_radius_size * 2 + 1;
	search_distance =
	    (meters) ? search_radius : ns_resolution * search_cells;

	/* skip distance */
	skip_radius = atof(par_skip_radius->answer);
	skip_cells =
	    meters ? (int)(skip_radius / max_resolution) : skip_radius;
	if (skip_cells >= search_cells)
	    G_fatal_error(_("Skip radius size must be at least 1 cell lower than radius"));
	skip_distance = (meters) ? skip_radius : ns_resolution * skip_cells;

	/* flatness parameters */
	flat_threshold = atof(par_flat_threshold->answer);
	if (flat_threshold <= 0.)
	    G_fatal_error(_("Flatenss threshold must be grater than 0"));
	flat_threshold = DEGREE2RAD(flat_threshold);

	flat_distance = atof(par_flat_distance->answer);
	flat_distance =
	    (meters) ? flat_distance : ns_resolution * flat_distance;
	flat_threshold_height = tan(flat_threshold) * flat_distance;
	if ((flat_distance > 0 && flat_distance <= skip_distance) ||
	    flat_distance >= search_distance) {
	    G_warning(_("Flatenss distance should be between skip and search radius. Otherwise ignored"));
	    flat_distance = 0;
	}
	if (multires) {
	    start_radius = atof(par_multi_start->answer);
	    start_cells =
		meters ? (int)(start_radius / max_resolution) : start_radius;
	    if (start_cells <= skip_cells)
		start_cells = skip_cells + 1;
	    start_distance =
		(meters) ? start_radius : ns_resolution * start_cells;

	    step_radius = atof(par_multi_step->answer);
	    step_cells =
		meters ? (int)(step_radius / max_resolution) : step_radius;
	    step_distance =
		(meters) ? step_radius : ns_resolution * step_cells;
	    if (step_distance < ns_resolution)
		G_fatal_error(_("For multiresolution mode step must be greater than or equal to resolution of one cell"));

	    if (G_legal_filename(par_multi_prefix->answer) < 0 ||
		strlen(par_multi_prefix->answer) > 19)
		G_fatal_error(_("<%s> is an incorrect prefix"),
			      par_multi_prefix->answer);
	    strcpy(prefix, par_multi_prefix->answer);
	    strcat(prefix, "_");
	    num_of_steps = (int)ceil(search_distance / step_distance);
	}			/* end multires preparation */

	/* print information about distances */
	G_verbose_message("Search distance m: %f, cells: %d", search_distance,
		  search_cells);
	G_verbose_message("Skip distance m: %f, cells: %d", skip_distance,
		  skip_cells);
	G_verbose_message("Flat threshold distance m: %f, height: %f", flat_distance,
		  flat_threshold_height);
	G_verbose_message("%s version", (extended) ? "Extended" : "Basic");
	if (multires) {
	    G_verbose_message
		("Multiresolution mode: search start at: m: %f, cells: %d",
		 start_distance, start_cells);
	    G_verbose_message
		("Multiresolution mode: search step is: m: %f, number of steps %d",
		 step_distance, num_of_steps);
	    G_verbose_message("Prefix for output: %s", prefix);
	}
    }

    /* generate global ternary codes */
    for (i = 0; i < 6561; ++i)
	global_ternary_codes[i] = ternary_rotate(i);

    /* open DEM */
    strcpy(elevation.elevname, opt_input->answer);
    open_map(&elevation);

    if (!multires) {
	PATTERN *pattern;
	PATTERN patterns[4];
	void *pointer_buf;
	double search_dist = search_distance;
	double skip_dist = skip_distance;
	double flat_dist = flat_distance;
	double area_of_octagon =
	    4 * (search_distance * search_distance) * sin(DEGREE2RAD(45.));

	cell_step = 1;
	/* prepare outputs */
	for (i = 1; i < io_size; ++i)
	    if (opt_output[i]->answer) {
		rasters[i].fd =
		    Rast_open_new(opt_output[i]->answer,
				  rasters[i].out_data_type);
		rasters[i].buffer =
		    Rast_allocate_buf(rasters[i].out_data_type);
	    }

	/* main loop */
	for (row = 0; row < nrows; ++row) {
	    G_percent(row, nrows, 2);
	    cur_row = (row < row_radius_size) ? row :
		((row >=
		  nrows - row_radius_size - 1) ? row_buffer_size - (nrows -
								    row -
								    1) :
		 row_radius_size);

	    if (row > (row_radius_size) &&
		row < nrows - (row_radius_size + 1))
		shift_buffers(row);
	    for (col = 0; col < ncols; ++col) {
		/* on borders forms ussualy are innatural. */
		if (row < (skip_cells + 1) || row > nrows - (skip_cells + 2)
		    || col < (skip_cells + 1) ||
		    col > ncols - (skip_cells + 2) ||
		    Rast_is_f_null_value(&elevation.elev[cur_row][col])) {
		    /* set outputs to NULL and do nothing if source value is null   or border */
		    for (i = 1; i < io_size; ++i)
			if (opt_output[i]->answer) {
			    pointer_buf = rasters[i].buffer;
			    switch (rasters[i].out_data_type) {
			    case CELL_TYPE:
				Rast_set_c_null_value(&((CELL *) pointer_buf)
						      [col], 1);
				break;
			    case FCELL_TYPE:
				Rast_set_f_null_value(&((FCELL *) pointer_buf)
						      [col], 1);
				break;
			    case DCELL_TYPE:
				Rast_set_d_null_value(&((DCELL *) pointer_buf)
						      [col], 1);
				break;
			    default:
				G_fatal_error(_("Unknown output data type"));
			    }
			}
		    continue;
		}		/* end null value */
		{
		    int cur_form, small_form;

		    search_distance = search_dist;
		    skip_distance = skip_dist;
		    flat_distance = flat_dist;
		    pattern_size =
			calc_pattern(&patterns[0], row, cur_row, col);
		    pattern = &patterns[0];
		    cur_form =
			determine_form(pattern->num_negatives,
				       pattern->num_positives);

		    /* correction of forms */
		    if (extended && search_distance > 10 * max_resolution) {
			/* 1) remove extensive innatural forms: ridges, peaks, shoulders and footslopes */
			if ((cur_form == 4 || cur_form == 8 || cur_form == 2
			     || cur_form == 3)) {
			    search_distance =
				(search_dist / 2. <
				 4 * max_resolution) ? 4 *
				max_resolution : search_dist / 2.;
			    skip_distance = 0;
			    flat_distance = 0;
			    pattern_size =
				calc_pattern(&patterns[1], row, cur_row, col);
			    pattern = &patterns[1];
			    small_form =
				determine_form(pattern->num_negatives,
					       pattern->num_positives);
			    if (cur_form == 4 || cur_form == 8)
				cur_form = (small_form == 1) ? 1 : cur_form;
			    if (cur_form == 2 || cur_form == 3)
				cur_form = small_form;
			}
			/* 3) Depressions */

		    }		/* end of correction */
		    pattern = &patterns[0];
		    if (opt_output[o_forms]->answer)
			((CELL *) rasters[o_forms].buffer)[col] = cur_form;
		}

		if (opt_output[o_ternary]->answer)
		    ((CELL *) rasters[o_ternary].buffer)[col] =
			determine_ternary(pattern->pattern);
		if (opt_output[o_positive]->answer)
		    ((CELL *) rasters[o_positive].buffer)[col] =
			rotate(pattern->positives);
		if (opt_output[o_negative]->answer)
		    ((CELL *) rasters[o_negative].buffer)[col] =
			rotate(pattern->negatives);
		if (opt_output[o_intensity]->answer)
		    ((FCELL *) rasters[o_intensity].buffer)[col] =
			intensity(pattern->elevation, pattern_size);
		if (opt_output[o_exposition]->answer)
		    ((FCELL *) rasters[o_exposition].buffer)[col] =
			exposition(pattern->elevation);
		if (opt_output[o_range]->answer)
		    ((FCELL *) rasters[o_range].buffer)[col] =
			range(pattern->elevation);
		if (opt_output[o_variance]->answer)
		    ((FCELL *) rasters[o_variance].buffer)[col] =
			variance(pattern->elevation, pattern_size);

		/*                       used only for next four shape functions */
		if (opt_output[o_elongation]->answer ||
		    opt_output[o_azimuth]->answer ||
		    opt_output[o_extend]->answer ||
		    opt_output[o_width]->answer) {
		    float azimuth, elongation, width;

		    radial2cartesian(pattern);
		    shape(pattern, pattern_size, &azimuth, &elongation,
			  &width);
		    if (opt_output[o_azimuth]->answer)
			((FCELL *) rasters[o_azimuth].buffer)[col] = azimuth;
		    if (opt_output[o_elongation]->answer)
			((FCELL *) rasters[o_elongation].buffer)[col] =
			    elongation;
		    if (opt_output[o_width]->answer)
			((FCELL *) rasters[o_width].buffer)[col] = width;
		}
		if (opt_output[o_extend]->answer)
		    ((FCELL *) rasters[o_extend].buffer)[col] =
			extends(pattern, pattern_size) / area_of_octagon;

	    }			/* end for col */

	    /* write existing outputs */
	    for (i = 1; i < io_size; ++i)
		if (opt_output[i]->answer)
		    Rast_put_row(rasters[i].fd, rasters[i].buffer,
				 rasters[i].out_data_type);
	}
	G_percent(row, nrows, 2);	/* end main loop */

	/* finish and close */
	free_map(elevation.elev, row_buffer_size + 1);
	for (i = 1; i < io_size; ++i)
	    if (opt_output[i]->answer) {
		G_free(rasters[i].buffer);
		Rast_close(rasters[i].fd);
		Rast_short_history(opt_output[i]->answer, "raster", &history);
		Rast_command_history(&history);
		Rast_write_history(opt_output[i]->answer, &history);
	    }

	if (opt_output[o_forms]->answer)
	    write_form_cat_colors(opt_output[o_forms]->answer, ccolors);
	if (opt_output[o_intensity]->answer)
	    write_contrast_colors(opt_output[o_intensity]->answer);
	if (opt_output[o_exposition]->answer)
	    write_contrast_colors(opt_output[o_exposition]->answer);
	if (opt_output[o_range]->answer)
	    write_contrast_colors(opt_output[o_range]->answer);

	G_done_msg(" ");
	exit(EXIT_SUCCESS);
    }				/* end of multiresolution */

    if (multires) {
	PATTERN *multi_patterns;
	MULTI multiple_output[5];	/* ten form maps + all forms */
	char *postfixes[] = { "scale_300", "scale_100", "scale_50", "scale_20" "scale_10" };	/* in pixels */
	num_of_steps = 5;
	multi_patterns = G_malloc(num_of_steps * sizeof(PATTERN));
	/* prepare outputs */
	for (i = 0; i < 5; ++i) {
	    multiple_output[i].forms_buffer = Rast_allocate_buf(CELL_TYPE);
	    strcpy(multiple_output[i].name, prefix);
	    strcat(multiple_output[i].name, postfixes[i]);
	    multiple_output[i].fd =
		Rast_open_new(multiple_output[i].name, CELL_TYPE);
	}

	/* main loop */
	for (row = 0; row < nrows; ++row) {
	    G_percent(row, nrows, 2);
	    cur_row = (row < row_radius_size) ? row :
		((row >=
		  nrows - row_radius_size - 1) ? row_buffer_size - (nrows -
								    row -
								    1) :
		 row_radius_size);

	    if (row > (row_radius_size) &&
		row < nrows - (row_radius_size + 1))
		shift_buffers(row);
	    for (col = 0; col < ncols; ++col) {
		if (row < (skip_cells + 1) || row > nrows - (skip_cells + 2)
		    || col < (skip_cells + 1) ||
		    col > ncols - (skip_cells + 2) ||
		    Rast_is_f_null_value(&elevation.elev[cur_row][col])) {
		    for (i = 0; i < num_of_steps; ++i)
			Rast_set_c_null_value(&multiple_output[i].
					      forms_buffer[col], 1);
		    continue;
		}
		cell_step = 10;
		calc_pattern(&multi_patterns[0], row, cur_row, col);
	    }

	    for (i = 0; i < num_of_steps; ++i)
		Rast_put_row(multiple_output[i].fd,
			     multiple_output[i].forms_buffer, CELL_TYPE);

	}
	G_percent(row, nrows, 2);	/* end main loop */

	for (i = 0; i < num_of_steps; ++i) {
	    G_free(multiple_output[i].forms_buffer);
	    Rast_close(multiple_output[i].fd);
	    Rast_short_history(multiple_output[i].name, "raster", &history);
	    Rast_command_history(&history);
	    Rast_write_history(multiple_output[i].name, &history);
	}
	G_message("Multiresolution Done!");
	exit(EXIT_SUCCESS);
    }

}
