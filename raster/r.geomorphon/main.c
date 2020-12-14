
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
*		(C) Scientific idea of geomorphon copyrighted to authors.
*
*		This program is free software under the GNU General Public
*		License (>=v2). Read the file COPYING that comes with GRASS
*		for details.
*
 *****************************************************************************/

#include <stdio.h>
#include <errno.h>
#define MAIN
#include "local_proto.h"

#define WINDOW_THRESHOLD 100000000

typedef enum
{ o_forms, o_ternary, o_positive, o_negative, o_intensity,
    o_exposition,
    o_range, o_variance, o_elongation, o_azimuth, o_extend, o_width, o_size
} outputs;

typedef struct
{
    char name[100];
    int fd;
    CELL *forms_buffer;
} MULTI;

static const char *form_short_name(const FORMS f)
{
    const char *form_short_names[] = {
        /* skip 0 */
        [FL] = "FL",
        [PK] = "PK",
        [RI] = "RI",
        [SH] = "SH",
        [SP] = "SP",
        [SL] = "SL",
        [HL] = "HL",
        [FS] = "FS",
        [VL] = "VL",
        [PT] = "PT",
        [__] = "ERROR",
    };
    return (f >= FL && f <= PT) ? form_short_names[f] : form_short_names[__];
}

static const char *form_long_name(const FORMS f)
{
    /*
     * The list below is the same as in the documentation and the original paper.
     * The values in ccolors[] are different for PK and PT.
     */
    const char *form_long_names[] = {
        /* skip 0 */
        [FL] = "flat",
        [PK] = "peak",
        [RI] = "ridge",
        [SH] = "shoulder",
        [SP] = "spur",
        [SL] = "slope",
        [HL] = "hollow",
        [FS] = "footslope",
        [VL] = "valley",
        [PT] = "pit",
        [__] = "ERROR",
    };
    return (f >= FL && f <= PT) ? form_long_names[f] : form_long_names[__];
}

