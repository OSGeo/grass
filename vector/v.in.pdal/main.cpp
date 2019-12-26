/****************************************************************
 *
 * MODULE:       v.in.pdal
 * 
 * AUTHOR(S):    Vaclav Petras
 *               
 * PURPOSE:      Import LiDAR LAS points using PDAL
 *               
 * COPYRIGHT:    (C) 2015-2018 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/

#include <pdal/PointTable.hpp>
#include <pdal/PointLayout.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/io/LasReader.hpp>
#include <pdal/io/LasHeader.hpp>
#include <pdal/Options.hpp>
#include <pdal/filters/ReprojectionFilter.hpp>
#include <pdal/filters/StreamCallbackFilter.hpp>

extern "C"
{
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
}

extern "C"
{
#include "lidar.h"
#include "projection.h"
#include "filters.h"
}

#ifdef HAVE_LONG_LONG_INT
typedef unsigned long long gpoint_count;
#else
typedef unsigned long gpoint_count;
#endif

/* this is plain C but in sync with v.in.lidar */
static void check_layers_not_equal(int primary, int secondary,
                                   const char *primary_name,
                                   const char *secondary_name)
{
    if (primary && primary == secondary)
        G_fatal_error(_("Values of %s and %s are the same."
                        " All categories would be stored only"
                        " in layer number <%d>"), primary_name,
                      secondary_name, primary);
}

static void check_layers_in_list_not_equal(struct Option **options,
                                           int *values, size_t size)
{
    size_t layer_index_1, layer_index_2;
    for (layer_index_1 = 0; layer_index_1 < size; layer_index_1++) {
        for (layer_index_2 = 0; layer_index_2 < size; layer_index_2++) {
            if (layer_index_1 != layer_index_2) {
                check_layers_not_equal(values[layer_index_1],
                                       values[layer_index_2],
                                       options[layer_index_1]->key,
                                       options[layer_index_2]->key);
            }
        }
    }
}

void pdal_point_to_grass(struct Map_info *output_vector,
                         struct line_pnts *points, struct line_cats *cats,
                         pdal::PointRef& point,
                         struct GLidarLayers *layers, int cat,
                         pdal::Dimension::Id dim_to_use_as_z)
{
    Vect_reset_line(points);
    Vect_reset_cats(cats);

    using namespace pdal::Dimension;
    double x = point.getFieldAs<double>(Id::X);
    double y = point.getFieldAs<double>(Id::Y);
    double z = point.getFieldAs<double>(dim_to_use_as_z);

    /* TODO: optimize for case with no layers, by adding
     * and if to skip all the other ifs */
    if (layers->id_layer) {
        Vect_cat_set(cats, layers->id_layer, cat);
    }
    if (layers->return_layer) {
        int return_n = point.getFieldAs<int>(Id::ReturnNumber);
        int n_returns = point.getFieldAs<int>(Id::NumberOfReturns);
        int return_c = return_to_cat(return_n, n_returns);
        Vect_cat_set(cats, layers->return_layer, return_c);
    }
    if (layers->class_layer) {
        Vect_cat_set(cats, layers->class_layer,
                     point.getFieldAs<int>(Id::Classification));
    }
    if (layers->rgb_layer) {
        int red = point.getFieldAs<int>(Id::Red);
        int green = point.getFieldAs<int>(Id::Green);
        int blue = point.getFieldAs<int>(Id::Blue);
        int rgb = red;
        rgb = (rgb << 8) + green;
        rgb = (rgb << 8) + blue;
        rgb++;  /* cat 0 is not valid, add one */
        Vect_cat_set(cats, layers->rgb_layer, rgb);
    }

    Vect_append_point(points, x, y, z);
    Vect_write_line(output_vector, GV_POINT, points, cats);
}

