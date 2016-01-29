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

static void raster3d_set_value_float(RASTER3D_Map *raster, RASTER3D_Region *region, float value)
{
    int col, row, depth;

    for (depth = 0; depth < region->depths; depth++)
        for (row = 0; row < region->rows; row++)
            for (col = 0; col < region->cols; col++)
                Rast3d_put_float(raster, col, row, depth, value);
}

/* c = a / b */
static void raster3d_divide(RASTER3D_Map *a, RASTER3D_Map *b, RASTER3D_Map *c, RASTER3D_Region *region)
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
static void raster3d_divide_by_flat(RASTER3D_Map *a, RASTER3D_Map *b, RASTER3D_Map *c, RASTER3D_Region *region)
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

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *input_opt;
    struct Option *count_output_opt, *sum_output_opt, *mean_output_opt;
    struct Option *prop_count_output_opt, *prop_sum_output_opt;

    G_gisinit(argv[0]);
    module = G_define_module();
    G_add_keyword(_("3D raster"));
    G_add_keyword(_("import"));
    G_add_keyword(_("LIDAR"));
    module->description =
        _("Creates a 3D raster map from LAS LiDAR points");

    input_opt = G_define_standard_option(G_OPT_F_BIN_INPUT);
    input_opt->required = YES;
    input_opt->label = _("LAS input file");
    input_opt->description = _("LiDAR input file in LAS format (*.las or *.laz)");
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

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    LASReaderH LAS_reader;
    LAS_reader = LASReader_Create(input_opt->answer);
    if (LAS_reader == NULL)
        G_fatal_error(_("Unable to open file <%s>"), input_opt->answer);

    Rast3d_init_defaults();
    Rast3d_set_error_fun(Rast3d_fatal_error_noargs);

    RASTER3D_Region region, flat_region;
    RASTER3D_Map *count_raster, *sum_raster, *mean_raster;
    RASTER3D_Map *count_flat_raster, *sum_flat_raster;
    RASTER3D_Map *prop_count_raster, *prop_sum_raster;

    Rast3d_get_window(&region);
    Rast3d_get_window(&flat_region);
    flat_region.depths = 1;
    Rast3d_adjust_region(&flat_region);

    count_raster = Rast3d_open_new_opt_tile_size(count_output_opt->answer,
                                           RASTER3D_USE_CACHE_DEFAULT,
                                           &region, FCELL_TYPE, 32);
    if (!count_raster)
        Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                           count_output_opt->answer);
    sum_raster = Rast3d_open_new_opt_tile_size(sum_output_opt->answer,
                                           RASTER3D_USE_CACHE_DEFAULT,
                                           &region, FCELL_TYPE, 32);
    if (!sum_raster)
        Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                           sum_output_opt->answer);
    mean_raster = Rast3d_open_new_opt_tile_size(mean_output_opt->answer,
                                           RASTER3D_USE_CACHE_DEFAULT,
                                           &region, FCELL_TYPE, 32);
    if (!mean_raster)
        Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                           mean_output_opt->answer);
    count_flat_raster = Rast3d_open_new_opt_tile_size("r3_in_lidar_tmp_sum_flat",
                                           RASTER3D_USE_CACHE_DEFAULT,
                                           &flat_region, FCELL_TYPE, 32);
    if (!count_flat_raster)
        Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                           count_output_opt->answer);
    sum_flat_raster = Rast3d_open_new_opt_tile_size("r3_in_lidar_tmp_count_flat",
                                           RASTER3D_USE_CACHE_DEFAULT,
                                           &flat_region, FCELL_TYPE, 32);
    if (!sum_flat_raster)
        Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                           count_output_opt->answer);
    prop_count_raster = Rast3d_open_new_opt_tile_size(prop_count_output_opt->answer,
                                           RASTER3D_USE_CACHE_DEFAULT,
                                           &region, FCELL_TYPE, 32);
    if (!prop_count_raster)
        Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                           prop_count_output_opt->answer);
    prop_sum_raster = Rast3d_open_new_opt_tile_size(prop_sum_output_opt->answer,
                                           RASTER3D_USE_CACHE_DEFAULT,
                                           &region, FCELL_TYPE, 32);
    if (!prop_sum_raster)
        Rast3d_fatal_error(_("Unable to create 3D raster map <%s>"),
                           prop_sum_output_opt->answer);

    raster3d_set_value_float(count_raster, &region, 0);
    raster3d_set_value_float(sum_raster, &region, 0);
    raster3d_set_value_float(count_flat_raster, &flat_region, 0);
    raster3d_set_value_float(sum_flat_raster, &flat_region, 0);

    LASPointH LAS_point;
    double east, north, top;
    int row, col, depth;
    double value;
    double tmp;

    /* TODO: use long long */
    long unsigned inside = 0;
    long unsigned outside = 0;

    while ((LAS_point = LASReader_GetNextPoint(LAS_reader)) != NULL) {
        if (!LASPoint_IsValid(LAS_point))
            continue;

        east = LASPoint_GetX(LAS_point);
        north = LASPoint_GetY(LAS_point);
        top = LASPoint_GetZ(LAS_point);

        if (!Rast3d_is_valid_location(&region, north, east, top)) {
            outside += 1;
            continue;
        }
        Rast3d_location2coord(&region, north, east, top, &col, &row, &depth);
        value = LASPoint_GetIntensity(LAS_point);

        tmp = Rast3d_get_double(count_raster, col, row, depth);
        Rast3d_put_double(count_raster, col, row, depth, tmp + 1);
        tmp = Rast3d_get_double(count_flat_raster, col, row, 0);
        Rast3d_put_double(count_flat_raster, col, row, 0, tmp + 1);
        tmp = Rast3d_get_double(sum_raster, col, row, depth);
        Rast3d_put_double(sum_raster, col, row, depth, tmp + value);
        tmp = Rast3d_get_double(sum_flat_raster, col, row, 0);
        Rast3d_put_double(sum_flat_raster, col, row, 0, tmp + value);

        inside += 1;
    }

    raster3d_divide_by_flat(count_raster, count_flat_raster, prop_count_raster, &region);
    raster3d_divide_by_flat(sum_raster, sum_flat_raster, prop_sum_raster, &region);

    raster3d_divide(sum_raster, count_raster, mean_raster, &region);

    G_message("Number of point inside: %lu", inside);
    G_message("Number of point outside: %lu", outside);

    Rast3d_close(prop_sum_raster);
    Rast3d_close(prop_count_raster);
    Rast3d_close(sum_flat_raster);  /* TODO: delete */
    Rast3d_close(count_flat_raster);  /* TODO: delete */
    Rast3d_close(mean_raster);
    Rast3d_close(sum_raster);
    Rast3d_close(count_raster);

    exit(EXIT_SUCCESS);
}
