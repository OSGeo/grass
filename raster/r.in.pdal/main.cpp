/****************************************************************************
 *
 * MODULE:    r.in.pdal
 *
 * AUTHOR(S): Vaclav Petras
 *            Based on r.in.xyz and r.in.lidar by Markus Metz,
 *            Hamish Bowman, Volker Wichmann, Maris Nartiss
 *
 * PURPOSE:   Imports LAS LiDAR point clouds to a raster map using
 *            aggregate statistics.
 *
 * COPYRIGHT: (C) 2019-2024 by Vaclav Petras and the GRASS Development Team
 *
 *            This program is free software under the GNU General Public
 *            License (>=v2). Read the file COPYING that comes with
 *            GRASS for details.
 *
 *****************************************************************************/

#include <cstdio>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#endif
#include <pdal/PointTable.hpp>
#include <pdal/PointLayout.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/io/LasReader.hpp>
#include <pdal/io/LasHeader.hpp>
#include <pdal/Options.hpp>
#include <pdal/filters/MergeFilter.hpp>
#include <pdal/filters/ReprojectionFilter.hpp>
#include <pdal/filters/StreamCallbackFilter.hpp>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

extern "C" {
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
}

#include "grasslidarfilter.h"
#include "grassrasterwriter.h"
#include "info.h"

extern "C" {
#include "lidar.h"
#include "projection.h"
#include "filters.h"
#include "point_binning.h"
#include "string_list.h"
}

#define BUFFSIZE GPATH_MAX