int main(int argc, char *argv[])
{
    G_gisinit(argv[0]);

    GModule *module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("import"));
    G_add_keyword(_("LIDAR"));
    module->description =
        _("Converts LAS LiDAR point clouds to a GRASS vector map with PDAL.");

    Option *in_opt = G_define_standard_option(G_OPT_F_INPUT);
    in_opt->label = _("LAS input file");
    in_opt->description =
        _("LiDAR input files in LAS format (*.las or *.laz)");

    Option *out_opt = G_define_standard_option(G_OPT_V_OUTPUT);

    Option *id_layer_opt = G_define_standard_option(G_OPT_V_FIELD);
    id_layer_opt->key = "id_layer";
    id_layer_opt->label = _("Layer number to store generated point ID as category");
    id_layer_opt->description = _("Set to 1 by default, use -c to not store it");
    id_layer_opt->answer = NULL;
    id_layer_opt->guisection = _("Categories");

    Option *return_layer_opt = G_define_standard_option(G_OPT_V_FIELD);
    return_layer_opt->key = "return_layer";
    return_layer_opt->label =
        _("Layer number to store return information as category");
    return_layer_opt->description = _("Leave empty to not store it");
    return_layer_opt->answer = NULL;
    return_layer_opt->guisection = _("Categories");

    Option *class_layer_opt = G_define_standard_option(G_OPT_V_FIELD);
    class_layer_opt->key = "class_layer";
    class_layer_opt->label =
        _("Layer number to store class number as category");
    class_layer_opt->description = _("Leave empty to not store it");
    class_layer_opt->answer = NULL;
    class_layer_opt->guisection = _("Categories");

    Option *rgb_layer_opt = G_define_standard_option(G_OPT_V_FIELD);
    rgb_layer_opt->key = "rgb_layer";
    rgb_layer_opt->label =
        _("Layer number where RBG colors are stored as category");
    rgb_layer_opt->description = _("Leave empty to not store it");
    rgb_layer_opt->answer = NULL;
    rgb_layer_opt->guisection = _("Categories");

    Option *spatial_opt = G_define_option();
    spatial_opt->key = "spatial";
    spatial_opt->type = TYPE_DOUBLE;
    spatial_opt->multiple = NO;
    spatial_opt->required = NO;
    // TODO: does this require multiple or not?
    spatial_opt->key_desc = "xmin,ymin,xmax,ymax";
    spatial_opt->label = _("Import subregion only");
    spatial_opt->description =
        _("Format: xmin,ymin,xmax,ymax - usually W,S,E,N");
    spatial_opt->guisection = _("Selection");

    Option *zrange_opt = G_define_option();
    zrange_opt->key = "zrange";
    zrange_opt->type = TYPE_DOUBLE;
    zrange_opt->required = NO;
    zrange_opt->key_desc = "min,max";
    zrange_opt->description = _("Filter range for z data (min,max)");
    zrange_opt->guisection = _("Selection");

    Option *filter_opt = G_define_option();
    filter_opt->key = "return_filter";
    filter_opt->type = TYPE_STRING;
    filter_opt->required = NO;
    filter_opt->label = _("Only import points of selected return type");
    filter_opt->description = _("If not specified, all points are imported");
    filter_opt->options = "first,last,mid";
    filter_opt->guisection = _("Selection");

    Option *class_opt = G_define_option();
    class_opt->key = "class_filter";
    class_opt->type = TYPE_INTEGER;
    class_opt->multiple = YES;
    class_opt->required = NO;
    class_opt->label = _("Only import points of selected class(es)");
    class_opt->description = _("Input is comma separated integers. "
                               "If not specified, all points are imported.");
    class_opt->guisection = _("Selection");

    Flag *reproject_flag = G_define_flag();
    reproject_flag->key = 'w';
    reproject_flag->label =
        _("Reproject to location's coordinate system if needed");
    reproject_flag->description =
        _("Reprojects input dataset to the coordinate system of"
          " the GRASS location (by default only datasets with the"
          " matching cordinate system can be imported");
    reproject_flag->guisection = _("Projection");

    Flag *over_flag = G_define_flag();
    over_flag->key = 'o';
    over_flag->label =
        _("Override projection check (use current location's projection)");
    over_flag->description =
        _("Assume that the dataset has same projection as the current location");
    over_flag->guisection = _("Projection");

    // TODO: from the API it seems that also prj file path and proj string will work
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

    Option *max_ground_window_opt = G_define_option();
    max_ground_window_opt->key = "max_ground_window_size";
    max_ground_window_opt->type = TYPE_DOUBLE;
    max_ground_window_opt->required = NO;
    max_ground_window_opt->answer = "33";
    max_ground_window_opt->description =
        _("Maximum window size for ground filter");
    max_ground_window_opt->guisection = _("Ground filter");

    Option *ground_slope_opt = G_define_option();
    ground_slope_opt->key = "ground_slope";
    ground_slope_opt->type = TYPE_DOUBLE;
    ground_slope_opt->required = NO;
    ground_slope_opt->answer = "1.0";
    ground_slope_opt->description = _("Slope for ground filter");
    ground_slope_opt->guisection = _("Ground filter");

    Option *max_ground_distance_opt = G_define_option();
    max_ground_distance_opt->key = "max_ground_distance";
    max_ground_distance_opt->type = TYPE_DOUBLE;
    max_ground_distance_opt->required = NO;
    max_ground_distance_opt->answer = "2.5";
    max_ground_distance_opt->description =
        _("Maximum distance for ground filter");
    max_ground_distance_opt->guisection = _("Ground filter");

    Option *init_ground_distance_opt = G_define_option();
    init_ground_distance_opt->key = "initial_ground_distance";
    init_ground_distance_opt->type = TYPE_DOUBLE;
    init_ground_distance_opt->required = NO;
    init_ground_distance_opt->answer = "0.15";
    init_ground_distance_opt->description =
        _("Initial distance for ground filter");
    init_ground_distance_opt->guisection = _("Ground filter");

    Option *ground_cell_size_opt = G_define_option();
    ground_cell_size_opt->key = "ground_cell_size";
    ground_cell_size_opt->type = TYPE_DOUBLE;
    ground_cell_size_opt->required = NO;
    ground_cell_size_opt->answer = "1";
    ground_cell_size_opt->description =
        _("Initial distance for ground filter");
    ground_cell_size_opt->guisection = _("Ground filter");

    Flag *nocats_flag = G_define_flag();
    nocats_flag->key = 'c';
    nocats_flag->label =
        _("Do not automatically add unique ID as category to each point");
    nocats_flag->description =
        _("Create only requested layers and categories");
    /* v.in.lidar has this in Speed but we don't have it here */
    nocats_flag->guisection = _("Categories");

    Flag *region_flag = G_define_flag();
    region_flag->key = 'r';
    region_flag->guisection = _("Selection");
    region_flag->description = _("Limit import to the current region");

    Flag *extract_ground_flag = G_define_flag();
    extract_ground_flag->key = 'j';
    extract_ground_flag->label =
        _("Classify and extract ground points");
    extract_ground_flag->description =
        _("This assignes class 2 to the groud points");
    extract_ground_flag->guisection = _("Ground filter");

    // TODO: by inverting class filter and choosing 2 we can select non-groud points
    // this can be done as a separate flag (generally useful?)
    // or this flag can be changed (only ground is classified anyway)
    // and it would Classify ground and extract non-ground
    // probably better if only one step is required to get ground and non-ground
    Flag *classify_ground_flag = G_define_flag();
    classify_ground_flag->key = 'k';
    classify_ground_flag->description = _("Classify ground points");
    classify_ground_flag->guisection = _("Ground filter");

    Flag *height_filter_flag = G_define_flag();
    height_filter_flag->key = 'h';
    height_filter_flag->label =
        _("Compute height for points as a difference from ground");
    height_filter_flag->description =
        _("This requires points to have class 2");
    height_filter_flag->guisection = _("Transform");

    Flag *approx_ground_flag = G_define_flag();
    approx_ground_flag->key = 'm';
    approx_ground_flag->description =
        _("Use approximate algorithm in ground filter");
    approx_ground_flag->guisection = _("Ground filter");

    G_option_exclusive(spatial_opt, region_flag, NULL);
    G_option_exclusive(reproject_flag, over_flag, NULL);
    G_option_exclusive(extract_ground_flag, classify_ground_flag, NULL);
    G_option_exclusive(nocats_flag, id_layer_opt, NULL);
    G_option_requires(return_layer_opt, id_layer_opt, nocats_flag, NULL);
    G_option_requires(class_layer_opt, id_layer_opt, nocats_flag, NULL);
    G_option_requires(rgb_layer_opt, id_layer_opt, nocats_flag, NULL);

    if (G_parser(argc, argv))
        return EXIT_FAILURE;

    if (access(in_opt->answer, F_OK) != 0) {
        G_fatal_error(_("Input file <%s> does not exist"), in_opt->answer);
    }

    // we use full qualification because the dim ns contains too general names
    pdal::Dimension::Id dim_to_use_as_z = pdal::Dimension::Id::Z;

    struct GLidarLayers layers;
    GLidarLayers_set_no_layers(&layers);
    layers.id_layer = 1;
    if (id_layer_opt->answer && id_layer_opt->answer[0] != '\0')
        layers.id_layer = std::stoi(id_layer_opt->answer);
    if (return_layer_opt->answer && return_layer_opt->answer[0] != '\0')
        layers.return_layer = std::stoi(return_layer_opt->answer);
    if (class_layer_opt->answer && class_layer_opt->answer[0] != '\0')
        layers.class_layer = std::stoi(class_layer_opt->answer);
    if (rgb_layer_opt->answer && rgb_layer_opt->answer[0] != '\0')
        layers.rgb_layer = std::stoi(rgb_layer_opt->answer);

    if (nocats_flag->answer) {
        layers.id_layer = 0;
    }

    /* this is plain C but in sync with v.in.lidar */
    Option *layer_options[4] = {id_layer_opt, return_layer_opt,
                                class_layer_opt, rgb_layer_opt};
    int layer_values[4] = {layers.id_layer, layers.return_layer,
                           layers.class_layer, layers.rgb_layer};
    check_layers_in_list_not_equal(layer_options, layer_values, 4);

    if (layers.id_layer)
        G_verbose_message(_("Storing generated point IDs as categories"
                            " in the layer <%d>, consequently no more"
                            " than %d points can be imported"),
                          layers.id_layer, GV_CAT_MAX);

    double xmin = 0;
    double ymin = 0;
    double xmax = 0;
    double ymax = 0;
    bool use_spatial_filter = false;
    if (spatial_opt->answer)
        use_spatial_filter = spatial_filter_from_option(spatial_opt,
                                                        &xmin, &ymin,
                                                        &xmax, &ymax);
    else if (region_flag->answer)
        use_spatial_filter = spatial_filter_from_current_region(&xmin,
                                                                &ymin,
                                                                &xmax,
                                                                &ymax);

    double zrange_min, zrange_max;
    bool use_zrange = zrange_filter_from_option(zrange_opt, &zrange_min,
                                                &zrange_max);
    struct ReturnFilter return_filter_struct;
    bool use_return_filter =
        return_filter_create_from_string(&return_filter_struct,
                                         filter_opt->answer);
    struct ClassFilter class_filter;
    bool use_class_filter =
        class_filter_create_from_strings(&class_filter, class_opt->answers);

    pdal::StageFactory factory;
    std::string pdal_read_driver = factory.inferReaderDriver(in_opt->answer);
    if (pdal_read_driver.empty())
        G_fatal_error("Cannot determine input file type of <%s>",
                      in_opt->answer);

    pdal::Options las_opts;
    pdal::Option las_opt("filename", in_opt->answer);
    las_opts.add(las_opt);
    // if storing of cat is requested, limit the reader count
    pdal::Option count_opt("count", GV_CAT_MAX);
    if (layers.id_layer)
        las_opts.add(count_opt);
    // TODO: free reader
    // using plain pointer because we need to keep the last stage pointer
    pdal::Stage * reader = factory.createStage(pdal_read_driver);
    if (!reader)
        G_fatal_error("PDAL reader creation failed, a wrong format of <%s>",
                      in_opt->answer);
    reader->setOptions(las_opts);

    pdal::Stage * last_stage = reader;
    pdal::ReprojectionFilter reprojection_filter;

    // we reproject when requested regardless the input projection
    if (reproject_flag->answer) {
        G_message(_("Reprojecting the input to the location projection"));
        char *proj_wkt = location_projection_as_wkt(false);
        pdal::Options o4;
        // TODO: try catch for user input error
        if (input_srs_opt->answer)
            o4.add<std::string>("in_srs", input_srs_opt->answer);
        o4.add<std::string>("out_srs", proj_wkt);
        reprojection_filter.setOptions(o4);
        reprojection_filter.setInput(*reader);
        last_stage = &reprojection_filter;
    }

    if (extract_ground_flag->answer || classify_ground_flag->answer) {
        if (extract_ground_flag->answer)
            G_message(_("Extracting ground points"));
        if (classify_ground_flag->answer)
            G_message(_("Classifying ground points"));
        pdal::Options groundOptions;
        groundOptions.add<double>("max_window_size",
                                  atof(max_ground_window_opt->answer));
        groundOptions.add<double>("slope",
                                  atof(ground_slope_opt->answer));
        groundOptions.add<double>("max_distance",
                                  atof(max_ground_distance_opt->answer));
        groundOptions.add<double>("initial_distance",
                                  atof(init_ground_distance_opt->answer));
        groundOptions.add<double>("cell_size",
                                  atof(ground_cell_size_opt->answer));
        groundOptions.add<bool>("classify",
                                classify_ground_flag->answer);
        groundOptions.add<bool>("extract",
                                extract_ground_flag->answer);
        groundOptions.add<bool>("approximate",
                                approx_ground_flag->answer);
        groundOptions.add<bool>("debug", false);
        groundOptions.add<uint32_t>("verbose", 0);

        // TODO: free this or change pointer type to shared
        pdal::Stage * ground_stage(factory.createStage("filters.ground"));
        if (!ground_stage)
            G_fatal_error(_("Ground filter is not available"
                            " (PDAL probably compiled without PCL)"));
        ground_stage->setOptions(groundOptions);
        ground_stage->setInput(*last_stage);
        last_stage = ground_stage;
    }

    if (height_filter_flag->answer) {
        // TODO: we should test with if (point_layout->hasDim(Id::Classification))
        // but we don't have the info yet
        // TODO: free this or change pointer type to shared
        pdal::Stage * height_stage(factory.createStage("filters.height"));
        if (!height_stage)
            G_fatal_error(_("Height above ground filter is not available"
                            " (PDAL probably compiled without PCL)"));
        height_stage->setInput(*last_stage);
        last_stage = height_stage;
    }

    pdal::StreamCallbackFilter stream_filter;
    stream_filter.setInput(*last_stage);
    // there is no difference between 1 and 10k points in memory
    // consumption, so using 10k in case it is faster for some cases
    pdal::point_count_t point_table_capacity = 10000;
    pdal::FixedPointTable point_table(point_table_capacity);
    stream_filter.prepare(point_table);

    // getting projection is possible only after prepare
    if (over_flag->answer) {
        G_important_message(_("Overriding projection check and assuming"
                              " that the projection of input matches"
                              " the location projection"));
    }
    else if (!reproject_flag->answer) {
        pdal::SpatialReference spatial_reference = reader->getSpatialReference();
        if (spatial_reference.empty())
            G_fatal_error(_("The input dataset has undefined projection"));
        std::string dataset_wkt =
            spatial_reference.
            getWKT();
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
        if (layers.return_layer) {
            layers.return_layer = 0;
            G_warning(_("Cannot store return information because the"
                        " input does not have a return dimensions"));
        }
        use_return_filter = false;
    }

    if (point_layout->hasDim(pdal::Dimension::Id::Classification)) {
        use_class_filter = true;
    }
    else {
        if (layers.class_layer) {
            layers.class_layer = 0;
            G_warning(_("Cannot store class because the input"
                        " does not have a classification dimension"));
        }
        use_class_filter = false;
    }

    if (!(point_layout->hasDim(pdal::Dimension::Id::Red) &&
          point_layout->hasDim(pdal::Dimension::Id::Green) &&
          point_layout->hasDim(pdal::Dimension::Id::Blue))) {
        if (layers.rgb_layer) {
            layers.rgb_layer = 0;
            G_warning(_("Cannot store RGB colors because the input"
                        " does not have a RGB dimensions"));
        }
    }

    G_important_message(_("Scanning points..."));
    struct Map_info output_vector;

    // the overwrite warning comes quite late in the execution
    // but that's good enough
    if (Vect_open_new(&output_vector, out_opt->answer, 1) < 0)
        G_fatal_error(_("Unable to create vector map <%s>"), out_opt->answer);
    Vect_hist_command(&output_vector);

    // height is stored as a new attribute
    if (height_filter_flag->answer) {
        // TODO: This needs to be reviewed on Height vs Elevation
        dim_to_use_as_z = point_layout->findDim("Height");
        if (dim_to_use_as_z == pdal::Dimension::Id::Unknown)
            G_fatal_error(_("Cannot identify the height dimension"
                            " (probably something changed in PDAL)"));
    }

    // this is just for sure, we test the individual dimensions before
    // TODO: should we test Z explicitly as well?
    if (!point_layout->hasDim(dim_to_use_as_z))
        G_fatal_error(_("Dataset doesn't have requested dimension '%s'"
                        " with ID %d (possibly a programming error)"),
                      pdal::Dimension::name(dim_to_use_as_z).c_str(),
                      dim_to_use_as_z);

    struct line_pnts *points = Vect_new_line_struct();
    struct line_cats *cats = Vect_new_cats_struct();

    gpoint_count n_outside = 0;
    gpoint_count zrange_filtered = 0;
    gpoint_count n_filtered = 0;
    gpoint_count n_class_filtered = 0;

    int cat = 1;
    bool cat_max_reached = false;

    // define callback
    // Capture all values for reading by value, except for the ones
    // for writing which we capture by reference.
    // False is the proper value to return as a PDAL filter when
    // point is filtered out (and should not be used by next stage).
    // Here the return value does not matter unless we split this into
    // two or more stages.
    auto cb = [=, &cat, &cat_max_reached,
               &n_outside, &zrange_filtered, &n_filtered,
               &n_class_filtered, &class_filter, &return_filter_struct,
               &output_vector, &layers](pdal::PointRef& point) -> bool
    {
        // TODO: avoid duplication of reading the attributes here and when writing if needed
        double x = point.getFieldAs<double>(pdal::Dimension::Id::X);
        double y = point.getFieldAs<double>(pdal::Dimension::Id::Y);
        double z = point.getFieldAs<double>(dim_to_use_as_z);

        if (use_spatial_filter) {
            if (x < xmin || x > xmax || y < ymin || y > ymax) {
                n_outside++;
                return false;
            }
        }
        if (use_zrange) {
            if (z < zrange_min || z > zrange_max) {
                zrange_filtered++;
                return false;
            }
        }
        if (use_return_filter) {
            int return_n =
                point.getFieldAs<int>(pdal::Dimension::Id::ReturnNumber);
            int n_returns =
                point.getFieldAs<int>(pdal::Dimension::Id::NumberOfReturns);
            if (return_filter_is_out
                (&return_filter_struct, return_n, n_returns)) {
                n_filtered++;
                return false;
            }
        }
        if (use_class_filter) {
            int point_class =
                point.getFieldAs<int>(pdal::Dimension::Id::Classification);
            if (class_filter_is_out(&class_filter, point_class)) {
                n_class_filtered++;
                return false;
            }
        }
        pdal_point_to_grass(&output_vector, points, cats, point,
                            &layers, cat, dim_to_use_as_z);
        if (layers.id_layer) {
            // we limit the count of imported points, so we don't
            // need to check if we reached GV_CAT_MAX
            cat++;
        }
        return true;
    };

    // set the callback and run the actual processing
    stream_filter.setCallback(cb);
    stream_filter.execute(point_table);
    
    // not building topology by default
    Vect_close(&output_vector);
}
