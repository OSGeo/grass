/****************************************************************************
 *
 * MODULE:       r.in.Lidar
 *
 * AUTHOR(S):    Markus Metz
 *               Vaclav Petras (base raster addition and refactoring)
 *               Based on r.in.xyz by Hamish Bowman, Volker Wichmann
 *
 * PURPOSE:      Imports LAS LiDAR point clouds to a raster map using
 *               aggregate statistics.
 *
 * COPYRIGHT:    (C) 2011-2015 Markus Metz and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/segment.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include <liblas/capi/liblas.h>

#include "local_proto.h"
#include "rast_segment.h"
#include "point_binning.h"
#include "filters.h"

int main(int argc, char *argv[])
{
    int out_fd, base_raster;
    char *infile, *outmap;
    int percent;
    double zrange_min, zrange_max, d_tmp;
    double irange_min, irange_max;
    unsigned long estimated_lines;

    RASTER_MAP_TYPE rtype, base_raster_data_type;
    struct History history;
    char title[64];
    SEGMENT base_segment;
    struct PointBinning point_binning;
    void *base_array;
    void *raster_row;
    struct Cell_head region = {0};
    struct Cell_head input_region = {0};
    int rows, last_rows, row0, cols; /* scan box size */
    int row;                         /* counters */

    int pass, npasses;
    unsigned long line, line_total;
    unsigned int counter;
    unsigned long n_invalid;
    char buff[BUFFSIZE];
    double x, y, z;
    double intensity;
    int arr_row, arr_col;
    unsigned long count, count_total;
    int point_class;

    double zscale = 1.0;
    double iscale = 1.0;
    double res = 0.0;

    struct BinIndex bin_index_nodes;

    bin_index_nodes.num_nodes = 0;
    bin_index_nodes.max_nodes = 0;
    bin_index_nodes.nodes = 0;

    struct GModule *module;
    struct Option *input_opt, *output_opt, *percent_opt, *type_opt, *filter_opt,
        *class_opt;
    struct Option *method_opt, *base_raster_opt;
    struct Option *zrange_opt, *zscale_opt;
    struct Option *irange_opt, *iscale_opt;
    struct Option *trim_opt, *pth_opt, *res_opt;
    struct Option *file_list_opt;
    struct Flag *print_flag, *scan_flag, *shell_style, *over_flag,
        *extents_flag;
    struct Flag *intens_flag, *intens_import_flag;
    struct Flag *set_region_flag;
    struct Flag *base_rast_res_flag;
    struct Flag *only_valid_flag;

    /* LAS */
    LASReaderH LAS_reader;
    LASHeaderH LAS_header;
    LASSRSH LAS_srs;
    LASPointH LAS_point;
    int return_filter;

    const char *projstr;
    struct Cell_head cellhd = {0}, loc_wind = {0};

    unsigned int n_filtered;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("import"));
    G_add_keyword(_("LIDAR"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("conversion"));
    G_add_keyword(_("aggregation"));
    G_add_keyword(_("binning"));
    module->description = _("Creates a raster map from LAS LiDAR points using "
                            "univariate statistics.");

    input_opt = G_define_standard_option(G_OPT_F_BIN_INPUT);
    input_opt->required = NO;
    input_opt->label = _("LAS input file");
    input_opt->description =
        _("LiDAR input files in LAS format (*.las or *.laz)");
    input_opt->guisection = _("Input");

    output_opt = G_define_standard_option(G_OPT_R_OUTPUT);
    output_opt->required = NO;
    output_opt->guisection = _("Output");

    file_list_opt = G_define_standard_option(G_OPT_F_INPUT);
    file_list_opt->key = "file";
    file_list_opt->label = _("File containing names of LAS input files");
    file_list_opt->description =
        _("LiDAR input files in LAS format (*.las or *.laz)");
    file_list_opt->required = NO;
    file_list_opt->guisection = _("Input");

    method_opt = G_define_option();
    method_opt->key = "method";
    method_opt->type = TYPE_STRING;
    method_opt->required = NO;
    method_opt->description = _("Statistic to use for raster values");
    method_opt->options = "n,min,max,range,sum,mean,stddev,variance,coeff_var,"
                          "median,percentile,skewness,trimmean";
    method_opt->answer = "mean";
    method_opt->guisection = _("Statistic");
    G_asprintf((char **)&(method_opt->descriptions),
               "n;%s;"
               "min;%s;"
               "max;%s;"
               "range;%s;"
               "sum;%s;"
               "mean;%s;"
               "stddev;%s;"
               "variance;%s;"
               "coeff_var;%s;"
               "median;%s;"
               "percentile;%s;"
               "skewness;%s;"
               "trimmean;%s",
               _("Number of points in cell"),
               _("Minimum value of point values in cell"),
               _("Maximum value of point values in cell"),
               _("Range of point values in cell"),
               _("Sum of point values in cell"),
               _("Mean (average) value of point values in cell"),
               _("Standard deviation of point values in cell"),
               _("Variance of point values in cell"),
               _("Coefficient of variance of point values in cell"),
               _("Median value of point values in cell"),
               _("pth (nth) percentile of point values in cell"),
               _("Skewness of point values in cell"),
               _("Trimmed mean of point values in cell"));

    type_opt = G_define_standard_option(G_OPT_R_TYPE);
    type_opt->required = NO;
    type_opt->answer = "FCELL";

    base_raster_opt = G_define_standard_option(G_OPT_R_INPUT);
    base_raster_opt->key = "base_raster";
    base_raster_opt->required = NO;
    base_raster_opt->label = _("Subtract raster values from the Z coordinates");
    base_raster_opt->description =
        _("The scale for Z is applied beforehand, the range filter for"
          " Z afterwards");
    base_raster_opt->guisection = _("Transform");

    zrange_opt = G_define_option();
    zrange_opt->key = "zrange";
    zrange_opt->type = TYPE_DOUBLE;
    zrange_opt->required = NO;
    zrange_opt->key_desc = "min,max";
    zrange_opt->label = _("Filter range for Z data (min,max)");
    zrange_opt->description =
        _("Applied after base_raster transformation step");
    zrange_opt->guisection = _("Selection");

    zscale_opt = G_define_option();
    zscale_opt->key = "zscale";
    zscale_opt->type = TYPE_DOUBLE;
    zscale_opt->required = NO;
    zscale_opt->answer = "1.0";
    zscale_opt->description = _("Scale to apply to Z data");
    zscale_opt->guisection = _("Transform");

    irange_opt = G_define_option();
    irange_opt->key = "intensity_range";
    irange_opt->type = TYPE_DOUBLE;
    irange_opt->required = NO;
    irange_opt->key_desc = "min,max";
    irange_opt->description = _("Filter range for intensity values (min,max)");
    irange_opt->guisection = _("Selection");

    iscale_opt = G_define_option();
    iscale_opt->key = "intensity_scale";
    iscale_opt->type = TYPE_DOUBLE;
    iscale_opt->required = NO;
    iscale_opt->answer = "1.0";
    iscale_opt->description = _("Scale to apply to intensity values");
    iscale_opt->guisection = _("Transform");

    percent_opt = G_define_option();
    percent_opt->key = "percent";
    percent_opt->type = TYPE_INTEGER;
    percent_opt->required = NO;
    percent_opt->answer = "100";
    percent_opt->options = "1-100";
    percent_opt->description = _("Percent of map to keep in memory");

    /* I would prefer to call the following "percentile", but that has too
     * much namespace overlap with the "percent" option above */
    pth_opt = G_define_option();
    pth_opt->key = "pth";
    pth_opt->type = TYPE_INTEGER;
    pth_opt->required = NO;
    pth_opt->options = "1-100";
    pth_opt->description = _("pth percentile of the values");
    pth_opt->guisection = _("Statistic");

    trim_opt = G_define_option();
    trim_opt->key = "trim";
    trim_opt->type = TYPE_DOUBLE;
    trim_opt->required = NO;
    trim_opt->options = "0-50";
    trim_opt->label =
        _("Discard given percentage of the smallest and largest values");
    trim_opt->description = _("Discard <trim> percent of the smallest and "
                              "<trim> percent of the largest observations");
    trim_opt->guisection = _("Statistic");

    res_opt = G_define_option();
    res_opt->key = "resolution";
    res_opt->type = TYPE_DOUBLE;
    res_opt->required = NO;
    res_opt->description = _("Output raster resolution");
    res_opt->guisection = _("Output");

    filter_opt = G_define_option();
    filter_opt->key = "return_filter";
    filter_opt->type = TYPE_STRING;
    filter_opt->required = NO;
    filter_opt->label = _("Only import points of selected return type");
    filter_opt->description = _("If not specified, all points are imported");
    filter_opt->options = "first,last,mid";
    filter_opt->guisection = _("Selection");

    class_opt = G_define_option();
    class_opt->key = "class_filter";
    class_opt->type = TYPE_INTEGER;
    class_opt->multiple = YES;
    class_opt->required = NO;
    class_opt->label = _("Only import points of selected class(es)");
    class_opt->description = _("Input is comma separated integers. "
                               "If not specified, all points are imported.");
    class_opt->guisection = _("Selection");

    print_flag = G_define_flag();
    print_flag->key = 'p';
    print_flag->description = _("Print LAS file info and exit");

    extents_flag = G_define_flag();
    extents_flag->key = 'e';
    extents_flag->label =
        _("Use the extent of the input for the raster extent");
    extents_flag->description =
        _("Set internally computational region extents based on the"
          " point cloud");
    extents_flag->guisection = _("Output");

    set_region_flag = G_define_flag();
    set_region_flag->key = 'n';
    set_region_flag->label =
        _("Set computation region to match the new raster map");
    set_region_flag->description =
        _("Set computation region to match the 2D extent and resolution"
          " of the newly created new raster map");
    set_region_flag->guisection = _("Output");

    over_flag = G_define_flag();
    over_flag->key = 'o';
    over_flag->label =
        _("Override projection check (use current project's CRS)");
    over_flag->description =
        _("Assume that the dataset has the same coordinate reference system as "
          "the current project");

    scan_flag = G_define_flag();
    scan_flag->key = 's';
    scan_flag->description = _("Scan data file for extent then exit");

    shell_style = G_define_flag();
    shell_style->key = 'g';
    shell_style->description =
        _("In scan mode, print using shell script style");

    intens_flag = G_define_flag();
    intens_flag->key = 'i';
    intens_flag->label = _("Use intensity values rather than Z values");
    intens_flag->description =
        _("Uses intensity values everywhere as if they would be Z"
          " coordinates");

    intens_import_flag = G_define_flag();
    intens_import_flag->key = 'j';
    intens_import_flag->description =
        _("Use Z values for filtering, but intensity values for statistics");

    base_rast_res_flag = G_define_flag();
    base_rast_res_flag->key = 'd';
    base_rast_res_flag->label =
        _("Use base raster resolution instead of computational region");
    base_rast_res_flag->description =
        _("For getting values from base raster, use its actual"
          " resolution instead of computational region resolution");
    base_rast_res_flag->guisection = _("Transform");

    only_valid_flag = G_define_flag();
    only_valid_flag->key = 'v';
    only_valid_flag->label = _("Use only valid points");
    only_valid_flag->description =
        _("Points invalid according to APSRS LAS specification will be"
          " filtered out");
    only_valid_flag->guisection = _("Selection");

    G_option_required(input_opt, file_list_opt, NULL);
    G_option_exclusive(input_opt, file_list_opt, NULL);
    G_option_required(output_opt, print_flag, scan_flag, shell_style, NULL);
    G_option_exclusive(intens_flag, intens_import_flag, NULL);
    G_option_requires(base_rast_res_flag, base_raster_opt, NULL);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    int only_valid = FALSE;

    n_invalid = 0;
    if (only_valid_flag->answer)
        only_valid = TRUE;

    /* we could use rules but this gives more info and allows continuing */
    if (set_region_flag->answer && !(extents_flag->answer || res_opt->answer)) {
        G_warning(_("Flag %c makes sense only with %s option or -%c flag"),
                  set_region_flag->key, res_opt->key, extents_flag->key);
        /* avoid the call later on */
        set_region_flag->answer = '\0';
    }

    /* Trim option is used only for trimmean method */
    if (trim_opt->answer != NULL &&
        strcmp(method_opt->answer, "trimmean") != 0) {
        G_fatal_error(_("Trim option can be used only with trimmean method"));
    }

    struct StringList infiles;

    if (file_list_opt->answer) {
        if (access(file_list_opt->answer, F_OK) != 0)
            G_fatal_error(_("File <%s> does not exist"), file_list_opt->answer);
        string_list_from_file(&infiles, file_list_opt->answer);
    }
    else {
        string_list_from_one_item(&infiles, input_opt->answer);
    }

    /* parse input values */
    outmap = output_opt->answer;

    if (shell_style->answer && !scan_flag->answer) {
        scan_flag->answer =
            1; /* pointer not int, so set = shell_style->answer ? */
    }

    /* check zrange and extent relation */
    if (scan_flag->answer || extents_flag->answer) {
        if (zrange_opt->answer)
            G_warning(_("zrange will not be taken into account during scan"));
    }

    Rast_get_window(&region);
    /* G_get_window seems to be unreliable if the location has been changed */
    G_get_set_window(
        &loc_wind); /* TODO: v.in.lidar uses G_get_default_window() */

    estimated_lines = 0;
    int i;

    for (i = 0; i < infiles.num_items; i++) {
        infile = infiles.items[i];
        /* don't if file not found */
        if (access(infile, F_OK) != 0)
            G_fatal_error(_("Input file <%s> does not exist"), infile);
        /* Open LAS file */
        LAS_reader = LASReader_Create(infile);
        if (LAS_reader == NULL)
            G_fatal_error(_("Unable to open file <%s> as a LiDAR point cloud"),
                          infile);
        LAS_header = LASReader_GetHeader(LAS_reader);
        if (LAS_header == NULL) {
            G_fatal_error(_("Unable to read LAS header of <%s>"), infile);
        }

        LAS_srs = LASHeader_GetSRS(LAS_header);

        /* print info or check projection if we are actually importing */
        if (print_flag->answer) {
            /* print filename when there is more than one file */
            if (infiles.num_items > 1)
                fprintf(stdout, "File: %s\n", infile);
            /* Print LAS header */
            print_lasinfo(LAS_header, LAS_srs);
        }
        else {
            /* report that we are checking more files */
            if (i == 1)
                G_message(_("First file's projection checked,"
                            " checking projection of the other files..."));
            /* Fetch input map projection in GRASS form. */
            projstr = LASSRS_GetWKT_CompoundOK(LAS_srs);
            /* we are printing the non-warning messages only for first file */
            projection_check_wkt(cellhd, loc_wind, projstr, over_flag->answer,
                                 shell_style->answer || i);
            /* if there is a problem in some other file, first OK message
             * is printed but than a warning, this is not ideal but hopefully
             * not so confusing when importing multiple files */
        }
        if (scan_flag->answer || extents_flag->answer) {
            /* we assign to the first one (i==0) but update for the rest */
            scan_bounds(LAS_reader, shell_style->answer, extents_flag->answer,
                        i, zscale, &region);
        }
        /* number of estimated point across all files */
        /* TODO: this should be ull which won't work with percent report */
        estimated_lines += LASHeader_GetPointRecordsCount(LAS_header);
        /* We are closing all again and we will be opening them later,
         * so we don't have to worry about limit for open files. */
        LASSRS_Destroy(LAS_srs);
        LASHeader_Destroy(LAS_header);
        LASReader_Destroy(LAS_reader);
    }
    /* if we are not importing, end */
    if (print_flag->answer || scan_flag->answer)
        exit(EXIT_SUCCESS);

    return_filter = LAS_ALL;
    if (filter_opt->answer) {
        if (strcmp(filter_opt->answer, "first") == 0)
            return_filter = LAS_FIRST;
        else if (strcmp(filter_opt->answer, "last") == 0)
            return_filter = LAS_LAST;
        else if (strcmp(filter_opt->answer, "mid") == 0)
            return_filter = LAS_MID;
        else
            G_fatal_error(_("Unknown filter option <%s>"), filter_opt->answer);
    }
    struct ReturnFilter return_filter_struct;

    return_filter_struct.filter = return_filter;
    struct ClassFilter class_filter;

    class_filter_create_from_strings(&class_filter, class_opt->answers);

    percent = atoi(percent_opt->answer);
    /* TODO: we already used zscale */
    /* TODO: we don't report intensity range */
    if (zscale_opt->answer)
        zscale = atof(zscale_opt->answer);
    if (iscale_opt->answer)
        iscale = atof(iscale_opt->answer);

    /* parse zrange */
    if (zrange_opt->answer != NULL) {
        if (zrange_opt->answers[0] == NULL)
            G_fatal_error(_("Invalid zrange"));

        sscanf(zrange_opt->answers[0], "%lf", &zrange_min);
        sscanf(zrange_opt->answers[1], "%lf", &zrange_max);

        if (zrange_min > zrange_max) {
            d_tmp = zrange_max;
            zrange_max = zrange_min;
            zrange_min = d_tmp;
        }
    }
    /* parse irange */
    if (irange_opt->answer != NULL) {
        if (irange_opt->answers[0] == NULL)
            G_fatal_error(_("Invalid %s"), irange_opt->key);

        sscanf(irange_opt->answers[0], "%lf", &irange_min);
        sscanf(irange_opt->answers[1], "%lf", &irange_max);

        if (irange_min > irange_max) {
            d_tmp = irange_max;
            irange_max = irange_min;
            irange_min = d_tmp;
        }
    }

    point_binning_set(&point_binning, method_opt->answer, pth_opt->answer,
                      trim_opt->answer, FALSE);

    base_array = NULL;

    if (strcmp("CELL", type_opt->answer) == 0)
        rtype = CELL_TYPE;
    else if (strcmp("DCELL", type_opt->answer) == 0)
        rtype = DCELL_TYPE;
    else
        rtype = FCELL_TYPE;

    if (point_binning.method == METHOD_N)
        rtype = CELL_TYPE;

    if (res_opt->answer) {
        /* align to resolution */
        res = atof(res_opt->answer);

        if (!G_scan_resolution(res_opt->answer, &res, region.proj))
            G_fatal_error(_("Invalid input <%s=%s>"), res_opt->key,
                          res_opt->answer);

        if (res <= 0)
            G_fatal_error(_("Option '%s' must be > 0.0"), res_opt->key);

        region.ns_res = region.ew_res = res;

        region.north = ceil(region.north / res) * res;
        region.south = floor(region.south / res) * res;
        region.east = ceil(region.east / res) * res;
        region.west = floor(region.west / res) * res;

        G_adjust_Cell_head(&region, 0, 0);
    }
    else if (extents_flag->answer) {
        /* align to current region */
        Rast_align_window(&region, &loc_wind);
    }
    Rast_set_output_window(&region);

    rows = last_rows = region.rows;
    npasses = 1;
    if (percent < 100) {
        rows = (int)(region.rows * (percent / 100.0));
        npasses = region.rows / rows;
        last_rows = region.rows - npasses * rows;
        if (last_rows)
            npasses++;
        else
            last_rows = rows;
    }
    cols = region.cols;

    G_debug(2, "region.n=%f  region.s=%f  region.ns_res=%f", region.north,
            region.south, region.ns_res);
    G_debug(2, "region.rows=%d  [box_rows=%d]  region.cols=%d", region.rows,
            rows, region.cols);

    /* using row-based chunks (used for output) when input and output
     * region matches and using segment library when they don't */
    int use_segment = 0;
    int use_base_raster_res = 0;

    /* TODO: see if the input region extent is smaller than the raster
     * if yes, the we need to load the whole base raster if the -e
     * flag was defined (alternatively clip the regions) */
    if (base_rast_res_flag->answer)
        use_base_raster_res = 1;
    if (base_raster_opt->answer &&
        (res_opt->answer || use_base_raster_res || extents_flag->answer))
        use_segment = 1;
    if (base_raster_opt->answer && !use_segment) {
        /* TODO: do we need to test existence first? mapset? */
        base_raster = Rast_open_old(base_raster_opt->answer, "");
        base_raster_data_type = Rast_get_map_type(base_raster);
        base_array = G_calloc((size_t)rows * (cols + 1),
                              Rast_cell_size(base_raster_data_type));
    }
    if (base_raster_opt->answer && use_segment) {
        if (use_base_raster_res) {
            /* read raster actual extent and resolution */
            Rast_get_cellhd(base_raster_opt->answer, "", &input_region);
            /* TODO: make it only as small as the output is or points are */
            Rast_set_input_window(&input_region); /* we have split window */
        }
        else {
            Rast_get_input_window(&input_region);
        }
        rast_segment_open(&base_segment, base_raster_opt->answer,
                          &base_raster_data_type);
    }

    if (!scan_flag->answer) {
        if (!check_rows_cols_fit_to_size_t(rows, cols))
            G_fatal_error(
                _("Unable to process the whole map at once. "
                  "Please set the '%s' option to some value lower than 100."),
                percent_opt->key);
        point_binning_memory_test(&point_binning, rows, cols, rtype);
    }

    /* open output map */
    out_fd = Rast_open_new(outmap, rtype);

    /* allocate memory for a single row of output data */
    raster_row = Rast_allocate_output_buf(rtype);

    G_message(_("Reading data..."));

    count_total = line_total = 0;

    /* main binning loop(s) */
    for (pass = 1; pass <= npasses; pass++) {

        if (npasses > 1)
            G_message(_("Pass #%d (of %d)..."), pass, npasses);

        /* figure out segmentation */
        row0 = (pass - 1) * rows;
        if (pass == npasses) {
            rows = last_rows;
        }

        if (base_array) {
            G_debug(2, "filling base raster array");
            for (row = 0; row < rows; row++) {
                Rast_get_row(base_raster,
                             base_array +
                                 ((size_t)row * cols *
                                  Rast_cell_size(base_raster_data_type)),
                             row, base_raster_data_type);
            }
        }

        G_debug(2, "pass=%d/%d  rows=%d", pass, npasses, rows);

        point_binning_allocate(&point_binning, rows, cols, rtype);

        line = 0;
        count = 0;
        counter = 0;
        G_percent_reset();

        /* loop of input files */
        for (i = 0; i < infiles.num_items; i++) {
            infile = infiles.items[i];
            /* we already know file is there, so just do basic checks */
            LAS_reader = LASReader_Create(infile);
            if (LAS_reader == NULL)
                G_fatal_error(_("Unable to open file <%s>"), infile);

            while ((LAS_point = LASReader_GetNextPoint(LAS_reader)) != NULL) {
                line++;
                counter++;

                if (counter == 100000) { /* speed */
                    if (line < estimated_lines)
                        G_percent(line, estimated_lines, 3);
                    counter = 0;
                }

                /* We always count them and report because behavior
                 * changed in between 7.0 and 7.2 from undefined (but skipping
                 * invalid points) to filtering them out only when requested. */
                if (!LASPoint_IsValid(LAS_point)) {
                    n_invalid++;
                    if (only_valid)
                        continue;
                }

                x = LASPoint_GetX(LAS_point);
                y = LASPoint_GetY(LAS_point);
                if (intens_flag->answer)
                    /* use intensity as z here to allow all filters (and
                     * modifications) below to be applied for intensity */
                    z = LASPoint_GetIntensity(LAS_point);
                else
                    z = LASPoint_GetZ(LAS_point);

                int return_n = LASPoint_GetReturnNumber(LAS_point);
                int n_returns = LASPoint_GetNumberOfReturns(LAS_point);

                if (return_filter_is_out(&return_filter_struct, return_n,
                                         n_returns)) {
                    n_filtered++;
                    continue;
                }
                point_class = (int)LASPoint_GetClassification(LAS_point);
                if (class_filter_is_out(&class_filter, point_class))
                    continue;

                if (y <= region.south || y > region.north) {
                    continue;
                }
                if (x < region.west || x >= region.east) {
                    continue;
                }

                /* find the bin in the current array box */
                arr_row = (int)((region.north - y) / region.ns_res) - row0;
                if (arr_row < 0 || arr_row >= rows)
                    continue;
                arr_col = (int)((x - region.west) / region.ew_res);

                z = z * zscale;

                if (base_array) {
                    double base_z;

                    if (row_array_get_value_row_col(
                            base_array, arr_row, arr_col, cols,
                            base_raster_data_type, &base_z))
                        z -= base_z;
                    else
                        continue;
                }
                else if (use_segment) {
                    double base_z;

                    if (rast_segment_get_value_xy(&base_segment, &input_region,
                                                  base_raster_data_type, x, y,
                                                  &base_z))
                        z -= base_z;
                    else
                        continue;
                }

                if (zrange_opt->answer) {
                    if (z < zrange_min || z > zrange_max) {
                        continue;
                    }
                }

                if (intens_import_flag->answer || irange_opt->answer) {
                    intensity = LASPoint_GetIntensity(LAS_point);
                    intensity *= iscale;
                    if (irange_opt->answer) {
                        if (intensity < irange_min || intensity > irange_max) {
                            continue;
                        }
                    }
                    /* use intensity for statistics */
                    if (intens_import_flag->answer)
                        z = intensity;
                }

                count++;
                /*          G_debug(5, "x: %f, y: %f, z: %f", x, y, z); */

                update_value(&point_binning, &bin_index_nodes, cols, arr_row,
                             arr_col, rtype, x, y, z);
            } /* while !EOF of one input file */
            /* close input LAS file */
            LASReader_Destroy(LAS_reader);
        } /* end of loop for all input files files */

        G_percent(1, 1, 1); /* flush */
        G_debug(2, "pass %d finished, %lu coordinates in box", pass, count);
        count_total += count;
        line_total += line;

        /* calc stats and output */
        G_message(_("Writing output raster map..."));
        for (row = 0; row < rows; row++) {
            /* potentially vector writing can be independent on the binning */
            write_values(&point_binning, &bin_index_nodes, raster_row, row,
                         cols, rtype, NULL);

            G_percent(row, rows, 10);

            /* write out line of raster data */
            Rast_put_row(out_fd, raster_row, rtype);
        }

        /* free memory */
        point_binning_free(&point_binning, &bin_index_nodes);
    } /* passes loop */
    if (base_array)
        Rast_close(base_raster);
    if (use_segment)
        Segment_close(&base_segment);

    G_percent(1, 1, 1); /* flush */
    G_free(raster_row);

    G_message(_("%lu points found in input file(s)"), line_total);

    /* close raster file & write history */
    Rast_close(out_fd);

    sprintf(title, "Raw X,Y,Z data binned into a raster grid by cell %s",
            method_opt->answer);
    Rast_put_cell_title(outmap, title);

    Rast_short_history(outmap, "raster", &history);
    Rast_command_history(&history);
    Rast_set_history(&history, HIST_DATSRC_1, infile);
    Rast_write_history(outmap, &history);

    /* set computation region to the new raster map */
    /* TODO: should be in the done message */
    if (set_region_flag->answer)
        G_put_window(&region);

    if (n_invalid && only_valid)
        G_message(_("%lu input points were invalid and filtered out"),
                  n_invalid);
    if (n_invalid && !only_valid)
        G_message(_("%lu input points were invalid, use -%c flag to filter"
                    " them out"),
                  n_invalid, only_valid_flag->key);
    if (infiles.num_items > 1) {
        sprintf(buff,
                _("Raster map <%s> created."
                  " %lu points from %d files found in region."),
                outmap, count_total, infiles.num_items);
    }
    else {
        sprintf(buff,
                _("Raster map <%s> created."
                  " %lu points found in region."),
                outmap, count_total);
    }

    G_done_msg("%s", buff);
    G_debug(1, "Processed %lu points.", line_total);

    string_list_free(&infiles);

    exit(EXIT_SUCCESS);
}