int main(int argc, char *argv[])
{
    int out_fd;
    char *outmap;

    RASTER_MAP_TYPE rtype, base_raster_data_type;
    struct History history;
    char title[64];
    SEGMENT base_segment;
    struct PointBinning point_binning;
    void *raster_row;
    struct Cell_head region = {};
    struct Cell_head input_region = {};
    int rows, cols; /* scan box size */

    char buff[BUFFSIZE];

    struct BinIndex bin_index_nodes;

    bin_index_nodes.num_nodes = 0;
    bin_index_nodes.max_nodes = 0;
    bin_index_nodes.nodes = NULL;

    struct Cell_head loc_wind = {};

    G_gisinit(argv[0]);

    GModule *module = G_define_module();

    G_add_keyword(_("raster"));
    G_add_keyword(_("import"));
    G_add_keyword(_("LIDAR"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("conversion"));
    G_add_keyword(_("aggregation"));
    G_add_keyword(_("binning"));
    module->description = _("Creates a raster map from LAS LiDAR points using "
                            "univariate statistics.");

    Option *input_opt = G_define_standard_option(G_OPT_F_BIN_INPUT);

    input_opt->required = NO;
    input_opt->label = _("LAS input file");
    input_opt->description =
        _("LiDAR input files in LAS format (*.las or *.laz)");
    input_opt->guisection = _("Input");

    Option *output_opt = G_define_standard_option(G_OPT_R_OUTPUT);

    output_opt->required = NO;
    output_opt->guisection = _("Output");

    Option *file_list_opt = G_define_standard_option(G_OPT_F_INPUT);

    file_list_opt->key = "file";
    file_list_opt->label = _("File containing names of LAS input files");
    file_list_opt->description =
        _("LiDAR input files in LAS format (*.las or *.laz)");
    file_list_opt->required = NO;
    file_list_opt->guisection = _("Input");

    Option *method_opt = G_define_option();

    method_opt->key = "method";
    method_opt->type = TYPE_STRING;
    method_opt->required = NO;
    method_opt->description = _("Statistic to use for raster values");
    method_opt->options =
        "n,min,max,range,sum,mean,stddev,variance,coeff_var,median,mode,"
        "percentile,skewness,trimmean,sidnmax,sidnmin,ev1,ev2,ev3";
    method_opt->answer = const_cast<char *>("mean");

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
               "mode;%s;"
               "percentile;%s;"
               "skewness;%s;"
               "trimmean;%s;"
               "sidnmax;%s;"
               "sidnmin;%s;"
               "ev1;%s;"
               "ev2;%s;"
               "ev3;%s;",
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
               _("Mode value of point values in cell"),
               _("pth (nth) percentile of point values in cell"),
               _("Skewness of point values in cell"),
               _("Trimmed mean of point values in cell"),
               _("Maximum number of points in cell per source ID"),
               _("Minimum number of points in cell per source ID"),
               _("First eigenvalue of point x, y, z coordinates"),
               _("Second eigenvalue of point x, y, z coordinates"),
               _("Third eigenvalue of point x, y, z coordinates"));

    Option *type_opt = G_define_standard_option(G_OPT_R_TYPE);

    type_opt->required = NO;
    type_opt->answer = const_cast<char *>("FCELL");

    Option *base_raster_opt = G_define_standard_option(G_OPT_R_INPUT);

    base_raster_opt->key = "base_raster";
    base_raster_opt->required = NO;
    base_raster_opt->label = _("Subtract raster values from the Z coordinates");
    base_raster_opt->description =
        _("The scale for Z is applied beforehand, the range filter for"
          " Z afterwards");
    base_raster_opt->guisection = _("Transform");

    Option *zrange_opt = G_define_option();

    zrange_opt->key = "zrange";
    zrange_opt->type = TYPE_DOUBLE;
    zrange_opt->required = NO;
    zrange_opt->key_desc = "min,max";
    zrange_opt->label = _("Filter range for Z data (min,max)");
    zrange_opt->description =
        _("Applied after base_raster transformation step");
    zrange_opt->guisection = _("Selection");

    Option *zscale_opt = G_define_option();

    zscale_opt->key = "zscale";
    zscale_opt->type = TYPE_DOUBLE;
    zscale_opt->required = NO;
    zscale_opt->answer = const_cast<char *>("1.0");

    zscale_opt->description = _("Scale to apply to Z data");
    zscale_opt->guisection = _("Transform");

    Option *irange_opt = G_define_option();

    irange_opt->key = "irange";
    irange_opt->type = TYPE_DOUBLE;
    irange_opt->required = NO;
    irange_opt->key_desc = "min,max";
    irange_opt->description = _("Filter range for intensity values (min,max)");
    irange_opt->guisection = _("Selection");

    Option *iscale_opt = G_define_option();

    iscale_opt->key = "iscale";
    iscale_opt->type = TYPE_DOUBLE;
    iscale_opt->required = NO;
    iscale_opt->answer = const_cast<char *>("1.0");

    iscale_opt->description = _("Scale to apply to intensity values");
    iscale_opt->guisection = _("Transform");

    Option *drange_opt = G_define_option();

    drange_opt->key = "drange";
    drange_opt->type = TYPE_DOUBLE;
    drange_opt->required = NO;
    drange_opt->key_desc = "min,max";
    drange_opt->description =
        _("Filter range for output dimension values (min,max)");
    drange_opt->guisection = _("Selection");

    Option *dscale_opt = G_define_option();

    dscale_opt->key = "dscale";
    dscale_opt->type = TYPE_DOUBLE;
    dscale_opt->required = NO;
    dscale_opt->label = _("Scale to apply to output dimension values");
    dscale_opt->description =
        _("Use if output dimension is not Z or intensity");
    dscale_opt->guisection = _("Transform");

    Flag *reproject_flag = G_define_flag();

    reproject_flag->key = 'w';
    reproject_flag->label =
        _("Reproject to project's coordinate system if needed");
    reproject_flag->description =
        _("Reprojects input dataset to the coordinate system of"
          " the GRASS project (by default only datasets with"
          " matching coordinate system can be imported");
    reproject_flag->guisection = _("Projection");

    // TODO: from the API it seems that also prj file path and proj string will
    // work
    Option *input_srs_opt = G_define_option();

    input_srs_opt->key = "input_srs";
    input_srs_opt->type = TYPE_STRING;
    input_srs_opt->required = NO;
    input_srs_opt->label =
        _("Input dataset projection (WKT or EPSG, e.g. EPSG:4326)");
    input_srs_opt->description =
        _("Override input dataset coordinate system using EPSG code"
          " or WKT definition");
    input_srs_opt->guisection = _("Projection");

    /* I would prefer to call the following "percentile", but that has too
     * much namespace overlap with the "percent" option above */
    Option *pth_opt = G_define_option();

    pth_opt->key = "pth";
    pth_opt->type = TYPE_INTEGER;
    pth_opt->required = NO;
    pth_opt->options = "1-100";
    pth_opt->description = _("pth percentile of the values");
    pth_opt->guisection = _("Statistic");

    Option *trim_opt = G_define_option();

    trim_opt->key = "trim";
    trim_opt->type = TYPE_DOUBLE;
    trim_opt->required = NO;
    trim_opt->options = "0-50";
    trim_opt->label =
        _("Discard given percentage of the smallest and largest values");
    trim_opt->description = _("Discard <trim> percent of the smallest and "
                              "<trim> percent of the largest observations");
    trim_opt->guisection = _("Statistic");

    Option *res_opt = G_define_option();

    res_opt->key = "resolution";
    res_opt->type = TYPE_DOUBLE;
    res_opt->required = NO;
    res_opt->description = _("Output raster resolution");
    res_opt->guisection = _("Output");

    Option *return_filter_opt = G_define_option();

    return_filter_opt->key = "return_filter";
    return_filter_opt->type = TYPE_STRING;
    return_filter_opt->required = NO;
    return_filter_opt->label = _("Only import points of selected return type");
    return_filter_opt->description =
        _("If not specified, all points are imported");
    return_filter_opt->options = "first,last,mid";
    return_filter_opt->guisection = _("Selection");

    Option *class_filter_opt = G_define_option();

    class_filter_opt->key = "class_filter";
    class_filter_opt->type = TYPE_INTEGER;
    class_filter_opt->multiple = YES;
    class_filter_opt->required = NO;
    class_filter_opt->label = _("Only import points of selected class(es)");
    class_filter_opt->description =
        _("Input is comma separated integers. "
          "If not specified, all points are imported.");
    class_filter_opt->guisection = _("Selection");

    Option *dimension_opt = G_define_option();

    dimension_opt->key = "dimension";
    dimension_opt->type = TYPE_STRING;
    dimension_opt->required = NO;
    dimension_opt->label = _("Dimension (variable) to use for raster values");
    dimension_opt->options =
        "z,intensity,number,returns,direction,angle,class,source";
    dimension_opt->answer = const_cast<char *>("z");

    dimension_opt->guisection = _("Selection");
    G_asprintf((char **)&(dimension_opt->descriptions),
               "z;%s;"
               "intensity;%s;"
               "number;%s;"
               "returns;%s;"
               "direction;%s;"
               "angle;%s;"
               "class;%s;"
               "source;%s",
               _("Z coordinate"),
               /* GTC: LAS LiDAR point property */
               _("Intensity"),
               /* GTC: LAS LiDAR point property */
               _("Return number"),
               /* GTC: LAS LiDAR point property */
               _("Number of returns"),
               /* GTC: LAS LiDAR point property */
               _("Scan direction"),
               /* GTC: LAS LiDAR point property */
               _("Scan angle"),
               /* GTC: LAS LiDAR point property */
               _("Point class value"),
               /* GTC: LAS LiDAR point property */
               _("Source ID"));

    Option *user_dimension_opt = G_define_option();

    user_dimension_opt->key = "user_dimension";
    user_dimension_opt->type = TYPE_STRING;
    user_dimension_opt->required = NO;
    user_dimension_opt->label =
        _("Custom dimension (variable) to use for raster values");
    user_dimension_opt->description = _("PDAL dimension name");
    user_dimension_opt->guisection = _("Selection");

    Flag *extents_flag = G_define_flag();

    extents_flag->key = 'e';
    extents_flag->label =
        _("Use the extent of the input for the raster extent");
    extents_flag->description =
        _("Set internally computational region extents based on the"
          " point cloud");
    extents_flag->guisection = _("Output");

    Flag *set_region_flag = G_define_flag();

    set_region_flag->key = 'n';
    set_region_flag->label =
        _("Set computation region to match the new raster map");
    set_region_flag->description =
        _("Set computation region to match the 2D extent and resolution"
          " of the newly created new raster map");
    set_region_flag->guisection = _("Output");

    Flag *over_flag = G_define_flag();

    over_flag->key = 'o';
    over_flag->label =
        _("Override projection check (use current project's CRS)");
    over_flag->description =
        _("Assume that the dataset has the same coordinate reference system as "
          "the current project");
    over_flag->guisection = _("Projection");

    Flag *base_rast_res_flag = G_define_flag();

    base_rast_res_flag->key = 'd';
    base_rast_res_flag->label =
        _("Use base raster resolution instead of computational region");
    base_rast_res_flag->description =
        _("For getting values from base raster, use its actual"
          " resolution instead of computational region resolution");
    base_rast_res_flag->guisection = _("Transform");

    Flag *print_info_flag = G_define_flag();

    print_info_flag->key = 'p';
    print_info_flag->description = _("Print LAS file info and exit");

    Flag *print_extent_flag = G_define_flag();

    print_extent_flag->key = 'g';
    print_extent_flag->description =
        _("Print data file extent in shell script style and then exit");

    G_option_required(input_opt, file_list_opt, NULL);
    G_option_exclusive(input_opt, file_list_opt, NULL);
    G_option_requires(base_rast_res_flag, base_raster_opt, NULL);
    G_option_exclusive(base_rast_res_flag, res_opt, NULL);
    G_option_exclusive(reproject_flag, over_flag, NULL);
    G_option_required(output_opt, print_extent_flag, print_info_flag, NULL);

    if (G_parser(argc, argv))
        return EXIT_FAILURE;

    /* Get input file list. Needs to be done before printing extent. */
    struct StringList infiles;

    if (file_list_opt->answer) {
        if (access(file_list_opt->answer, F_OK) != 0)
            G_fatal_error(_("File <%s> does not exist"), file_list_opt->answer);
        string_list_from_file(&infiles, file_list_opt->answer);
    }
    else {
        string_list_from_one_item(&infiles, input_opt->answer);
    }

    /* If we print extent, there is no need to validate rest of the input */
    if (print_extent_flag->answer) {
#ifdef PDAL_USE_NOSRS
        print_extent(&infiles, over_flag->answer);
#else
        print_extent(&infiles);
#endif
        exit(EXIT_SUCCESS);
    }

    if (print_info_flag->answer) {
#ifdef PDAL_USE_NOSRS
        print_lasinfo(&infiles, over_flag->answer);
#else
        print_lasinfo(&infiles);
#endif
        exit(EXIT_SUCCESS);
    }

    /* we could use rules but this gives more info and allows continuing */
    if (set_region_flag->answer && !(extents_flag->answer || res_opt->answer ||
                                     base_rast_res_flag->answer)) {
        G_warning(_("Flag %c makes sense only with %s option or -%c flag or "
                    "-%c flag"),
                  set_region_flag->key, res_opt->key, extents_flag->key,
                  base_rast_res_flag->key);
        /* avoid the call later on */
        set_region_flag->answer = '\0';
    }

    /* Trim option is used only for trimmean method */
    if (trim_opt->answer != NULL &&
        strcmp(method_opt->answer, "trimmean") != 0) {
        G_fatal_error(_("Trim option can be used only with trimmean method"));
    }

    /* Point density counting does not require any dimension information */
    if ((strcmp(method_opt->answer, "sidnmax") == 0 ||
         strcmp(method_opt->answer, "sidnmin") == 0 ||
         strcmp(method_opt->answer, "n") == 0 ||
         strcmp(method_opt->answer, "ev1") == 0 ||
         strcmp(method_opt->answer, "ev2") == 0 ||
         strcmp(method_opt->answer, "ev3") == 0) &&
        (user_dimension_opt->answer ||
         !(strcmp(dimension_opt->answer, "z") == 0)))
        G_warning(_("Binning methods 'n', 'sidnmax', 'sidnmin' and "
                    "eigenvalues are ignoring specified dimension"));

    /* parse input values */
    outmap = output_opt->answer;
    if (input_opt->answer && access(input_opt->answer, F_OK) != 0) {
        G_fatal_error(_("Input file <%s> does not exist"), input_opt->answer);
    }

    /* Set up input extent for point spatial filter */
    double xmin = 0;
    double ymin = 0;
    double xmax = 0;
    double ymax = 0;
    bool use_spatial_filter = false;

    Rast_get_window(&region);
    /* G_get_window seems to be unreliable if the location has been changed */
    G_get_set_window(
        &loc_wind); /* TODO: v.in.lidar uses G_get_default_window() */

    /* Region is set based on whole point cloud that could be larger than
     * imported part */
    if (extents_flag->answer) {
        double min_x, max_x, min_y, max_y, min_z, max_z;

#ifdef PDAL_USE_NOSRS
        get_extent(&infiles, &min_x, &max_x, &min_y, &max_y, &min_z, &max_z,
                   over_flag->answer);
#else
        get_extent(&infiles, &min_x, &max_x, &min_y, &max_y, &min_z, &max_z);
#endif

        region.east = xmax = max_x;
        region.west = xmin = min_x;
        region.north = ymax = max_y;
        region.south = ymin = min_y;

        use_spatial_filter = true;
    }

    /* Set up filtering options */
    if (!extents_flag->answer) {
        use_spatial_filter =
            spatial_filter_from_current_region(&xmin, &ymin, &xmax, &ymax);
    }

    double zrange_min, zrange_max;
    bool use_zrange =
        range_filter_from_option(zrange_opt, &zrange_min, &zrange_max);
    double irange_min, irange_max;
    bool use_irange =
        range_filter_from_option(irange_opt, &irange_min, &irange_max);
    double drange_min, drange_max;
    bool use_drange =
        range_filter_from_option(drange_opt, &drange_min, &drange_max);
    struct ReturnFilter return_filter_struct;
    bool use_return_filter = return_filter_create_from_string(
        &return_filter_struct, return_filter_opt->answer);
    struct ClassFilter class_filter;
    bool use_class_filter = class_filter_create_from_strings(
        &class_filter, class_filter_opt->answers);

    point_binning_set(&point_binning, method_opt->answer, pth_opt->answer,
                      trim_opt->answer);

    /* Set up output map type */
    if (strcmp("CELL", type_opt->answer) == 0)
        rtype = CELL_TYPE;
    else if (strcmp("DCELL", type_opt->answer) == 0)
        rtype = DCELL_TYPE;
    else
        rtype = FCELL_TYPE;

    if (point_binning.method == METHOD_N ||
        point_binning.method == METHOD_MODE ||
        point_binning.method == METHOD_SIDNMAX ||
        point_binning.method == METHOD_SIDNMIN) {
        if (rtype != CELL_TYPE)
            G_warning(_("Output map type set to CELL"));
        rtype = CELL_TYPE;
    }

    /* Set up output dimension */
    // we use full qualification because the dim ns contains too general names
    pdal::Dimension::Id dim_to_import = pdal::Dimension::Id::Z;

    if (!user_dimension_opt->answer &&
        !(strcmp(dimension_opt->answer, "z") == 0)) {
        /* Should we enfocte the CELL type? */
        if (rtype != CELL_TYPE)
            G_warning(_("Output map type set to CELL"));
        rtype = CELL_TYPE;

        if (strcmp(dimension_opt->answer, "intensity") == 0) {
            dim_to_import = pdal::Dimension::Id::Intensity;
        }
        else if (strcmp(dimension_opt->answer, "number") == 0) {
            dim_to_import = pdal::Dimension::Id::ReturnNumber;
        }
        else if (strcmp(dimension_opt->answer, "returns") == 0) {
            dim_to_import = pdal::Dimension::Id::NumberOfReturns;
        }
        else if (strcmp(dimension_opt->answer, "direction") == 0) {
            dim_to_import = pdal::Dimension::Id::ScanDirectionFlag;
        }
        else if (strcmp(dimension_opt->answer, "angle") == 0) {
            dim_to_import = pdal::Dimension::Id::ScanAngleRank;
        }
        else if (strcmp(dimension_opt->answer, "class") == 0) {
            dim_to_import = pdal::Dimension::Id::Classification;
        }
        else if (strcmp(dimension_opt->answer, "source") == 0) {
            dim_to_import = pdal::Dimension::Id::PointSourceId;
        }
    }

    if (point_binning.method == METHOD_SIDNMAX ||
        point_binning.method == METHOD_SIDNMIN)
        dim_to_import = pdal::Dimension::Id::PointSourceId;

    if (dim_to_import != pdal::Dimension::Id::Z &&
        (strcmp(method_opt->answer, "ev1") == 0 ||
         strcmp(method_opt->answer, "ev2") == 0 ||
         strcmp(method_opt->answer, "ev3") == 0))
        dim_to_import = pdal::Dimension::Id::Z;

    /* Set up axis and output value scaling */
    double zscale = 1.0;
    double iscale = 1.0;
    double dscale = 1.0;
    double output_scale = 1.0;

    if (zscale_opt->answer)
        zscale = atof(zscale_opt->answer);
    if (iscale_opt->answer)
        iscale = atof(iscale_opt->answer);
    if (dscale_opt->answer)
        dscale = atof(dscale_opt->answer);

    if (zscale_opt->answer && dim_to_import == pdal::Dimension::Id::Z)
        output_scale = zscale;
    if (iscale_opt->answer && dim_to_import == pdal::Dimension::Id::Intensity)
        output_scale = iscale;
    if (dscale_opt->answer)
        output_scale = dscale;

    double res = 0.0;

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
    if (base_rast_res_flag->answer) {
        Rast_get_cellhd(base_raster_opt->answer, "", &input_region);
        region.ns_res = input_region.ns_res;
        region.ew_res = input_region.ew_res;
        G_adjust_Cell_head(&region, 0, 0);
    }

    Rast_set_output_window(&region);
    rows = region.rows;
    cols = region.cols;

    G_debug(2, "region.n=%f  region.s=%f  region.ns_res=%f", region.north,
            region.south, region.ns_res);
    G_debug(2, "region.rows=%d  [box_rows=%d]  region.cols=%d", region.rows,
            rows, region.cols);

    /* using segment library for the base raster */
    // TODO: use segment library also for the binning removing the
    // current memory limitations
    // TODO: remove hardcoded memory requirements, let user supply it
    int use_base_raster_res = 0;

    /* TODO: see if the input region extent is smaller than the raster
     * if yes, the we need to load the whole base raster if the -e
     * flag was defined (alternatively clip the regions) */
    if (base_rast_res_flag->answer)
        use_base_raster_res = 1;
    if (base_raster_opt->answer) {
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

    // TODO: use memory requirements supplied by user
    // TODO: use segment library for binning
    point_binning_allocate(&point_binning, rows, cols, rtype);

    /* open output map */
    out_fd = Rast_open_new(outmap, rtype);

    /* allocate memory for a single row of output data */
    raster_row = Rast_allocate_output_buf(rtype);

    G_message(_("Reading data..."));

    std::vector<pdal::Stage *> readers;
    pdal::StageFactory factory;
    pdal::MergeFilter merge_filter;
    /* loop of input files */
    for (int i = 0; i < infiles.num_items; i++) {
        const char *infile = infiles.items[i];

        std::string pdal_read_driver = factory.inferReaderDriver(infile);
        if (pdal_read_driver.empty())
            G_fatal_error(_("Cannot determine input file type of <%s>"),
                          infile);

        pdal::Options las_opts;
        pdal::Option las_opt("filename", infile);
        las_opts.add(las_opt);
#ifdef PDAL_USE_NOSRS
        if (over_flag->answer) {
            pdal::Option nosrs_opt("nosrs", true);
            las_opts.add(nosrs_opt);
        }
#endif
        // stages created by factory are destroyed with the factory
        pdal::Stage *reader = factory.createStage(pdal_read_driver);
        if (!reader)
            G_fatal_error(
                _("PDAL reader creation failed, a wrong format of <%s>"),
                infile);
        reader->setOptions(las_opts);
        readers.push_back(reader);
        merge_filter.setInput(*reader);
    }

    // we need to keep pointer to the last stage
    // merge filter puts the n readers into one stage,
    // so we don't have to worry about the list of stages later
    pdal::Stage *last_stage = &merge_filter;
    pdal::ReprojectionFilter reprojection_filter;

    // we reproject when requested regardless of the input projection
    if (reproject_flag->answer) {
        G_message(_("Reprojecting the input to the project's CRS"));
        char *proj_wkt = location_projection_as_wkt(false);

        pdal::Options o4;
        // TODO: try catch for user input error
        if (input_srs_opt->answer)
            o4.add<std::string>("in_srs", input_srs_opt->answer);
        o4.add<std::string>("out_srs", proj_wkt);
        reprojection_filter.setOptions(o4);
        reprojection_filter.setInput(*last_stage);
        last_stage = &reprojection_filter;
    }

    /* Enable all filters */
    GrassLidarFilter grass_filter;

    if (base_raster_opt->answer)
        grass_filter.set_base_raster(&base_segment, &input_region,
                                     base_raster_data_type);
    if (use_spatial_filter)
        grass_filter.set_spatial_filter(xmin, xmax, ymin, ymax);
    if (use_zrange)
        grass_filter.set_zrange_filter(zrange_min, zrange_max);
    if (use_irange)
        grass_filter.set_irange_filter(irange_min, irange_max);
    if (use_drange)
        grass_filter.set_drange_filter(drange_min, drange_max);
    if (use_return_filter)
        grass_filter.set_return_filter(return_filter_struct);
    if (use_class_filter)
        grass_filter.set_class_filter(class_filter);
    grass_filter.set_z_scale(zscale); // Default is 1 == no scale
    grass_filter.set_intensity_scale(iscale);
    grass_filter.set_d_scale(dscale);
    grass_filter.setInput(*last_stage);

    GrassRasterWriter binning_writer;

    binning_writer.set_output_scale(output_scale);
    binning_writer.setInput(grass_filter);
    // stream_filter.setInput(*last_stage);
    //  there is no difference between 1 and 10k points in memory
    //  consumption, so using 10k in case it is faster for some cases
    pdal::point_count_t point_table_capacity = 10000;
    pdal::FixedPointTable point_table(point_table_capacity);
    try {
        binning_writer.prepare(point_table);
    }
    catch (const std::exception &err) {
        G_fatal_error(_("PDAL error: %s"), err.what());
    }

    // getting projection is possible only after prepare
    if (over_flag->answer) {
        G_important_message(_("Overriding projection check and assuming"
                              " that the CRS of input matches"
                              " the project's CRS"));
    }
    else if (!reproject_flag->answer) {
        pdal::SpatialReference spatial_reference =
            merge_filter.getSpatialReference();
        if (spatial_reference.empty())
            G_fatal_error(_("The input dataset has undefined projection"));
        std::string dataset_wkt = spatial_reference.getWKT();
        bool proj_match = is_wkt_projection_same_as_loc(dataset_wkt.c_str());

        if (!proj_match)
            wkt_projection_mismatch_report(dataset_wkt.c_str());
    }

    G_important_message(_("Running PDAL algorithms..."));

    // get the layout to see the dimensions
    pdal::PointLayoutPtr point_layout = point_table.layout();

    // TODO: test also z
    // TODO: the falses for filters should be perhaps fatal error
    // (bad input) or warning if filter was requested by the user

    // update layers we are writing based on what is in the data
    // update usage of our filters as well
    if (point_layout->hasDim(pdal::Dimension::Id::ReturnNumber) &&
        point_layout->hasDim(pdal::Dimension::Id::NumberOfReturns)) {
        use_return_filter = true;
    }
    else {
        use_return_filter = false;
    }

    if (point_layout->hasDim(pdal::Dimension::Id::Classification))
        use_class_filter = true;
    else
        use_class_filter = false;

    G_message(_("Scanning points..."));

    if (user_dimension_opt->answer) {
        dim_to_import = point_layout->findDim(user_dimension_opt->answer);
        if (dim_to_import == pdal::Dimension::Id::Unknown)
            G_fatal_error(_("Cannot identify the requested dimension. "
                            "Check dimension name spelling."));
        if (!(strcmp(dimension_opt->answer, "z") == 0))
            G_warning(
                _("Both dimension and user dimension parameters are specified. "
                  "Using '%s' as the dimension to import."),
                user_dimension_opt->answer);
    }

    // this is just for sure, we tested the individual dimensions before
    // TODO: should we test Z explicitly as well?
    if (!point_layout->hasDim(dim_to_import))
        G_fatal_error(_("Dataset doesn't have requested dimension '%s'"
                        " (possibly a programming error)"),
                      pdal::Dimension::name(dim_to_import).c_str());

    // TODO: add percentage printing to one of the filters
    binning_writer.set_binning(&region, &point_binning, &bin_index_nodes, rtype,
                               cols);
    binning_writer.dim_to_import(dim_to_import);
    if (base_raster_opt->answer)
        binning_writer.set_base_raster(&base_segment, &input_region,
                                       base_raster_data_type);
    grass_filter.dim_to_import(dim_to_import);

    // run the actual processing
    binning_writer.execute(point_table);

    /* calc stats and output */
    G_message(_("Writing output raster map..."));
    for (int row = 0; row < rows; row++) {
        /* assemble final values into a row */
        write_values(&point_binning, &bin_index_nodes, raster_row, row, cols,
                     rtype);
        G_percent(row, rows, 10);

        /* write out line of raster data */
        Rast_put_row(out_fd, raster_row, rtype);
    }
    /* free memory */
    point_binning_free(&point_binning, &bin_index_nodes);
    if (base_raster_opt->answer)
        Segment_close(&base_segment);

    G_percent(1, 1, 1); /* flush */
    G_free(raster_row);

    G_message(_(GPOINT_COUNT_FORMAT " points found in input file(s)"),
              grass_filter.num_processed());

    /* close raster file & write history */
    Rast_close(out_fd);

    snprintf(title, sizeof(title),
             "Raw X,Y,Z data binned into a raster grid by cell %s",
             method_opt->answer);

    Rast_put_cell_title(outmap, title);

    Rast_short_history(outmap, "raster", &history);
    Rast_command_history(&history);

    // Hist fields are limited to 4096
    char file_list[4096];

    if (file_list_opt->answer)
        std::snprintf(file_list, sizeof(file_list), "%s",
                      file_list_opt->answer);
    else
        std::snprintf(file_list, sizeof(file_list), "%s", input_opt->answer);

    Rast_set_history(&history, HIST_DATSRC_1, file_list);
    Rast_write_history(outmap, &history);

    /* set computation region to the new raster map */
    /* TODO: should be in the done message */
    if (set_region_flag->answer)
        G_put_window(&region);

    if (infiles.num_items > 1) {
        snprintf(buff, BUFFSIZE,
                 _("Raster map <%s> created."
                   " " GPOINT_COUNT_FORMAT
                   " points from %d files found in region."),
                 outmap, grass_filter.num_passed(), infiles.num_items);
    }
    else {
        snprintf(buff, BUFFSIZE,
                 _("Raster map <%s> created."
                   " " GPOINT_COUNT_FORMAT " points found in region."),
                 outmap, grass_filter.num_passed());
    }

    G_done_msg("%s", buff);
    G_message("Filtered spatially " GPOINT_COUNT_FORMAT " points.",
              grass_filter.num_spatially_filtered());
    G_message("Filtered z range " GPOINT_COUNT_FORMAT " points.",
              grass_filter.num_zrange_filtered());
    G_message("Filtered i range " GPOINT_COUNT_FORMAT " points.",
              grass_filter.num_irange_filtered());
    G_message("Filtered d range " GPOINT_COUNT_FORMAT " points.",
              grass_filter.num_drange_filtered());
    G_message("Filtered class " GPOINT_COUNT_FORMAT " points.",
              grass_filter.num_class_filtered());
    G_message("Filtered return " GPOINT_COUNT_FORMAT " points.",
              grass_filter.num_return_filtered());

    G_message("Processed into raster " GPOINT_COUNT_FORMAT " points.",
              binning_writer.n_processed);

    G_debug(1, "Processed " GPOINT_COUNT_FORMAT " points.",
            grass_filter.num_processed());

    string_list_free(&infiles);

    exit(EXIT_SUCCESS);
}
