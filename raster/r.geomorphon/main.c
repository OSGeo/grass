/****************************************************************************
 *
 * MODULE:       r.geomorphon
 * AUTHOR(S):    Jarek Jasiewicz jarekj amu.edu.pl with collaboration of Tom
 *               Stepinski stepintz uc.edu based on idea of Tomasz Stepinski
 *               and Jaroslaw Jasiewicz
 *
 * PURPOSE:      Calculate terrain forms using machine-vison technique called
 *               geomorphons This module allow to calculate standard set of
 *               terrain forms using look-up table proposed by authors,
 *               calculate patterns (binary and ternary) for every pixel as
 *               well as several geomorphometrical parameters This technology
 *               is currently capable of "experimental" stage.
 *
 * COPYRIGHT:    (C) 2002,2012 by the GRASS Development Team
 *               (C) Scientific idea of geomorphon copyrighted to authors.
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#if defined(_OPENMP)
#include <omp.h>
#endif

#include <stdio.h>
#include <errno.h>
#define MAIN
#include "local_proto.h"

#define WINDOW_THRESHOLD 100000000

typedef enum {
    o_forms,
    o_ternary,
    o_positive,
    o_negative,
    o_intensity,
    o_exposition,
    o_range,
    o_variance,
    o_elongation,
    o_azimuth,
    o_extend,
    o_width,
    o_size
} outputs;

/* Compute the geomorphon pattern and landform for one cell, including the
 * extended-form correction. Shared by the parallel raster path and the serial
 * one-off path. The effective search, skip and flatness distances are returned
 * for the one-off profile; the raster path ignores them. */
static void compute_forms(FCELL **rows, int cur_row, int row, int col,
                          double search_dist, double skip_dist,
                          double flat_dist, int extended, double max_resolution,
                          int oneoff, PATTERN *patterns, PATTERN **pattern_out,
                          int *pattern_size_out, FORMS *cur_form_out,
                          FORMS *orig_form_out, double *eff_search_out,
                          double *eff_skip_out, double *eff_flat_out)
{
    double eff_search = search_dist;
    double eff_skip = skip_dist;
    double eff_flat = flat_dist;
    PATTERN *pattern;
    int pattern_size;
    FORMS cur_form, orig_form;

    pattern_size = calc_pattern(&patterns[0], row, cur_row, col, oneoff,
                                eff_search, eff_flat, rows);
    pattern = &patterns[0];
    cur_form = orig_form =
        determine_form(pattern->num_negatives, pattern->num_positives);

    if (extended && eff_search > 10 * max_resolution) {
        if ((cur_form == SH || cur_form == FS || cur_form == PK ||
             cur_form == RI)) {
            FORMS small_form;

            eff_search = (search_dist / 2. < 4 * max_resolution)
                             ? 4 * max_resolution
                             : search_dist / 2.;
            eff_skip = 0;
            eff_flat = 0;
            pattern_size = calc_pattern(&patterns[1], row, cur_row, col, 0,
                                        eff_search, eff_flat, rows);
            pattern = &patterns[1];
            small_form =
                determine_form(pattern->num_negatives, pattern->num_positives);
            if (cur_form == SH || cur_form == FS)
                cur_form = (small_form == FL) ? FL : cur_form;
            if (cur_form == PK || cur_form == RI)
                cur_form = small_form;
        }
    }
    *pattern_out = pattern;
    *pattern_size_out = pattern_size;
    *cur_form_out = cur_form;
    *orig_form_out = orig_form;
    *eff_search_out = eff_search;
    *eff_skip_out = eff_skip;
    *eff_flat_out = eff_flat;
}

