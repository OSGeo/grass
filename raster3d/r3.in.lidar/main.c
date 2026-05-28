/****************************************************************************
 *
 * MODULE:       r3.in.Lidar
 *
 * AUTHOR(S):    Vaclav Petras
 *
 * PURPOSE:      Imports LAS LiDAR point clouds to a 3D raster map using
 *               aggregate statistics.
 *
 * COPYRIGHT:    (C) 2016 Vaclav Petras and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>
#include <liblas/capi/liblas.h>

#include "info.h"
#include "string_list.h"
#include "projection.h"
#include "rast_segment.h"
#include "filters.h"

struct PointBinning3D {
    RASTER3D_Region region, flat_region;
    RASTER3D_Map *count_raster, *sum_raster, *mean_raster;
    RASTER3D_Map *count_flat_raster, *sum_flat_raster;
    RASTER3D_Map *prop_count_raster, *prop_sum_raster;
};

/* TODO: do this in some more efficient way, perhaps function in the lib */
static void raster3d_set_value_float(RASTER3D_Map *raster,
                                     RASTER3D_Region *region, float value)
{
    int col, row, depth;

    for (depth = 0; depth < region->depths; depth++)
        for (row = 0; row < region->rows; row++)
            for (col = 0; col < region->cols; col++)
                Rast3d_put_float(raster, col, row, depth, value);
}

/* c = a / b */
static void raster3d_divide(RASTER3D_Map *a, RASTER3D_Map *b, RASTER3D_Map *c,
                            RASTER3D_Region *region)
{
    int col, row, depth;
    double tmp;

    G_percent_reset();
    for (depth = 0; depth < region->depths; depth++) {
        G_percent(depth, region->depths, 5);
        for (row = 0; row < region->rows; row++) {
            for (col = 0; col < region->cols; col++) {
                tmp = Rast3d_get_double(b, col, row, 0);
                /* TODO: compare to epsilon */
                if (tmp > 0) {
                    tmp = Rast3d_get_double(a, col, row, depth) / tmp;
                    Rast3d_put_double(c, col, row, depth, tmp);
                }
                else {
                    /* TODO: check this implementation */
                    Rast3d_set_null_value(&tmp, 1, DCELL_TYPE);
                    Rast3d_put_double(c, col, row, depth, tmp);
                }
            }
        }
    }
    G_percent(1, 1, 1); /* flush */
}

/* c = a / b where b has depth 1 */
static void raster3d_divide_by_flat(RASTER3D_Map *a, RASTER3D_Map *b,
                                    RASTER3D_Map *c, RASTER3D_Region *region)
{
    int col, row, depth;
    double tmp;

    G_percent_reset();
    for (depth = 0; depth < region->depths; depth++) {
        G_percent(depth, region->depths, 5);
        for (row = 0; row < region->rows; row++) {
            for (col = 0; col < region->cols; col++) {
                tmp = Rast3d_get_double(b, col, row, 0);
                /* since it is count, using cast to integer to check
                   against zero, limits the value to max of CELL */
                if (((CELL)tmp) > 0) {
                    tmp = Rast3d_get_double(a, col, row, depth) / tmp;
                    Rast3d_put_double(c, col, row, depth, tmp);
                }
                else {
                    /* TODO: check this implementation */
                    Rast3d_set_null_value(&tmp, 1, DCELL_TYPE);
                    Rast3d_put_double(c, col, row, depth, tmp);
                }
            }
        }
    }
    G_percent(1, 1, 1); /* flush */
}

/* initialize raster pointers */
void binning_init(struct PointBinning3D *binning)
{
    binning->count_raster = binning->sum_raster = binning->mean_raster = NULL;
    binning->count_flat_raster = binning->sum_flat_raster = NULL;
    binning->prop_count_raster = binning->prop_sum_raster = NULL;
}