int main(int argc, char **argv)
{
    struct
    {                           /* struct is used both for interface and output */
        const char *name;
        const char *description;
        const char *gui;
        RASTER_MAP_TYPE out_data_type;
        int fd;
        void *buffer;
    } rasters[] = {             /* rasters stores output buffers */
        {
        "forms", "Most common geomorphic forms", "Patterns", CELL_TYPE,
                -1, NULL}, {
        "ternary", "Code of ternary patterns", "Patterns", CELL_TYPE, -1,
                NULL}, {
        "positive", "Code of binary positive patterns", "Patterns",
                CELL_TYPE, -1, NULL}, {
        "negative", "Code of binary negative patterns", "Patterns",
                CELL_TYPE, -1, NULL}, {
        "intensity",
                "Rasters containing mean relative elevation of the form",
                "Geometry", FCELL_TYPE, -1, NULL}, {
        "exposition",
                "Rasters containing maximum difference between extend and central cell",
                "Geometry", FCELL_TYPE, -1, NULL}, {
        "range",
                "Rasters containing difference between max and min elevation of the form extend",
                "Geometry", FCELL_TYPE, -1, NULL}, {
        "variance", "Rasters containing variance of form boundary",
                "Geometry", FCELL_TYPE, -1, NULL}, {
        "elongation", "Rasters containing local elongation", "Geometry",
                FCELL_TYPE, -1, NULL}, {
        "azimuth", "Rasters containing local azimuth of the elongation",
                "Geometry", FCELL_TYPE, -1, NULL}, {
        "extend", "Rasters containing local extend (area) of the form",
                "Geometry", FCELL_TYPE, -1, NULL}, {
        "width", "Rasters containing local width of the form", "Geometry",
                FCELL_TYPE, -1, NULL}
    };

    struct GModule *module;
    struct Option
        *opt_input,
        *opt_output[o_size],
        *par_search_radius,
        *par_skip_radius,
        *par_flat_threshold,
        *par_flat_distance,
        *par_comparison,
        *par_coords,
        *par_profiledata,
        *par_profileformat,
        *par_multi_prefix, *par_multi_step, *par_multi_start;
    struct Flag *flag_units, *flag_extended;

    struct History history;

    int i;
    int meters = 0, multires = 0, extended = 0; /* flags */
    int oneoff;
    int row, cur_row, col;
    int nrows;
    int pattern_size;
    int search_cells, step_cells, start_cells;
    int num_of_steps;
    double start_distance, step_distance;       /* multiresolution mode */
    double skip_distance;
    double max_resolution;
    char prefix[20];
    double oneoff_easting, oneoff_northing;
    int oneoff_row, oneoff_col;
    FILE *profile_file;

    G_gisinit(argv[0]);

    {                           /* interface  parameters */
        module = G_define_module();
        module->description =
            _("Calculates geomorphons (terrain forms) and associated geometry using machine vision approach.");
        G_add_keyword(_("raster"));
        G_add_keyword(_("geomorphons"));
        G_add_keyword(_("terrain patterns"));
        G_add_keyword(_("machine vision geomorphometry"));

        opt_input = G_define_standard_option(G_OPT_R_ELEV);

        for (i = o_forms; i < o_size; ++i) {
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
        par_flat_threshold->description = _("Flatness threshold (degrees)");

        par_flat_distance = G_define_option();
        par_flat_distance->key = "dist";
        par_flat_distance->type = TYPE_DOUBLE;
        par_flat_distance->answer = "0";
        par_flat_distance->required = YES;
        par_flat_distance->description =
            _("Flatness distance, zero for none");

        par_comparison = G_define_option();
        par_comparison->key = "comparison";
        par_comparison->type = TYPE_STRING;
        par_comparison->options = "anglev1,anglev2,anglev2_distance";
        par_comparison->answer = "anglev1";
        par_comparison->required = NO;
        par_comparison->description =
            _("Comparison mode for zenith/nadir line-of-sight search");

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
            _("Distance where search will start in multiple mode (zero to omit)");
        par_multi_start->guisection = _("Multires");

        flag_units = G_define_flag();
        flag_units->key = 'm';
        flag_units->description =
            _("Use meters to define search units (default is cells)");

        flag_extended = G_define_flag();
        flag_extended->key = 'e';
        flag_extended->description = _("Use extended form correction");

        par_coords = G_define_standard_option(G_OPT_M_COORDS);
        par_coords->description = _("Coordinates to profile");
        par_coords->guisection = _("Profile");
        G_option_excludes(par_coords, par_multi_prefix, NULL);
        for (i = o_forms; i < o_size; i++)
            G_option_excludes(par_coords, opt_output[i], NULL);

        par_profiledata = G_define_standard_option(G_OPT_F_OUTPUT);
        par_profiledata->key = "profiledata";
        par_profiledata->answer = "-";
        par_profiledata->required = NO;
        par_profiledata->description =
            _("Profile output file name (\"-\" for stdout)");
        par_profiledata->guisection = _("Profile");
        G_option_requires(par_profiledata, par_coords, NULL);

        par_profileformat = G_define_option();
        par_profileformat->key = "profileformat";
        par_profileformat->type = TYPE_STRING;
        par_profileformat->options = "json,yaml,xml";
        par_profileformat->answer = "json";
        par_profileformat->required = NO;
        par_profileformat->description = _("Profile output format");
        par_profileformat->guisection = _("Profile");
        G_option_requires(par_profileformat, par_coords, NULL);

        if (G_parser(argc, argv))
            exit(EXIT_FAILURE);
    }

    {                           /* calculate parameters */
        int num_outputs = 0;
        double search_radius, skip_radius, start_radius, step_radius;
        double ns_resolution;

        multires = (par_multi_prefix->answer) ? 1 : 0;
        if (!strcmp(par_comparison->answer, "anglev1"))
            compmode = ANGLEV1;
        else if (!strcmp(par_comparison->answer, "anglev2"))
            compmode = ANGLEV2;
        else if (!strcmp(par_comparison->answer, "anglev2_distance"))
            compmode = ANGLEV2_DISTANCE;
        else
            G_fatal_error(_("Failed parsing <%s>"), par_comparison->answer);
        oneoff = par_coords->answer != NULL;
        for (i = o_forms; i < o_size; ++i)      /* check for outputs */
            if (opt_output[i]->answer) {
                if (G_legal_filename(opt_output[i]->answer) < 0)
                    G_fatal_error(_("<%s> is an illegal file name"),
                                  opt_output[i]->answer);
                num_outputs++;
            }
        if (!num_outputs && !multires && !oneoff)
            G_fatal_error(_("At least one output is required, e.g. %s"),
                          opt_output[o_forms]->key);

        meters = (flag_units->answer != 0);
        extended = (flag_extended->answer != 0);
        nrows = Rast_window_rows();
        ncols = Rast_window_cols();
        Rast_get_window(&window);
        G_begin_distance_calculations();

        if (oneoff) {
            if (!G_scan_easting
                (par_coords->answers[0], &oneoff_easting, G_projection()))
                G_fatal_error(_("Illegal east coordinate <%s>"),
                              par_coords->answers[0]);
            oneoff_col = Rast_easting_to_col(oneoff_easting, &window);
            if (!G_scan_northing
                (par_coords->answers[1], &oneoff_northing, G_projection()))
                G_fatal_error(_("Illegal north coordinate <%s>"),
                              par_coords->answers[1]);
            oneoff_row = Rast_northing_to_row(oneoff_northing, &window);
            if (oneoff_row < 0 || oneoff_row >= nrows ||
                oneoff_col < 0 || oneoff_col >= ncols)
                G_fatal_error(_("The coordinates are outside of the computational region"));

            if (!strcmp(par_profiledata->answer, "-"))
                profile_file = stdout;
            else {
                profile_file = fopen(par_profiledata->answer, "w");
                if (!profile_file)
                    G_fatal_error(_("Failed to open output file <%s>: %d (%s)"),
                                  par_profiledata->answer, errno,
                                  strerror(errno));
            }
        }

        if (G_projection() == PROJECTION_LL) {  /* for LL max_res should be NS */
            ns_resolution =
                G_distance(0, Rast_row_to_northing(0, &window), 0,
                           Rast_row_to_northing(1, &window));
            max_resolution = ns_resolution;
        }
        else {
            max_resolution = MAX(window.ns_res, window.ew_res); /* max_resolution MORE meters per cell */
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
            G_fatal_error(_("Flatness threshold must be grater than 0"));
        flat_threshold = DEGREE2RAD(flat_threshold);

        flat_distance = atof(par_flat_distance->answer);
        flat_distance =
            (meters) ? flat_distance : ns_resolution * flat_distance;
        flat_threshold_height = tan(flat_threshold) * flat_distance;
        if ((flat_distance > 0 && flat_distance <= skip_distance) ||
            flat_distance >= search_distance) {
            G_warning(_("Flatness distance should be between skip and search radius. Otherwise ignored"));
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
        }                       /* end multires preparation */

        /* print information about distances */
        G_verbose_message("Search distance m: %f, cells: %d", search_distance,
                          search_cells);
        G_verbose_message("Skip distance m: %f, cells: %d", skip_distance,
                          skip_cells);
        G_verbose_message("Flat threshold distance m: %f, height: %f",
                          flat_distance, flat_threshold_height);
        G_verbose_message("%s version", (extended) ? "Extended" : "Basic");
        /*
         * FIXME: It would be nice to have an additional "-s" flag to set the
         * current computational region just large enough for the one-off
         * computation (plus some margins and any required outward alignment
         * to the cell boundary). And also perhaps another "-r" flag to
         * restore the region afterwards.
         */
        if (oneoff) {
            unsigned long window_square = nrows * ncols;
            unsigned long search_square = 4 * search_cells * search_cells;

            if (window_square > WINDOW_THRESHOLD &&
                window_square / search_square > 10)
                G_warning(_("There may be a notable processing delay because the "
                           "computational region is %lu times larger than necessary"),
                          window_square / search_square);
        }
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

    generate_ternary_codes();

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
        unsigned char oneoff_done = 0;

        cell_step = 1;
        /* prepare outputs */
        for (i = o_forms; i < o_size; ++i)
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

            /* If skipping the current row, only after the buffer shift would be fine. */
            if (oneoff && row != oneoff_row)
                continue;

            for (col = 0; col < ncols; ++col) {
                /* If skipping the current column, early would be fine. */
                if (oneoff && col != oneoff_col)
                    continue;

                /* on borders forms ussualy are innatural. */
                if (row < (skip_cells + 1) || row > nrows - (skip_cells + 2)
                    || col < (skip_cells + 1) ||
                    col > ncols - (skip_cells + 2) ||
                    Rast_is_f_null_value(&elevation.elev[cur_row][col])) {
                    /* set outputs to NULL and do nothing if source value is null   or border */
                    for (i = o_forms; i < o_size; ++i)
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
                }               /* end null value */
                {
                    FORMS cur_form;
                    FORMS orig_form;

                    search_distance = search_dist;
                    skip_distance = skip_dist;
                    flat_distance = flat_dist;
                    pattern_size =
                        calc_pattern(&patterns[0], row, cur_row, col, oneoff);
                    pattern = &patterns[0];
                    cur_form =
                        orig_form =
                        determine_form(pattern->num_negatives,
                                       pattern->num_positives);

                    /* correction of forms */
                    if (extended && search_distance > 10 * max_resolution) {
                        /* 1) remove extensive innatural forms: ridges, peaks, shoulders and footslopes */
                        if ((cur_form == SH || cur_form == FS ||
                             cur_form == PK || cur_form == RI)) {
                            FORMS small_form;

                            search_distance =
                                (search_dist / 2. <
                                 4 * max_resolution) ? 4 *
                                max_resolution : search_dist / 2.;
                            skip_distance = 0;
                            flat_distance = 0;
                            pattern_size =
                                calc_pattern(&patterns[1], row, cur_row, col, 0);
                            pattern = &patterns[1];
                            small_form =
                                determine_form(pattern->num_negatives,
                                               pattern->num_positives);
                            if (cur_form == SH || cur_form == FS)
                                cur_form = (small_form == FL) ? FL : cur_form;
                            if (cur_form == PK || cur_form == RI)
                                cur_form = small_form;
                        }
                        /* 3) Depressions */

                    }           /* end of correction */

                    /* one-off mode */
                    if (oneoff) {
                        char buf[BUFSIZ];
                        float azimuth, elongation, width;

                        radial2cartesian(pattern);
                        shape(pattern, pattern_size, &azimuth, &elongation,
                              &width);
                        prof_map_info();
                        prof_sso("computation_parameters");
                        prof_dbl("easting", oneoff_easting);
                        prof_dbl("northing", oneoff_northing);
                        prof_mtr("search_m", search_distance);
                        prof_int("search_cells", search_cells);
                        prof_mtr("skip_m", skip_distance);
                        prof_int("skip_cells", skip_cells);
                        prof_dbl("flat_thresh_deg",
                                 RAD2DEGREE(flat_threshold));
                        prof_mtr("flat_distance_m", flat_distance);
                        prof_mtr("flat_height_m", flat_threshold_height);
                        prof_bln("extended_correction", extended);
                        prof_eso();     /* computation_parameters */
                        prof_sso("intermediate_data");
                        if (extended) {
                            prof_int("initial_landform_cat", orig_form);
                            prof_str("initial_landform_code",
                                     form_short_name(orig_form));
                            prof_str("initial_landform_name",
                                     form_long_name(orig_form));
                        }
                        prof_int("ternary_498",
                                 determine_ternary(pattern->pattern));
                        prof_int("ternary_6561",
                                 preliminary_ternary(pattern->pattern));
                        prof_int("pattern_size", pattern_size);
                        prof_dbl("origin_easting",
                                 Rast_col_to_easting(col + 0.5, &window));
                        prof_dbl("origin_northing",
                                 Rast_row_to_northing(row + 0.5, &window));
                        prof_pattern(elevation.elev[cur_row][col], pattern);
                        prof_eso();     /* intermediate_data */
                        prof_sso("final_results");
                        prof_int("landform_cat", cur_form);
                        prof_str("landform_code", form_short_name(cur_form));
                        prof_str("landform_name", form_long_name(cur_form));
                        prof_int("landform_deviation",
                                 form_deviation(pattern->num_negatives,
                                                pattern->num_positives));
                        prof_dbl("azimuth", azimuth);
                        prof_dbl("elongation", elongation);
                        prof_mtr("width_m", width);
                        prof_mtr("intensity_m",
                                 intensity(pattern->elevation, pattern_size));
                        prof_mtr("exposition_m",
                                 exposition(pattern->elevation));
                        prof_mtr("range_m", range(pattern->elevation));
                        prof_dbl("variance",
                                 variance(pattern->elevation, pattern_size));
                        prof_dbl("extends",
                                 extends(pattern) / area_of_octagon);
                        prof_mtr("octagon_perimeter_m",
                                 octa_perimeter(pattern));
                        prof_mtr("octagon_area_m2", extends(pattern));
                        prof_mtr("mesh_perimeter_m", mesh_perimeter(pattern));
                        prof_mtr("mesh_area_m2", mesh_area(pattern));
                        prof_eso();     /* final_results */
                        /*
                         * When adding new data items, increment the minor
                         * version. When deleting, moving, renaming or
                         * otherwise changing existing data, increment the
                         * major version and reset the minor version.
                         */
                        prof_int("format_version_major", 0);
                        prof_int("format_version_minor", 9);
                        prof_utc("timestamp", time(NULL));
                        G_snprintf(buf, sizeof(buf),
                                   "r.geomorphon GRASS GIS %s [%s]",
                                   GRASS_VERSION_STRING,
                                   GRASS_HEADERS_VERSION);
                        prof_str("generator", buf);

                        oneoff_done =
                            prof_write(profile_file,
                                       par_profileformat->answer);
                        if (oneoff_done)
                            G_verbose_message(_("Profile data has been written"));
                        else
                            G_important_message(_("Failed writing profile data"));
                        /* Break out of both loops. */
                        row = nrows - 1;
                        break;
                    }           /* end of one-off mode */

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
                        extends(pattern) / area_of_octagon;

            }                   /* end for col */

            /* write existing outputs */
            for (i = o_forms; i < o_size; ++i)
                if (opt_output[i]->answer)
                    Rast_put_row(rasters[i].fd, rasters[i].buffer,
                                 rasters[i].out_data_type);
        }
        G_percent(row, nrows, 2);       /* end main loop */

        /* finish and close */
        free_map(elevation.elev, row_buffer_size + 1);
        for (i = o_forms; i < o_size; ++i)
            if (opt_output[i]->answer) {
                G_free(rasters[i].buffer);
                Rast_close(rasters[i].fd);
                Rast_short_history(opt_output[i]->answer, "raster", &history);
                Rast_command_history(&history);
                Rast_write_history(opt_output[i]->answer, &history);
            }

        if (opt_output[o_forms]->answer)
            write_form_cat_colors(opt_output[o_forms]->answer);
        if (opt_output[o_intensity]->answer)
            write_contrast_colors(opt_output[o_intensity]->answer);
        if (opt_output[o_exposition]->answer)
            write_contrast_colors(opt_output[o_exposition]->answer);
        if (opt_output[o_range]->answer)
            write_contrast_colors(opt_output[o_range]->answer);

        G_done_msg(" ");

        if (oneoff) {
            if (strcmp(par_profiledata->answer, "-"))
                fclose(profile_file);
            /*
             * In case all the earlier checks had not detected some edge case
             * and the one-off computation has not been done properly or not
             * at all, do not fail to fail now.
             */
            if (!oneoff_done) {
                G_fatal_error(_("Failed to profile the computation, please check the parameters"));
                exit(EXIT_FAILURE);
            }
        }

        exit(EXIT_SUCCESS);
    }                           /* end of NOT multiresolution */

    if (multires) {
        PATTERN *multi_patterns;
        MULTI multiple_output[5];       /* ten form maps + all forms */
        char *postfixes[] = { "scale_300", "scale_100", "scale_50", "scale_20", "scale_10" };   /* in pixels */
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
                        Rast_set_c_null_value(&multiple_output[i].forms_buffer
                                              [col], 1);
                    continue;
                }
                cell_step = 10;
                calc_pattern(&multi_patterns[0], row, cur_row, col, 0);
            }

            for (i = 0; i < num_of_steps; ++i)
                Rast_put_row(multiple_output[i].fd,
                             multiple_output[i].forms_buffer, CELL_TYPE);

        }
        G_percent(row, nrows, 2);       /* end main loop */

        for (i = 0; i < num_of_steps; ++i) {
            G_free(multiple_output[i].forms_buffer);
            Rast_close(multiple_output[i].fd);
            Rast_short_history(multiple_output[i].name, "raster", &history);
            Rast_command_history(&history);
            Rast_write_history(multiple_output[i].name, &history);
        }
        G_message("Multiresolution Done!");
        exit(EXIT_SUCCESS);
    }                           /* end of multiresolution */

}