int main(int argc, char **argv)
{
    struct { /* struct is used both for interface and output */
        const char *name;
        const char *description;
        const char *gui;
        RASTER_MAP_TYPE out_data_type;
        int fd;
        void *buffer;
    } rasters[] = {
        /* rasters stores output buffers */
        {"forms", "Most common geomorphic forms", "Patterns", CELL_TYPE, -1,
         NULL},
        {"ternary", "Code of ternary patterns", "Patterns", CELL_TYPE, -1,
         NULL},
        {"positive", "Code of binary positive patterns", "Patterns", CELL_TYPE,
         -1, NULL},
        {"negative", "Code of binary negative patterns", "Patterns", CELL_TYPE,
         -1, NULL},
        {"intensity", "Rasters containing mean relative elevation of the form",
         "Geometry", FCELL_TYPE, -1, NULL},
        {"exposition",
         "Rasters containing maximum difference between extend and central "
         "cell",
         "Geometry", FCELL_TYPE, -1, NULL},
        {"range",
         "Rasters containing difference between max and min elevation of the "
         "form extend",
         "Geometry", FCELL_TYPE, -1, NULL},
        {"variance", "Rasters containing variance of form boundary", "Geometry",
         FCELL_TYPE, -1, NULL},
        {"elongation", "Rasters containing local elongation", "Geometry",
         FCELL_TYPE, -1, NULL},
        {"azimuth", "Rasters containing local azimuth of the elongation",
         "Geometry", FCELL_TYPE, -1, NULL},
        {"extend", "Rasters containing local extend (area) of the form",
         "Geometry", FCELL_TYPE, -1, NULL},
        {"width", "Rasters containing local width of the form", "Geometry",
         FCELL_TYPE, -1, NULL}};

    struct GModule *module;
    struct Option *opt_input, *opt_output[o_size], *par_search_radius,
        *par_skip_radius, *par_flat_threshold, *par_flat_distance,
        *par_comparison, *par_coords, *par_profiledata, *par_profileformat,
        *par_memory, *par_nprocs;
    struct Flag *flag_units, *flag_extended;

    struct History history;

    int i;
    int meters = 0, extended = 0; /* flags */
    int oneoff;
    int nrows;
    int brows;  /* rows per output band buffer */
    int memory; /* memory cap in MB from the memory= option */
    int nprocs; /* number of OpenMP threads */
    int search_cells;
    double search_distance, flat_distance;
    double skip_distance;
    double max_resolution;
    double oneoff_easting, oneoff_northing;
    int oneoff_row, oneoff_col;
    FILE *profile_file;

    G_gisinit(argv[0]);

    { /* interface  parameters */
        module = G_define_module();
        module->description =
            _("Calculates geomorphons (terrain forms) and associated geometry "
              "using machine vision approach.");
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
        par_flat_distance->description = _("Flatness distance, zero for none");

        par_comparison = G_define_option();
        par_comparison->key = "comparison";
        par_comparison->type = TYPE_STRING;
        par_comparison->options = "anglev1,anglev2,anglev2_distance";
        par_comparison->answer = "anglev1";
        par_comparison->required = NO;
        par_comparison->description =
            _("Comparison mode for zenith/nadir line-of-sight search");

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
        for (i = o_forms; i < o_size; i++)
            G_option_excludes(par_coords, opt_output[i], NULL);

        par_profiledata = G_define_standard_option(G_OPT_F_OUTPUT);
        par_profiledata->key = "profiledata";
        par_profiledata->required = NO;
        par_profiledata->description =
            _("Profile output file name (\"-\" for stdout)");
        par_profiledata->guisection = _("Profile");
        G_option_requires(par_profiledata, par_coords, NULL);
        G_option_requires(par_coords, par_profiledata, NULL);

        par_profileformat = G_define_option();
        par_profileformat->key = "profileformat";
        par_profileformat->type = TYPE_STRING;
        par_profileformat->options = "json,yaml,xml";
        par_profileformat->required = NO;
        par_profileformat->description = _("Profile output format");
        par_profileformat->guisection = _("Profile");
        G_option_requires(par_profileformat, par_coords, NULL);
        G_option_requires(par_coords, par_profileformat, NULL);

        par_nprocs = G_define_standard_option(G_OPT_M_NPROCS);
        par_memory = G_define_standard_option(G_OPT_MEMORYMB);

        if (G_parser(argc, argv))
            exit(EXIT_FAILURE);
    }

    { /* calculate parameters */
        int num_outputs = 0;
        double search_radius, skip_radius;
        double ns_resolution;

        if (!strcmp(par_comparison->answer, "anglev1"))
            compmode = ANGLEV1;
        else if (!strcmp(par_comparison->answer, "anglev2"))
            compmode = ANGLEV2;
        else if (!strcmp(par_comparison->answer, "anglev2_distance"))
            compmode = ANGLEV2_DISTANCE;
        else
            G_fatal_error(_("Failed parsing <%s>"), par_comparison->answer);
        oneoff = par_coords->answer != NULL;
        for (i = o_forms; i < o_size; ++i) /* check for outputs */
            if (opt_output[i]->answer) {
                if (G_legal_filename(opt_output[i]->answer) < 0)
                    G_fatal_error(_("<%s> is an illegal file name"),
                                  opt_output[i]->answer);
                num_outputs++;
            }
        if (!num_outputs && !oneoff)
            G_fatal_error(_("At least one output is required, e.g. %s"),
                          opt_output[o_forms]->key);

        meters = (flag_units->answer != 0);
        extended = (flag_extended->answer != 0);
        nrows = Rast_window_rows();
        ncols = Rast_window_cols();
        Rast_get_window(&window);
        G_begin_distance_calculations();

        if (oneoff) {
            if (!G_scan_easting(par_coords->answers[0], &oneoff_easting,
                                G_projection()))
                G_fatal_error(_("Illegal east coordinate <%s>"),
                              par_coords->answers[0]);
            oneoff_col = Rast_easting_to_col(oneoff_easting, &window);
            if (!G_scan_northing(par_coords->answers[1], &oneoff_northing,
                                 G_projection()))
                G_fatal_error(_("Illegal north coordinate <%s>"),
                              par_coords->answers[1]);
            oneoff_row = Rast_northing_to_row(oneoff_northing, &window);
            if (oneoff_row < 0 || oneoff_row >= nrows || oneoff_col < 0 ||
                oneoff_col >= ncols)
                G_fatal_error(_(
                    "The coordinates are outside of the computational region"));

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

        if (G_projection() == PROJECTION_LL) { /* for LL max_res should be NS */
            ns_resolution = G_distance(0, Rast_row_to_northing(0, &window), 0,
                                       Rast_row_to_northing(1, &window));
            max_resolution = ns_resolution;
        }
        else {
            max_resolution =
                MAX(window.ns_res,
                    window.ew_res); /* max_resolution MORE meters per cell */
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
        /* open_map fills the row buffer by reading row_buffer_size + 1 rows
         * from the map, so a region with fewer rows than that would die
         * partway through that read with a confusing low level error. */
        if (nrows < row_buffer_size + 1)
            G_fatal_error(
                _("The current region has only %d rows but search=%s needs "
                  "at least %d rows. Make the region taller with g.region "
                  "or use a smaller search value."),
                nrows, par_search_radius->answer, row_buffer_size + 1);
        /* Band height from the memory cap. The shared input strip is
         * (brows + 2 * row_radius_size) rows and each requested output adds a
         * brows-row band, so both terms scale with brows. CELL and FCELL are
         * both four bytes. */
        memory = atoi(par_memory->answer);
        nprocs = G_set_omp_num_threads(par_nprocs);
        nprocs = Rast_disable_omp_on_mask(nprocs);
        if (nprocs < 1)
            G_fatal_error(_("<%d> is not valid number of nprocs."), nprocs);
        {
            size_t fixed = (size_t)2 * row_radius_size * ncols * sizeof(FCELL);
            size_t cap = (size_t)memory * (1 << 20);
            size_t per_row = (size_t)ncols * sizeof(FCELL) * (1 + num_outputs);
            size_t budget;

            if (cap <= fixed) {
                long need = (long)((fixed + per_row + (1 << 20) - 1) >> 20);

                G_warning(
                    _("Requested memory=%d MB is too small for search=%s. "
                      "At least %ld MB is needed. Proceeding with a larger "
                      "buffer than requested."),
                    memory, par_search_radius->answer, need);
                budget = 0;
            }
            else
                budget = cap - fixed;
            size_t brows_sz = budget / per_row;

            if (brows_sz > (size_t)nrows)
                brows = nrows;
            else
                brows = (int)brows_sz;
            if (brows < nprocs)
                brows = nprocs;
        }
        /* One-off profiles a single cell (oneoff was set above), so a single
         * band on its row suffices and there is no need to size for memory. */
        if (oneoff)
            brows = 1;
        search_distance =
            (meters) ? search_radius : ns_resolution * search_cells;

        /* skip distance */
        skip_radius = atof(par_skip_radius->answer);
        skip_cells = meters ? (int)(skip_radius / max_resolution) : skip_radius;
        if (skip_cells >= search_cells)
            G_fatal_error(_(
                "Skip radius size must be at least 1 cell lower than radius"));
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
            G_warning(_("Flatness distance should be between skip and search "
                        "radius. Otherwise ignored"));
            flat_distance = 0;
        }

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
                G_warning(
                    _("There may be a notable processing delay because the "
                      "computational region is %lu times larger than "
                      "necessary"),
                    window_square / search_square);
        }
    }

    generate_ternary_codes();

    /* open DEM */
    strcpy(elevation.elevname, opt_input->answer);
    open_map(&elevation);

    {
        double search_dist = search_distance;
        double skip_dist = skip_distance;
        double flat_dist = flat_distance;
        double area_of_octagon =
            4 * (search_distance * search_distance) * sin(DEGREE2RAD(45.));
        unsigned char oneoff_done = 0;
        /* One shared input strip (band rows plus halo), reused for every band.
         */
        int strip_cap = brows + 2 * row_radius_size + 1;
        FCELL *strip_block =
            (FCELL *)G_malloc((size_t)strip_cap * ncols * sizeof(FCELL));
        FCELL **strip_ptr =
            (FCELL **)G_malloc((size_t)strip_cap * sizeof(FCELL *));

        if (oneoff) {
            /* Serial single-cell profile, outside any parallel region. */
            void *tmp_buf = Rast_allocate_buf(elevation.raster_type);
            int win_top = MIN(MAX(oneoff_row - row_radius_size, 0),
                              nrows - row_buffer_size - 1);
            int strip_lo = win_top;
            int strip_hi = win_top + row_buffer_size;
            int strip_rows, cur_row = oneoff_row - win_top, k;
            FCELL **rows;

            if (strip_hi > nrows - 1)
                strip_hi = nrows - 1;
            strip_rows = strip_hi - strip_lo + 1;
            for (k = 0; k < strip_rows; ++k)
                strip_ptr[k] = strip_block + (size_t)k * ncols;
            load_strip(elevation.fd, elevation.raster_type, tmp_buf, strip_ptr,
                       strip_lo, strip_rows);
            rows = &strip_ptr[win_top - strip_lo];

            if (!(oneoff_row < skip_cells + 1 ||
                  oneoff_row > nrows - (skip_cells + 2) ||
                  oneoff_col < skip_cells + 1 ||
                  oneoff_col > ncols - (skip_cells + 2) ||
                  Rast_is_f_null_value(&rows[cur_row][oneoff_col]))) {
                PATTERN patterns[4];
                PATTERN *pattern;
                int pattern_size;
                FORMS cur_form, orig_form;
                double eff_search, eff_skip, eff_flat;
                char buf[BUFSIZ];
                float azimuth, elongation, width;

                compute_forms(rows, cur_row, oneoff_row, oneoff_col,
                              search_dist, skip_dist, flat_dist, extended,
                              max_resolution, 1, patterns, &pattern,
                              &pattern_size, &cur_form, &orig_form, &eff_search,
                              &eff_skip, &eff_flat);
                radial2cartesian(pattern);
                shape(pattern, pattern_size, &azimuth, &elongation, &width);
                prof_map_info();
                prof_sso("computation_parameters");
                prof_dbl("easting", oneoff_easting);
                prof_dbl("northing", oneoff_northing);
                prof_mtr("search_m", eff_search);
                prof_int("search_cells", search_cells);
                prof_mtr("skip_m", eff_skip);
                prof_int("skip_cells", skip_cells);
                prof_dbl("flat_thresh_deg", RAD2DEGREE(flat_threshold));
                prof_mtr("flat_distance_m", eff_flat);
                prof_mtr("flat_height_m", flat_threshold_height);
                prof_bln("extended_correction", extended);
                prof_eso(); /* computation_parameters */
                prof_sso("intermediate_data");
                if (extended) {
                    prof_int("initial_landform_cat", orig_form);
                    prof_str("initial_landform_code",
                             form_short_name(orig_form));
                    prof_str("initial_landform_name",
                             form_long_name(orig_form));
                }
                prof_int("ternary_498", determine_ternary(pattern->pattern));
                prof_int("ternary_6561", preliminary_ternary(pattern->pattern));
                prof_int("pattern_size", pattern_size);
                prof_dbl("origin_easting",
                         Rast_col_to_easting(oneoff_col + 0.5, &window));
                prof_dbl("origin_northing",
                         Rast_row_to_northing(oneoff_row + 0.5, &window));
                prof_pattern(rows[cur_row][oneoff_col], pattern);
                prof_eso(); /* intermediate_data */
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
                prof_mtr("exposition_m", exposition(pattern->elevation));
                prof_mtr("range_m", range(pattern->elevation));
                prof_dbl("variance",
                         variance(pattern->elevation, pattern_size));
                prof_dbl("extends", extends(pattern) / area_of_octagon);
                prof_mtr("octagon_perimeter_m", octa_perimeter(pattern));
                prof_mtr("octagon_area_m2", extends(pattern));
                prof_mtr("mesh_perimeter_m", mesh_perimeter(pattern));
                prof_mtr("mesh_area_m2", mesh_area(pattern));
                prof_eso(); /* final_results */
                /*
                 * When adding new data items, increment the minor version.
                 * When deleting, moving, renaming or otherwise changing
                 * existing data, increment the major version and reset the
                 * minor version.
                 */
                prof_int("format_version_major", 0);
                prof_int("format_version_minor", 9);
                prof_utc("timestamp", time(NULL));
                snprintf(buf, sizeof(buf), "r.geomorphon GRASS %s [%s]",
                         GRASS_VERSION_STRING, GRASS_HEADERS_VERSION);
                prof_str("generator", buf);
                oneoff_done =
                    prof_write(profile_file, par_profileformat->answer);
                if (oneoff_done)
                    G_verbose_message(_("Profile data has been written"));
                else
                    G_important_message(_("Failed writing profile data"));
            }
            G_free(tmp_buf);
        }
        else {
            /* Parallel raster path: per-thread descriptors and scratch, one
             * shared strip loaded in disjoint chunks, then computed. */
            int *fd_thread = G_malloc(sizeof(int) * nprocs);
            void **tmp_thread = G_malloc(sizeof(void *) * nprocs);
            int computed = 0;
            int band_start, t;

            for (t = 0; t < nprocs; ++t) {
                fd_thread[t] = Rast_open_old(elevation.elevname, "");
                tmp_thread[t] = Rast_allocate_buf(elevation.raster_type);
            }
            for (i = o_forms; i < o_size; ++i)
                if (opt_output[i]->answer) {
                    rasters[i].fd = Rast_open_new(opt_output[i]->answer,
                                                  rasters[i].out_data_type);
                    rasters[i].buffer =
                        G_malloc((size_t)brows * ncols *
                                 Rast_cell_size(rasters[i].out_data_type));
                }

            for (band_start = 0; band_start < nrows; band_start += brows) {
                int band_end = band_start + brows;
                int band_count, strip_lo, strip_hi, strip_rows, k, r;

                if (band_end > nrows)
                    band_end = nrows;
                band_count = band_end - band_start;

                strip_lo = MIN(MAX(band_start - row_radius_size, 0),
                               nrows - row_buffer_size - 1);
                strip_hi = MIN(MAX(band_end - 1 - row_radius_size, 0),
                               nrows - row_buffer_size - 1) +
                           row_buffer_size;
                if (strip_hi > nrows - 1)
                    strip_hi = nrows - 1;
                strip_rows = strip_hi - strip_lo + 1;
                for (k = 0; k < strip_rows; ++k)
                    strip_ptr[k] = strip_block + (size_t)k * ncols;

#pragma omp parallel if (nprocs > 1)
                {
                    int tid = 0, nth = 1;
                    int ls, le, rs, re, row;
#if defined(_OPENMP)
                    tid = omp_get_thread_num();
                    nth = omp_get_num_threads();
#endif
                    /* Each thread loads a disjoint chunk of the strip rows
                     * through its own descriptor and scratch. */
                    ls = strip_rows * tid / nth;
                    le = strip_rows * (tid + 1) / nth;
                    if (le > ls)
                        load_strip(fd_thread[tid], elevation.raster_type,
                                   tmp_thread[tid], &strip_ptr[ls],
                                   strip_lo + ls, le - ls);

                    /* Every thread's strip writes must be visible to all before
                     * any thread reads the shared strip for computation. */
#pragma omp barrier

                    /* Split this band's output rows across threads. */
                    rs = band_start + band_count * tid / nth;
                    re = band_start + band_count * (tid + 1) / nth;
                    for (row = rs; row < re; ++row) {
                        int win_top = MIN(MAX(row - row_radius_size, 0),
                                          nrows - row_buffer_size - 1);
                        int cur_row = row - win_top;
                        FCELL **rows = &strip_ptr[win_top - strip_lo];
                        size_t boff = (size_t)(row - band_start) * ncols;
                        int col;

                        if (tid == 0) {
                            int done;
#pragma omp atomic read
                            done = computed;
                            G_percent(done, nrows, 2);
                        }

                        for (col = 0; col < ncols; ++col) {
                            PATTERN patterns[4];
                            PATTERN *pattern;
                            void *pointer_buf;
                            int pattern_size;

                            /* on borders forms ussualy are innatural. */
                            if (row < (skip_cells + 1) ||
                                row > nrows - (skip_cells + 2) ||
                                col < (skip_cells + 1) ||
                                col > ncols - (skip_cells + 2) ||
                                Rast_is_f_null_value(&rows[cur_row][col])) {
                                /* set outputs to NULL on null source or border
                                 */
                                int m;

                                for (m = o_forms; m < o_size; ++m)
                                    if (opt_output[m]->answer) {
                                        pointer_buf = rasters[m].buffer;
                                        switch (rasters[m].out_data_type) {
                                        case CELL_TYPE:
                                            Rast_set_c_null_value(
                                                &((CELL *)
                                                      pointer_buf)[boff + col],
                                                1);
                                            break;
                                        case FCELL_TYPE:
                                            Rast_set_f_null_value(
                                                &((FCELL *)
                                                      pointer_buf)[boff + col],
                                                1);
                                            break;
                                        case DCELL_TYPE:
                                            Rast_set_d_null_value(
                                                &((DCELL *)
                                                      pointer_buf)[boff + col],
                                                1);
                                            break;
                                        default:
                                            G_fatal_error(
                                                _("Unknown output data type"));
                                        }
                                    }
                                continue;
                            } /* end null value */
                            {
                                FORMS cur_form, orig_form;
                                double eff_search, eff_skip, eff_flat;

                                compute_forms(
                                    rows, cur_row, row, col, search_dist,
                                    skip_dist, flat_dist, extended,
                                    max_resolution, 0, patterns, &pattern,
                                    &pattern_size, &cur_form, &orig_form,
                                    &eff_search, &eff_skip, &eff_flat);
                                pattern = &patterns[0];
                                if (opt_output[o_forms]->answer)
                                    ((CELL *)rasters[o_forms]
                                         .buffer)[boff + col] = cur_form;
                                if (opt_output[o_ternary]->answer)
                                    ((CELL *)rasters[o_ternary]
                                         .buffer)[boff + col] =
                                        determine_ternary(pattern->pattern);
                                if (opt_output[o_positive]->answer)
                                    ((CELL *)rasters[o_positive]
                                         .buffer)[boff + col] =
                                        rotate(pattern->positives);
                                if (opt_output[o_negative]->answer)
                                    ((CELL *)rasters[o_negative]
                                         .buffer)[boff + col] =
                                        rotate(pattern->negatives);
                                if (opt_output[o_intensity]->answer)
                                    ((FCELL *)rasters[o_intensity]
                                         .buffer)[boff + col] =
                                        intensity(pattern->elevation,
                                                  pattern_size);
                                if (opt_output[o_exposition]->answer)
                                    ((FCELL *)rasters[o_exposition]
                                         .buffer)[boff + col] =
                                        exposition(pattern->elevation);
                                if (opt_output[o_range]->answer)
                                    ((FCELL *)rasters[o_range]
                                         .buffer)[boff + col] =
                                        range(pattern->elevation);
                                if (opt_output[o_variance]->answer)
                                    ((FCELL *)rasters[o_variance]
                                         .buffer)[boff + col] =
                                        variance(pattern->elevation,
                                                 pattern_size);
                                if (opt_output[o_elongation]->answer ||
                                    opt_output[o_azimuth]->answer ||
                                    opt_output[o_extend]->answer ||
                                    opt_output[o_width]->answer) {
                                    float azimuth, elongation, width;

                                    radial2cartesian(pattern);
                                    shape(pattern, pattern_size, &azimuth,
                                          &elongation, &width);
                                    if (opt_output[o_azimuth]->answer)
                                        ((FCELL *)rasters[o_azimuth]
                                             .buffer)[boff + col] = azimuth;
                                    if (opt_output[o_elongation]->answer)
                                        ((FCELL *)rasters[o_elongation]
                                             .buffer)[boff + col] = elongation;
                                    if (opt_output[o_width]->answer)
                                        ((FCELL *)rasters[o_width]
                                             .buffer)[boff + col] = width;
                                }
                                if (opt_output[o_extend]->answer)
                                    ((FCELL *)rasters[o_extend]
                                         .buffer)[boff + col] =
                                        extends(pattern) / area_of_octagon;
                            }
                        } /* end for col */
#pragma omp atomic update
                        computed++;
                    } /* end for row */
                } /* end parallel */

                /* serial in-order drain of this band's rows, per map */
                for (i = o_forms; i < o_size; ++i)
                    if (opt_output[i]->answer) {
                        size_t rowsz = (size_t)ncols *
                                       Rast_cell_size(rasters[i].out_data_type);

                        for (r = 0; r < band_count; ++r)
                            Rast_put_row(rasters[i].fd,
                                         (char *)rasters[i].buffer +
                                             (size_t)r * rowsz,
                                         rasters[i].out_data_type);
                    }
            } /* end band loop */
            G_percent(nrows, nrows, 2);

            for (t = 0; t < nprocs; ++t) {
                Rast_close(fd_thread[t]);
                G_free(tmp_thread[t]);
            }
            G_free(fd_thread);
            G_free(tmp_thread);
            for (i = o_forms; i < o_size; ++i)
                if (opt_output[i]->answer) {
                    G_free(rasters[i].buffer);
                    Rast_close(rasters[i].fd);
                    Rast_short_history(opt_output[i]->answer, "raster",
                                       &history);
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
        }

        G_free(strip_block);
        G_free(strip_ptr);
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
                G_fatal_error(_("Failed to profile the computation, please "
                                "check the parameters"));
                exit(EXIT_FAILURE);
            }
        }

        exit(EXIT_SUCCESS);
    }
}