void binning_add_point(struct PointBinning3D *binning, int row, int col,
                       int depth, double value)
{
    double tmp; /* TODO: check if these should be in binning struct */

    if (binning->count_raster) {
        tmp = Rast3d_get_double(binning->count_raster, col, row, depth);
        Rast3d_put_double(binning->count_raster, col, row, depth, tmp + 1);
    }
    if (binning->count_flat_raster) {
        tmp = Rast3d_get_double(binning->count_flat_raster, col, row, 0);
        Rast3d_put_double(binning->count_flat_raster, col, row, 0, tmp + 1);
    }
    if (binning->sum_raster) {
        tmp = Rast3d_get_double(binning->sum_raster, col, row, depth);
        Rast3d_put_double(binning->sum_raster, col, row, depth, tmp + value);
    }
    if (binning->sum_flat_raster) {
        tmp = Rast3d_get_double(binning->sum_flat_raster, col, row, 0);
        Rast3d_put_double(binning->sum_flat_raster, col, row, 0, tmp + value);
    }
}

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *input_opt;
    struct Option *file_list_opt;
    struct Option *count_output_opt, *sum_output_opt, *mean_output_opt;
    struct Option *prop_count_output_opt, *prop_sum_output_opt;
    struct Option *filter_opt, *class_opt;
    struct Option *zscale_opt;
    struct Option *iscale_opt;
    struct Option *irange_opt;
    struct Option *base_raster_opt;
    struct Flag *base_rast_res_flag;
    struct Flag *only_valid_flag;
    struct Flag *print_flag, *scan_flag, *shell_style;
    struct Flag *over_flag;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("import"));
    G_add_keyword(_("LIDAR"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("conversion"));
    G_add_keyword(_("aggregation"));
    G_add_keyword(_("binning"));
    module->description = _("Creates a 3D raster map from LAS LiDAR points "
                            "using univariate statistics.");

    input_opt = G_define_standard_option(G_OPT_F_BIN_INPUT);
    input_opt->required = NO;
    input_opt->label = _("LAS input file");
    input_opt->description =
        _("LiDAR input file in LAS format (*.las or *.laz)");
    input_opt->guisection = _("Input");

    file_list_opt = G_define_standard_option(G_OPT_F_INPUT);
    file_list_opt->key = "file";
    file_list_opt->label = _("File containing names of LAS input files");
    file_list_opt->description =
        _("LiDAR input files in LAS format (*.las or *.laz)");
    file_list_opt->required = NO;
    file_list_opt->guisection = _("Input");

    count_output_opt = G_define_standard_option(G_OPT_R3_OUTPUT);
    count_output_opt->key = "n";
    count_output_opt->required = NO;
    count_output_opt->label = _("Count of points per cell");
    count_output_opt->guisection = _("Output");

    sum_output_opt = G_define_standard_option(G_OPT_R3_OUTPUT);
    sum_output_opt->key = "sum";
    sum_output_opt->required = NO;
    sum_output_opt->label = _("Sum of values of point intensities per cell");
    sum_output_opt->guisection = _("Output");

    mean_output_opt = G_define_standard_option(G_OPT_R3_OUTPUT);
    mean_output_opt->key = "mean";
    mean_output_opt->required = NO;
    mean_output_opt->label = _("Mean of point intensities per cell");
    mean_output_opt->guisection = _("Output");

    /* TODO: proportional versus relative in naming */
    prop_count_output_opt = G_define_standard_option(G_OPT_R3_OUTPUT);
    prop_count_output_opt->key = "proportional_n";
    prop_count_output_opt->required = NO;
    prop_count_output_opt->label =
        _("3D raster map of proportional point count");
    prop_count_output_opt->description =
        _("Point count per 3D cell divided by point count per vertical"
          " column");
    prop_count_output_opt->guisection = _("Proportional output");

    prop_sum_output_opt = G_define_standard_option(G_OPT_R3_OUTPUT);
    prop_sum_output_opt->key = "proportional_sum";
    prop_sum_output_opt->required = NO;
    prop_sum_output_opt->label =
        _("3D raster map of proportional sum of values");
    prop_sum_output_opt->description =
        _("Sum of values per 3D cell divided by sum of values per"
          " vertical column");
    prop_sum_output_opt->guisection = _("Proportional output");

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

    base_raster_opt = G_define_standard_option(G_OPT_R_INPUT);
    base_raster_opt->key = "base_raster";
    base_raster_opt->required = NO;
    base_raster_opt->label = _("Subtract raster values from the z coordinates");
    base_raster_opt->description =
        _("The scale for z is applied beforehand, the filter afterwards");
    base_raster_opt->guisection = _("Transform");

    base_rast_res_flag = G_define_flag();
    base_rast_res_flag->key = 'd';
    base_rast_res_flag->description =
        _("Use base raster actual resolution instead of computational region");

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

    only_valid_flag = G_define_flag();
    only_valid_flag->key = 'v';
    only_valid_flag->label = _("Use only valid points");
    only_valid_flag->description =
        _("Points invalid according to APSRS LAS specification will be"
          " filtered out");
    only_valid_flag->guisection = _("Selection");

    over_flag = G_define_flag();
    over_flag->key = 'o';
    over_flag->label =
        _("Override projection check (use current projects's CRS)");
    over_flag->description =
        _("Assume that the dataset has the same coordinate "
          "reference system as the current project");

    print_flag = G_define_flag();
    print_flag->key = 'p';
    print_flag->description = _("Print LAS file info and exit");

    scan_flag = G_define_flag();
    scan_flag->key = 's';
    scan_flag->description = _("Scan data file for extent then exit");

    shell_style = G_define_flag();
    shell_style->key = 'g';
    shell_style->description =
        _("In scan mode, print using shell script style");

    G_option_required(input_opt, file_list_opt, NULL);
    G_option_exclusive(input_opt, file_list_opt, NULL);
    G_option_required(count_output_opt, sum_output_opt, mean_output_opt,
                      prop_count_output_opt, prop_sum_output_opt, print_flag,
                      scan_flag, shell_style, NULL);
    G_option_requires(base_rast_res_flag, base_raster_opt, NULL);
    G_option_requires_all(mean_output_opt, count_output_opt, sum_output_opt,
                          NULL);
    G_option_requires_all(prop_count_output_opt, count_output_opt, NULL);
    G_option_requires_all(prop_sum_output_opt, sum_output_opt, NULL);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    if (shell_style->answer && !scan_flag->answer) {
        scan_flag->answer = TRUE;
    }

    /* check intensity range and extent relation */
    if (scan_flag->answer) {
        if (irange_opt->answer)
            G_warning(_("%s will not be taken into account during scan"),
                      irange_opt->key);
    }

    int only_valid = FALSE;

    if (only_valid_flag->answer)
        only_valid = TRUE;

    struct StringList infiles;

    if (file_list_opt->answer) {
        if (access(file_list_opt->answer, F_OK) != 0)
            G_fatal_error(_("File <%s> does not exist"), file_list_opt->answer);
        string_list_from_file(&infiles, file_list_opt->answer);
    }
    else {
        string_list_from_one_item(&infiles, input_opt->answer);
    }

    double zscale = 1.0;
    double iscale = 1.0;

    if (zscale_opt->answer)
        zscale = atof(zscale_opt->answer);
    if (iscale_opt->answer)
        iscale = atof(iscale_opt->answer);

    LASReaderH LAS_reader;
    LASHeaderH LAS_header;
    LASSRSH LAS_srs;
    char *infile;

    /* for the CRS info */
    const char *projstr;
    struct Cell_head current_region = {0};
    struct Cell_head file_region = {0};
    G_get_set_window(&current_region);

    /* extent for all data */
    struct Cell_head data_region = {0};

    long unsigned header_count = 0;
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
            projection_check_wkt(file_region, current_region, projstr,
                                 over_flag->answer, shell_style->answer || i);
            /* if there is a problem in some other file, first OK message
             * is printed but than a warning, this is not ideal but hopefully
             * not so confusing when importing multiple files */
        }
        if (scan_flag->answer) {
            /* we assign to the first one (i==0) but update for the rest */
            scan_bounds(LAS_reader, shell_style->answer, FALSE, i, zscale,
                        &data_region);
        }
        /* number of estimated point across all files */
        /* TODO: this should be ull which won't work with percent report */
        header_count += LASHeader_GetPointRecordsCount(LAS_header);
        /* We are closing all again and we will be opening them later,
         * so we don't have to worry about limit for open files. */
        LASSRS_Destroy(LAS_srs);
        LASHeader_Destroy(LAS_header);
        LASReader_Destroy(LAS_reader);
    }
    /* if we are not importing, end */
    if (print_flag->answer || scan_flag->answer)
        exit(EXIT_SUCCESS);

    Rast3d_init_defaults();
    Rast3d_set_error_fun(Rast3d_fatal_error_noargs);

    double irange_min;
    double irange_max;
    int use_irange =
        range_filter_from_option(irange_opt, &irange_min, &irange_max);

    struct ReturnFilter return_filter_struct;
    int use_return_filter = return_filter_create_from_string(
        &return_filter_struct, filter_opt->answer);
    struct ClassFilter class_filter;
    int use_class_filter =
        class_filter_create_from_strings(&class_filter, class_opt->answers);

    /* TODO: rename */
    struct Cell_head input_region;
    SEGMENT base_segment;
    RASTER_MAP_TYPE base_raster_data_type;
    int use_segment = FALSE;
    int use_base_raster_res = FALSE;

    if (base_rast_res_flag->answer)
        use_base_raster_res = TRUE;
    if (base_raster_opt->answer) {
        use_segment = TRUE;
        if (use_base_raster_res) {
            /* read raster's actual extent and resolution */
            Rast_get_cellhd(base_raster_opt->answer, "", &input_region);
            /* TODO: make it only as small as the output */
            Rast_set_input_window(&input_region); /* we use split window */
        }
        else {
            Rast_get_input_window(&input_region);
        }
        rast_segment_open(&base_segment, base_raster_opt->answer,
                          &base_raster_data_type);
    }

    struct PointBinning3D binning;
    int cache = RASTER3D_USE_CACHE_DEFAULT;
    int type = FCELL_TYPE;
    int max_tile_size = 32;

    binning_init(&binning);
    /* TODO: this should probably happen before we change 2D region just to be
     * sure */
    Rast3d_get_window(&binning.region);
    Rast3d_get_window(&binning.flat_region);
    binning.flat_region.depths = 1;
    Rast3d_adjust_region(&binning.flat_region);

    if (count_output_opt->answer) {
        binning.count_raster =
            Rast3d_open_new_opt_tile_size(count_output_opt->answer, cache,
                                          &binning.region, type, max_tile_size);
        if (!binning.count_raster)
            Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                               count_output_opt->answer);
    }
    if (sum_output_opt->answer) {
        binning.sum_raster =
            Rast3d_open_new_opt_tile_size(sum_output_opt->answer, cache,
                                          &binning.region, type, max_tile_size);
        if (!binning.sum_raster)
            Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                               sum_output_opt->answer);
    }
    if (mean_output_opt->answer) {
        binning.mean_raster =
            Rast3d_open_new_opt_tile_size(mean_output_opt->answer, cache,
                                          &binning.region, type, max_tile_size);
        if (!binning.mean_raster)
            Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                               mean_output_opt->answer);
    }
    if (prop_count_output_opt->answer) {
        binning.prop_count_raster =
            Rast3d_open_new_opt_tile_size(prop_count_output_opt->answer, cache,
                                          &binning.region, type, max_tile_size);
        if (!binning.prop_count_raster)
            Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                               prop_count_output_opt->answer);
    }
    if (prop_sum_output_opt->answer) {
        binning.prop_sum_raster =
            Rast3d_open_new_opt_tile_size(prop_sum_output_opt->answer, cache,
                                          &binning.region, type, max_tile_size);
        if (!binning.prop_sum_raster)
            Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                               prop_sum_output_opt->answer);
    }
    if (prop_count_output_opt->answer) {
        binning.count_flat_raster = Rast3d_open_new_opt_tile_size(
            "r3_in_lidar_tmp_sum_flat", cache, &binning.flat_region, type,
            max_tile_size);

        if (!binning.count_flat_raster)
            Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                               count_output_opt->answer);
    }
    if (prop_sum_output_opt->answer) {
        binning.sum_flat_raster = Rast3d_open_new_opt_tile_size(
            "r3_in_lidar_tmp_count_flat", cache, &binning.flat_region, type,
            max_tile_size);
        if (!binning.sum_flat_raster)
            Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                               count_output_opt->answer);
    }

    G_verbose_message(_("Preparing the maps..."));

    G_percent_reset();
    if (binning.count_raster)
        raster3d_set_value_float(binning.count_raster, &binning.region, 0);
    G_percent(25, 100, 1);
    if (binning.sum_raster)
        raster3d_set_value_float(binning.sum_raster, &binning.region, 0);
    G_percent(50, 100, 1);
    if (binning.count_flat_raster)
        raster3d_set_value_float(binning.count_flat_raster,
                                 &binning.flat_region, 0);
    G_percent(75, 100, 1);
    if (binning.sum_flat_raster)
        raster3d_set_value_float(binning.sum_flat_raster, &binning.flat_region,
                                 0);
    G_percent(1, 1, 1); /* flush */

    LASPointH LAS_point;
    double east, north, top;
    int row, col, depth;
    double value;
    double base_z;

    /* TODO: use long long */
    long unsigned inside = 0;
    long unsigned outside = 0;
    long unsigned in_nulls = 0; /* or outside */
    long unsigned n_return_filtered = 0;
    long unsigned n_class_filtered = 0;
    long unsigned irange_filtered = 0;
    long unsigned n_invalid = 0;
    long unsigned counter = 0;

    G_verbose_message(_("Reading points..."));
    G_percent_reset();

    /* loop of input files */
    for (i = 0; i < infiles.num_items; i++) {
        infile = infiles.items[i];
        /* we already know file is there, so just do basic checks */
        LAS_reader = LASReader_Create(infile);
        while ((LAS_point = LASReader_GetNextPoint(LAS_reader)) != NULL) {
            if (counter == 100000) { /* report only some for speed */
                if (inside <
                    header_count) /* TODO: inside can greatly underestimate */
                    G_percent(inside, header_count, 3);
                counter = 0;
            }
            counter++;
            /* We always count them and report because r.in.lidar behavior
             * changed in between 7.0 and 7.2 from undefined (but skipping
             * invalid points) to filtering them out only when requested. */
            if (!LASPoint_IsValid(LAS_point)) {
                n_invalid++;
                if (only_valid)
                    continue;
            }
            if (use_return_filter) {
                int return_n = LASPoint_GetReturnNumber(LAS_point);
                int n_returns = LASPoint_GetNumberOfReturns(LAS_point);

                if (return_filter_is_out(&return_filter_struct, return_n,
                                         n_returns)) {
                    n_return_filtered++;
                    continue;
                }
            }
            if (use_class_filter) {
                int point_class = (int)LASPoint_GetClassification(LAS_point);

                if (class_filter_is_out(&class_filter, point_class)) {
                    n_class_filtered++;
                    continue;
                }
            }
            value = LASPoint_GetIntensity(LAS_point);
            value *= iscale;
            if (use_irange && (value < irange_min || value > irange_max)) {
                irange_filtered++;
                continue;
            }

            east = LASPoint_GetX(LAS_point);
            north = LASPoint_GetY(LAS_point);
            top = LASPoint_GetZ(LAS_point);
            top *= zscale;

            if (use_segment) {
                if (rast_segment_get_value_xy(&base_segment, &input_region,
                                              base_raster_data_type, east,
                                              north, &base_z)) {
                    top -= base_z;
                }
                else {
                    /* TODO: separate message for this */
                    in_nulls += 1;
                    continue;
                }
            }

            Rast3d_location2coord(&binning.region, north, east, top, &col, &row,
                                  &depth);
            if (col >= binning.region.cols || row >= binning.region.rows ||
                depth >= binning.region.depths || col < 0 || row < 0 ||
                depth < 0) {
                outside += 1;
                continue;
            }
            binning_add_point(&binning, row, col, depth, value);
            inside += 1;
        }
        /* close input LAS file */
        LASReader_Destroy(LAS_reader);
    }
    /* end of loop for all input files */

    G_percent(1, 1, 1); /* flush */

    if (binning.prop_count_raster) {
        G_verbose_message(_("Computing proportional count map..."));
        raster3d_divide_by_flat(binning.count_raster, binning.count_flat_raster,
                                binning.prop_count_raster, &binning.region);
    }
    if (binning.prop_sum_raster) {
        G_verbose_message(_("Computing proportional sum map..."));
        raster3d_divide_by_flat(binning.sum_raster, binning.sum_flat_raster,
                                binning.prop_sum_raster, &binning.region);
    }
    if (binning.mean_raster) {
        G_verbose_message(_("Computing mean map..."));
        raster3d_divide(binning.sum_raster, binning.count_raster,
                        binning.mean_raster, &binning.region);
    }

    G_verbose_message(_("Closing the maps..."));

    G_percent_reset();
    if (binning.sum_flat_raster)
        Rast3d_close(binning.sum_flat_raster); /* TODO: delete */
    G_percent(1, 7, 1);
    if (binning.count_flat_raster)
        Rast3d_close(binning.count_flat_raster); /* TODO: delete */
    G_percent(2, 7, 1);
    if (binning.prop_sum_raster)
        Rast3d_close(binning.prop_sum_raster);
    G_percent(2, 7, 1);
    if (binning.prop_count_raster)
        Rast3d_close(binning.prop_count_raster);
    G_percent(4, 7, 1);
    if (binning.mean_raster)
        Rast3d_close(binning.mean_raster);
    G_percent(5, 7, 1);
    if (binning.sum_raster)
        Rast3d_close(binning.sum_raster);
    G_percent(6, 7, 1);
    if (binning.count_raster)
        Rast3d_close(binning.count_raster);
    G_percent(1, 1, 1);

    if (use_segment)
        Segment_close(&base_segment);

    G_message("Number of points inside: %lu", inside);
    if (use_segment)
        G_message("Number of points outside or in base raster NULL cells: %lu",
                  outside + in_nulls);
    else
        G_message("Number of points outside: %lu", outside);
    if (n_invalid && only_valid)
        G_message(_("%lu input points were not valid and filtered out"),
                  n_invalid);
    if (n_return_filtered)
        G_message(_("%lu input points were filtered out by return number"),
                  n_return_filtered);
    if (n_class_filtered)
        G_message(_("%lu input points were filtered out by class number"),
                  n_class_filtered);
    if (irange_filtered)
        G_message(_("%lu input points had intensity out of range"),
                  irange_filtered);
    if (n_invalid && !only_valid)
        G_message(_("%lu input points were not valid, use -%c flag to filter"
                    " them out"),
                  n_invalid, only_valid_flag->key);

    exit(EXIT_SUCCESS);
}
