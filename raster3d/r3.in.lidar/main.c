
/****************************************************************************
*
* MODULE:       r3.in.Lidar
*
* AUTHOR(S):    Vaclav Petras
*
* PURPOSE:      Imports LAS LiDAR point clouds to a 3D raster map using
*               aggregate statistics.
*
* COPYRIGHT:    (C) 2016 Vaclav Petras and the The GRASS Development Team
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

#include "rast_segment.h"
#include "filters.h"

struct PointBinning3D
{
    RASTER3D_Region region, flat_region;
    RASTER3D_Map *count_raster, *sum_raster, *mean_raster;
    RASTER3D_Map *count_flat_raster, *sum_flat_raster;
    RASTER3D_Map *prop_count_raster, *prop_sum_raster;
};

static void raster3d_set_value_float(RASTER3D_Map * raster,
                                     RASTER3D_Region * region, float value)
{
    int col, row, depth;

    for (depth = 0; depth < region->depths; depth++)
        for (row = 0; row < region->rows; row++)
            for (col = 0; col < region->cols; col++)
                Rast3d_put_float(raster, col, row, depth, value);
}

/* c = a / b */
static void raster3d_divide(RASTER3D_Map * a, RASTER3D_Map * b,
                            RASTER3D_Map * c, RASTER3D_Region * region)
{
    int col, row, depth;
    double tmp;

    for (depth = 0; depth < region->depths; depth++)
        for (row = 0; row < region->rows; row++)
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

/* c = a / b where b has depth 1 */
static void raster3d_divide_by_flat(RASTER3D_Map * a, RASTER3D_Map * b,
                                    RASTER3D_Map * c,
                                    RASTER3D_Region * region)
{
    int col, row, depth;
    double tmp;

    for (depth = 0; depth < region->depths; depth++)
        for (row = 0; row < region->rows; row++)
            for (col = 0; col < region->cols; col++) {
                tmp = Rast3d_get_double(b, col, row, 0);
                /* since it is count, using cast to integer to check
                   againts zero, limits the value to max of CELL */
                if (((CELL) tmp) > 0) {
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

void binning_add_point(struct PointBinning3D binning, int row, int col,
                       int depth, double value)
{
    double tmp;                 /* TODO: check if these should be in binning struct */

    tmp = Rast3d_get_double(binning.count_raster, col, row, depth);
    Rast3d_put_double(binning.count_raster, col, row, depth, tmp + 1);
    tmp = Rast3d_get_double(binning.count_flat_raster, col, row, 0);
    Rast3d_put_double(binning.count_flat_raster, col, row, 0, tmp + 1);
    tmp = Rast3d_get_double(binning.sum_raster, col, row, depth);
    Rast3d_put_double(binning.sum_raster, col, row, depth, tmp + value);
    tmp = Rast3d_get_double(binning.sum_flat_raster, col, row, 0);
    Rast3d_put_double(binning.sum_flat_raster, col, row, 0, tmp + value);
}

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *input_opt;
    struct Option *count_output_opt, *sum_output_opt, *mean_output_opt;
    struct Option *prop_count_output_opt, *prop_sum_output_opt;
    struct Option *filter_opt, *class_opt;
    struct Option *base_raster_opt;
    struct Flag *base_rast_res_flag;
    struct Flag *only_valid_flag;

    G_gisinit(argv[0]);
    module = G_define_module();
    G_add_keyword(_("3D raster"));
    G_add_keyword(_("import"));
    G_add_keyword(_("LIDAR"));
    module->description = _("Creates a 3D raster map from LAS LiDAR points");

    input_opt = G_define_standard_option(G_OPT_F_BIN_INPUT);
    input_opt->required = YES;
    input_opt->label = _("LAS input file");
    input_opt->description =
        _("LiDAR input file in LAS format (*.las or *.laz)");
    input_opt->guisection = _("Input");

    count_output_opt = G_define_standard_option(G_OPT_R3_OUTPUT);
    count_output_opt->key = "n";
    count_output_opt->required = YES;
    count_output_opt->label = _("Count of points per cell");
    count_output_opt->guisection = _("Output");

    sum_output_opt = G_define_standard_option(G_OPT_R3_OUTPUT);
    sum_output_opt->key = "sum";
    sum_output_opt->required = YES;
    sum_output_opt->label = _("Sum of values of point intensities per cell");
    sum_output_opt->guisection = _("Output");

    mean_output_opt = G_define_standard_option(G_OPT_R3_OUTPUT);
    mean_output_opt->key = "mean";
    mean_output_opt->required = YES;
    mean_output_opt->label = _("Mean of point intensities per cell");
    mean_output_opt->guisection = _("Output");

    /* TODO: proportional versus relative in naming */
    prop_count_output_opt = G_define_standard_option(G_OPT_R3_OUTPUT);
    prop_count_output_opt->key = "proportional_n";
    prop_count_output_opt->required = YES;
    prop_count_output_opt->label =
        _("3D raster map of proportional point count");
    prop_count_output_opt->description =
        _("Point count per 3D cell divided by point count per vertical"
          " column");
    prop_count_output_opt->guisection = _("Proportional output");

    prop_sum_output_opt = G_define_standard_option(G_OPT_R3_OUTPUT);
    prop_sum_output_opt->key = "proportional_sum";
    prop_sum_output_opt->required = YES;
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
    base_raster_opt->label =
        _("Subtract raster values from the z coordinates");
    base_raster_opt->description =
        _("The scale for z is applied beforehand, the filter afterwards");
    base_raster_opt->guisection = _("Transform");

    base_rast_res_flag = G_define_flag();
    base_rast_res_flag->key = 'd';
    base_rast_res_flag->description =
        _("Use base raster actual resolution instead of computational region");

    only_valid_flag = G_define_flag();
    only_valid_flag->key = 'v';
    only_valid_flag->label = _("Use only valid points");
    only_valid_flag->description =
        _("Points invalid according to APSRS LAS specification will be"
          " filtered out");
    only_valid_flag->guisection = _("Selection");

    G_option_requires(base_rast_res_flag, base_raster_opt, NULL);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    int only_valid = FALSE;
    if (only_valid_flag->answer)
        only_valid = TRUE;

    LASReaderH LAS_reader;

    LAS_reader = LASReader_Create(input_opt->answer);
    if (LAS_reader == NULL)
        G_fatal_error(_("Unable to open file <%s>"), input_opt->answer);

    Rast3d_init_defaults();
    Rast3d_set_error_fun(Rast3d_fatal_error_noargs);

    struct ReturnFilter return_filter_struct;
    int use_return_filter =
        return_filter_create_from_string(&return_filter_struct,
                                         filter_opt->answer);
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
            Rast_set_input_window(&input_region);       /* we use split window */
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

    Rast3d_get_window(&binning.region);
    Rast3d_get_window(&binning.flat_region);
    binning.flat_region.depths = 1;
    Rast3d_adjust_region(&binning.flat_region);

    binning.count_raster =
        Rast3d_open_new_opt_tile_size(count_output_opt->answer, cache,
                                      &binning.region, type, max_tile_size);
    if (!binning.count_raster)
        Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                           count_output_opt->answer);
    binning.sum_raster = Rast3d_open_new_opt_tile_size(sum_output_opt->answer,
                                                       cache,
                                                       &binning.region, type,
                                                       max_tile_size);
    if (!binning.sum_raster)
        Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                           sum_output_opt->answer);
    binning.mean_raster =
        Rast3d_open_new_opt_tile_size(mean_output_opt->answer, cache,
                                      &binning.region, type, max_tile_size);
    if (!binning.mean_raster)
        Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                           mean_output_opt->answer);
    binning.count_flat_raster =
        Rast3d_open_new_opt_tile_size("r3_in_lidar_tmp_sum_flat", cache,
                                      &binning.flat_region, type,
                                      max_tile_size);
    if (!binning.count_flat_raster)
        Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                           count_output_opt->answer);
    binning.sum_flat_raster =
        Rast3d_open_new_opt_tile_size("r3_in_lidar_tmp_count_flat", cache,
                                      &binning.flat_region, type,
                                      max_tile_size);
    if (!binning.sum_flat_raster)
        Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                           count_output_opt->answer);
    binning.prop_count_raster =
        Rast3d_open_new_opt_tile_size(prop_count_output_opt->answer, cache,
                                      &binning.region, type, max_tile_size);
    if (!binning.prop_count_raster)
        Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                           prop_count_output_opt->answer);
    binning.prop_sum_raster =
        Rast3d_open_new_opt_tile_size(prop_sum_output_opt->answer, cache,
                                      &binning.region, type, max_tile_size);
    if (!binning.prop_sum_raster)
        Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                           prop_sum_output_opt->answer);

    raster3d_set_value_float(binning.count_raster, &binning.region, 0);
    raster3d_set_value_float(binning.sum_raster, &binning.region, 0);
    raster3d_set_value_float(binning.count_flat_raster, &binning.flat_region,
                             0);
    raster3d_set_value_float(binning.sum_flat_raster, &binning.flat_region,
                             0);

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
    long unsigned n_invalid = 0;

    while ((LAS_point = LASReader_GetNextPoint(LAS_reader)) != NULL) {
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
            if (return_filter_is_out(&return_filter_struct, return_n, n_returns)) {
                n_return_filtered++;
                continue;
            }
        }
        if (use_class_filter) {
            int point_class = (int) LASPoint_GetClassification(LAS_point);
            if (class_filter_is_out(&class_filter, point_class)) {
                n_class_filtered++;
                continue;
            }
        }

        east = LASPoint_GetX(LAS_point);
        north = LASPoint_GetY(LAS_point);
        top = LASPoint_GetZ(LAS_point);

        if (use_segment) {
            if (rast_segment_get_value_xy(&base_segment, &input_region,
                                          base_raster_data_type, east, north,
                                          &base_z)) {
                top -= base_z;
            }
            else {
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
        value = LASPoint_GetIntensity(LAS_point);
        binning_add_point(binning, row, col, depth, value);
        inside += 1;
    }

    raster3d_divide_by_flat(binning.count_raster, binning.count_flat_raster,
                            binning.prop_count_raster, &binning.region);
    raster3d_divide_by_flat(binning.sum_raster, binning.sum_flat_raster,
                            binning.prop_sum_raster, &binning.region);

    raster3d_divide(binning.sum_raster, binning.count_raster,
                    binning.mean_raster, &binning.region);

    G_message("Number of points inside: %lu", inside);
    if (use_segment)
        G_message
            ("Number of points outside or in base raster NULL cells: %lu",
             outside + in_nulls);
    else
        G_message("Number of points outside: %lu", outside);
    if (n_invalid && only_valid)
        G_message(_("%lu input points were not valid and filtered out"),
                  n_invalid);
    if (n_return_filtered)
        G_message(_("%lu input points were filtered out by return number"), n_return_filtered);
    if (n_class_filtered)
        G_message(_("%lu input points were filtered out by class number"), n_class_filtered);
    if (n_invalid && !only_valid)
        G_message(_("%lu input points were not valid, use -%c flag to filter"
                    " them out"), n_invalid, only_valid_flag->key);

    Rast3d_close(binning.prop_sum_raster);
    Rast3d_close(binning.prop_count_raster);
    Rast3d_close(binning.sum_flat_raster);      /* TODO: delete */
    Rast3d_close(binning.count_flat_raster);    /* TODO: delete */
    Rast3d_close(binning.mean_raster);
    Rast3d_close(binning.sum_raster);
    Rast3d_close(binning.count_raster);

    if (use_segment)
        Segment_close(&base_segment);

    exit(EXIT_SUCCESS);
}
