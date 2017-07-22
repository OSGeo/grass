
/****************************************************************
 *
 * MODULE:       v.in.lidar
 *
 * AUTHOR(S):    Markus Metz
 *               Vaclav Petras (decimation, cats, areas, zrange)
 *               based on v.in.ogr
 *
 * PURPOSE:      Import LiDAR LAS points
 *
 * COPYRIGHT:    (C) 2011-2015 by the GRASS Development Team
 *
 *               This program is free software under the
 *               GNU General Public License (>=v2).
 *               Read the file COPYING that comes with GRASS
 *               for details.
**************************************************************/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include <liblas/capi/liblas.h>

#include "count_decimation.h"
#include "projection.h"
#include "lidar.h"
#include "attributes.h"
#include "info.h"
#include "vector_mask.h"
#include "filters.h"

#ifndef MAX
#  define MIN(a,b)      ((a<b) ? a : b)
#  define MAX(a,b)      ((a>b) ? a : b)
#endif

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


int main(int argc, char *argv[])
{
    int i;
    float xmin = 0., ymin = 0., xmax = 0., ymax = 0.;
    struct GModule *module;
    struct Option *in_opt, *out_opt, *spat_opt, *filter_opt, *class_opt;
    struct Option *id_layer_opt;
    struct Option *return_layer_opt;
    struct Option *class_layer_opt;
    struct Option *rgb_layer_opt;
    struct Option *vector_mask_opt, *vector_mask_field_opt;
    struct Option *skip_opt, *preserve_opt, *offset_opt, *limit_opt;
    struct Option *outloc_opt, *zrange_opt;
    struct Flag *print_flag, *notab_flag, *region_flag, *notopo_flag;
    struct Flag *nocats_flag;
    struct Flag *over_flag, *extend_flag, *no_import_flag;
    struct Flag *invert_mask_flag;
    struct Flag *only_valid_flag;
    char buf[2000];
    struct Key_Value *loc_proj_info = NULL, *loc_proj_units = NULL;
    struct Key_Value *proj_info, *proj_units;
    const char *projstr;
    struct Cell_head cellhd, loc_wind, cur_wind;
    char error_msg[8192];

    /* Vector */
    struct Map_info Map;
    int cat;

    /* Attributes */
    struct field_info *Fi;
    dbDriver *driver;
    
    /* LAS */
    LASReaderH LAS_reader;
    LASHeaderH LAS_header;
    LASSRSH LAS_srs;
    LASPointH LAS_point;
    double scale_x, scale_y, scale_z, offset_x, offset_y, offset_z;
    int las_point_format;
    int have_time, have_color;
    int point_class;

    struct line_pnts *Points;
    struct line_cats *Cats;

    int cat_max_reached = FALSE;

#ifdef HAVE_LONG_LONG_INT
    unsigned long long n_features; /* what libLAS reports as point count */
    unsigned long long points_imported; /* counter how much we have imported */
    unsigned long long feature_count, n_outside, zrange_filtered,
        n_outside_mask, n_filtered, n_class_filtered, not_valid;
#else
    unsigned long n_features;
    unsigned long points_imported;
    unsigned long feature_count, n_outside, zrange_filtered,
        n_outside_mask, n_filtered, n_class_filtered, not_valid;
#endif

    int overwrite;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("import"));
    G_add_keyword(_("LIDAR"));
    G_add_keyword(_("level1"));

    module->description = _("Converts LAS LiDAR point clouds to a GRASS vector map with libLAS.");

    in_opt = G_define_standard_option(G_OPT_F_BIN_INPUT);
    in_opt->label = _("LAS input file");
    in_opt->description = _("LiDAR input files in LAS format (*.las or *.laz)");

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);

    id_layer_opt = G_define_standard_option(G_OPT_V_FIELD);
    id_layer_opt->key = "id_layer";
    id_layer_opt->label = _("Layer number to store generated point ID as category");
    id_layer_opt->description = _("Set to 1 by default, use -c to not store it");
    id_layer_opt->answer = NULL;
    id_layer_opt->guisection = _("Categories");

    return_layer_opt = G_define_standard_option(G_OPT_V_FIELD);
    return_layer_opt->key = "return_layer";
    return_layer_opt->label =
        _("Layer number to store return information as category");
    return_layer_opt->description = _("Leave empty to not store it");
    return_layer_opt->answer = NULL;
    return_layer_opt->guisection = _("Categories");

    class_layer_opt = G_define_standard_option(G_OPT_V_FIELD);
    class_layer_opt->key = "class_layer";
    class_layer_opt->label =
        _("Layer number to store class number as category");
    class_layer_opt->description = _("Leave empty to not store it");
    class_layer_opt->answer = NULL;
    class_layer_opt->guisection = _("Categories");

    rgb_layer_opt = G_define_standard_option(G_OPT_V_FIELD);
    rgb_layer_opt->key = "rgb_layer";
    rgb_layer_opt->label =
        _("Layer number where RBG colors are stored as category");
    rgb_layer_opt->description = _("Leave empty to not store it");
    rgb_layer_opt->answer = NULL;
    rgb_layer_opt->guisection = _("Categories");

    spat_opt = G_define_option();
    spat_opt->key = "spatial";
    spat_opt->type = TYPE_DOUBLE;
    spat_opt->multiple = YES;
    spat_opt->required = NO;
    spat_opt->key_desc = "xmin,ymin,xmax,ymax";
    spat_opt->label = _("Import subregion only");
    spat_opt->guisection = _("Selection");
    spat_opt->description =
	_("Format: xmin,ymin,xmax,ymax - usually W,S,E,N");

    zrange_opt = G_define_option();
    zrange_opt->key = "zrange";
    zrange_opt->type = TYPE_DOUBLE;
    zrange_opt->required = NO;
    zrange_opt->key_desc = "min,max";
    zrange_opt->description = _("Filter range for z data (min,max)");
    zrange_opt->guisection = _("Selection");

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

    vector_mask_opt = G_define_standard_option(G_OPT_V_INPUT);
    vector_mask_opt->key = "mask";
    vector_mask_opt->required = NO;
    vector_mask_opt->label = _("Areas where to import points");
    vector_mask_opt->description = _("Name of vector map with areas where the points should be imported");
    vector_mask_opt->guisection = _("Selection");

    vector_mask_field_opt = G_define_standard_option(G_OPT_V_FIELD);
    vector_mask_field_opt->key = "mask_layer";
    vector_mask_field_opt->label = _("Layer number or name for mask option");
    vector_mask_field_opt->guisection = _("Selection");

    skip_opt = G_define_option();
    skip_opt->key = "skip";
    skip_opt->type = TYPE_INTEGER;
    skip_opt->multiple = NO;
    skip_opt->required = NO;
    skip_opt->label = _("Do not import every n-th point");
    skip_opt->description = _("For example, 5 will import 80 percent of points. "
                              "If not specified, all points are imported");
    skip_opt->guisection = _("Decimation");

    preserve_opt = G_define_option();
    preserve_opt->key = "preserve";
    preserve_opt->type = TYPE_INTEGER;
    preserve_opt->multiple = NO;
    preserve_opt->required = NO;
    preserve_opt->label = _("Import only every n-th point");
    preserve_opt->description = _("For example, 4 will import 25 percent of points. "
                                  "If not specified, all points are imported");
    preserve_opt->guisection = _("Decimation");

    offset_opt = G_define_option();
    offset_opt->key = "offset";
    offset_opt->type = TYPE_INTEGER;
    offset_opt->multiple = NO;
    offset_opt->required = NO;
    offset_opt->label = _("Skip first n points");
    offset_opt->description = _("Skips the given number of points at the beginning.");
    offset_opt->guisection = _("Decimation");

    limit_opt = G_define_option();
    limit_opt->key = "limit";
    limit_opt->type = TYPE_INTEGER;
    limit_opt->multiple = NO;
    limit_opt->required = NO;
    limit_opt->label = _("Import only n points");
    limit_opt->description = _("Imports only the given number of points");
    limit_opt->guisection = _("Decimation");

    outloc_opt = G_define_option();
    outloc_opt->key = "location";
    outloc_opt->type = TYPE_STRING;
    outloc_opt->required = NO;
    outloc_opt->description = _("Name for new location to create");
    outloc_opt->key_desc = "name";

    print_flag = G_define_flag();
    print_flag->key = 'p';
    print_flag->description =
	_("Print LAS file info and exit");
    print_flag->suppress_required = YES;

    region_flag = G_define_flag();
    region_flag->key = 'r';
    region_flag->guisection = _("Selection");
    region_flag->description = _("Limit import to the current region");

    invert_mask_flag = G_define_flag();
    invert_mask_flag->key = 'u';
    invert_mask_flag->description = _("Invert mask when selecting points");
    invert_mask_flag->guisection = _("Selection");

    only_valid_flag = G_define_flag();
    only_valid_flag->key = 'v';
    only_valid_flag->label = _("Use only valid points");
    only_valid_flag->description =
        _("Points invalid according to APSRS LAS specification will be"
          " filtered out");
    only_valid_flag->guisection = _("Selection");

    extend_flag = G_define_flag();
    extend_flag->key = 'e';
    extend_flag->description =
        _("Extend region extents based on new dataset");

    notab_flag = G_define_standard_flag(G_FLG_V_TABLE);
    notab_flag->guisection = _("Speed");

    nocats_flag = G_define_flag();
    nocats_flag->key = 'c';
    nocats_flag->label =
        _("Do not automatically add unique ID as category to each point");
    nocats_flag->description =
        _("Create only requested layers and categories");
    nocats_flag->guisection = _("Speed");

    notopo_flag = G_define_standard_flag(G_FLG_V_TOPO);
    notopo_flag->guisection = _("Speed");

    over_flag = G_define_flag();
    over_flag->key = 'o';
    over_flag->label =
        _("Override projection check (use current location's projection)");
    over_flag->description =
        _("Assume that the dataset has same projection as the current location");

    no_import_flag = G_define_flag();
    no_import_flag->key = 'i';
    no_import_flag->description =
	_("Create the location specified by the \"location\" parameter and exit."
          " Do not import the vector data.");
    no_import_flag->suppress_required = YES;

    G_option_exclusive(skip_opt, preserve_opt, NULL);
    G_option_requires(nocats_flag, notab_flag, NULL);
    G_option_exclusive(nocats_flag, id_layer_opt, NULL);
    G_option_requires(return_layer_opt, id_layer_opt, nocats_flag, NULL);
    G_option_requires(class_layer_opt, id_layer_opt, nocats_flag, NULL);
    G_option_requires(rgb_layer_opt, id_layer_opt, nocats_flag, NULL);

    /* The parser checks if the map already exists in current mapset, this is
     * wrong if location options is used, so we switch out the check and do it
     * in the module after the parser */
    overwrite = G_check_overwrite(argc, argv);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Don't crash on cmd line if file not found */
    if (access(in_opt->answer, F_OK) != 0) {
	G_fatal_error(_("Input file <%s> does not exist"), in_opt->answer);
    }
    /* Open LAS file*/
    LAS_reader = LASReader_Create(in_opt->answer);
    if (LAS_reader == NULL)
        G_fatal_error(_("Unable to open file <%s> as a LiDAR point cloud. %s"),
                      in_opt->answer, LASError_GetLastErrorMsg());
    LAS_header = LASReader_GetHeader(LAS_reader);
    if  (LAS_header == NULL) {
        G_fatal_error(_("Unable to read LAS header of <%s>"), in_opt->answer);
    }

    LAS_srs = LASHeader_GetSRS(LAS_header);

    scale_x = LASHeader_GetScaleX(LAS_header);
    scale_y = LASHeader_GetScaleY(LAS_header);
    scale_z = LASHeader_GetScaleZ(LAS_header);

    offset_x = LASHeader_GetOffsetX(LAS_header);
    offset_y = LASHeader_GetOffsetY(LAS_header);
    offset_z = LASHeader_GetOffsetZ(LAS_header);

    xmin = LASHeader_GetMinX(LAS_header);
    xmax = LASHeader_GetMaxX(LAS_header);
    ymin = LASHeader_GetMinY(LAS_header);
    ymax = LASHeader_GetMaxY(LAS_header);

    /* Print LAS header */
    if (print_flag->answer) {
	/* print... */
	print_lasinfo(LAS_header, LAS_srs);

	LASSRS_Destroy(LAS_srs);
	LASHeader_Destroy(LAS_header);
	LASReader_Destroy(LAS_reader);

	exit(EXIT_SUCCESS);
    }

    int only_valid = FALSE;
    if (only_valid_flag->answer)
        only_valid = TRUE;

    struct ReturnFilter return_filter_struct;
    return_filter_create_from_string(&return_filter_struct, filter_opt->answer);
    struct ClassFilter class_filter;
    class_filter_create_from_strings(&class_filter, class_opt->answers);

    int id_layer = 1;
    int return_layer = 0;
    int class_layer = 0;
    int rgb_layer = 0;
    if (id_layer_opt->answer)
        id_layer = atoi(id_layer_opt->answer);
    if (return_layer_opt->answer)
        return_layer = atoi(return_layer_opt->answer);
    if (class_layer_opt->answer)
        class_layer = atoi(class_layer_opt->answer);
    if (rgb_layer_opt->answer)
        rgb_layer = atoi(rgb_layer_opt->answer);

    if (nocats_flag->answer) {
        id_layer = 0;
    }
    /* no cats forces no table earlier */
    if (!notab_flag->answer && !id_layer) {
        G_message(_("-%c flag is not set but ID layer is not specified"), notab_flag->key);
        G_fatal_error(_("ID layer is required to store attribute table"));
    }

    struct Option *layer_options[4] = {id_layer_opt, return_layer_opt,
                                       class_layer_opt, rgb_layer_opt};
    int layer_values[4] = {id_layer, return_layer, class_layer, rgb_layer};
    check_layers_in_list_not_equal(layer_options, layer_values, 4);

    if (id_layer)
        G_verbose_message(_("Storing generated point IDs as categories"
                            " in the layer <%d>, consequently no more"
                            " than %d points can be imported"),
                          id_layer, GV_CAT_MAX);

    double zrange_min, zrange_max;
    int use_zrange = FALSE;

    if (zrange_opt->answer != NULL) {
        if (zrange_opt->answers[0] == NULL || zrange_opt->answers[1] == NULL)
            G_fatal_error(_("Invalid zrange <%s>"), zrange_opt->answer);
        sscanf(zrange_opt->answers[0], "%lf", &zrange_min);
        sscanf(zrange_opt->answers[1], "%lf", &zrange_max);
        /* for convenience, switch order to make valid input */
        if (zrange_min > zrange_max) {
            double tmp = zrange_max;

            zrange_max = zrange_min;
            zrange_min = tmp;
        }
        use_zrange = TRUE;
    }

    if (region_flag->answer) {
	if (spat_opt->answer)
	    G_fatal_error(_("Select either the current region flag or the spatial option, not both"));

	G_get_window(&cur_wind);
	xmin = cur_wind.west;
	xmax = cur_wind.east;
	ymin = cur_wind.south;
	ymax = cur_wind.north;
    }
    if (spat_opt->answer) {
	/* See as reference: gdal/ogr/ogr_capi_test.c */

	/* cut out a piece of the map */
	/* order: xmin,ymin,xmax,ymax */
	int arg_s_num = 0;
	i = 0;
	while (spat_opt->answers[i]) {
	    if (i == 0)
		xmin = atof(spat_opt->answers[i]);
	    if (i == 1)
		ymin = atof(spat_opt->answers[i]);
	    if (i == 2)
		xmax = atof(spat_opt->answers[i]);
	    if (i == 3)
		ymax = atof(spat_opt->answers[i]);
	    arg_s_num++;
	    i++;
	}
	if (arg_s_num != 4)
	    G_fatal_error(_("4 parameters required for 'spatial' parameter"));
    }
    if (spat_opt->answer || region_flag->answer) {
	G_debug(2, "cut out with boundaries: xmin:%f ymin:%f xmax:%f ymax:%f",
		xmin, ymin, xmax, ymax);
    }

    /* fetch boundaries */
    G_get_window(&cellhd);
    cellhd.north = ymax;
    cellhd.south = ymin;
    cellhd.west = xmin;
    cellhd.east = xmax;
    cellhd.rows = 20;	/* TODO - calculate useful values */
    cellhd.cols = 20;
    cellhd.ns_res = (cellhd.north - cellhd.south) / cellhd.rows;
    cellhd.ew_res = (cellhd.east - cellhd.west) / cellhd.cols;

    /* Fetch input map projection in GRASS form. */
    proj_info = NULL;
    proj_units = NULL;
    projstr = LASSRS_GetWKT_CompoundOK(LAS_srs);

    /* Do we need to create a new location? */
    if (outloc_opt->answer != NULL) {
	/* Convert projection information non-interactively as we can't
	 * assume the user has a terminal open */
	if (GPJ_wkt_to_grass(&cellhd, &proj_info,
			     &proj_units, projstr, 0) < 0) {
	    G_fatal_error(_("Unable to convert input map projection to GRASS "
			    "format; cannot create new location."));
	}
	else {
            if (0 != G_make_location(outloc_opt->answer, &cellhd,
                                     proj_info, proj_units)) {
                G_fatal_error(_("Unable to create new location <%s>"),
                              outloc_opt->answer);
            }
	    G_message(_("Location <%s> created"), outloc_opt->answer);
	}

        /* If the i flag is set, clean up? and exit here */
        if(no_import_flag->answer)
            exit(EXIT_SUCCESS);

	/*  TODO: */
	G_warning("Import into new location not yet implemented");
	/* at this point the module should be using G_create_alt_env()
	    to change context to the newly created location; once done
	    it should switch back with G_switch_env(). See r.in.gdal */
    }
    else {
	/* Does the projection of the current location match the dataset? */
	/* G_get_window seems to be unreliable if the location has been changed */
	G_get_default_window(&loc_wind);
    projstr = LASSRS_GetWKT_CompoundOK(LAS_srs);
    /* we are printing the non-warning messages only for first file */
    projection_check_wkt(cellhd, loc_wind, projstr, over_flag->answer,
                         TRUE);
    }

    if (!outloc_opt->answer) {	/* Check if the map exists */
	if (G_find_vector2(out_opt->answer, G_mapset())) {
	    if (overwrite)
		G_warning(_("Vector map <%s> already exists and will be overwritten"),
			  out_opt->answer);
	    else
		G_fatal_error(_("Vector map <%s> already exists"),
			      out_opt->answer);
	}
    }

    /* open output vector */
    sprintf(buf, "%s", out_opt->answer);
    /* strip any @mapset from vector output name */
    G_find_vector(buf, G_mapset());
    if (Vect_open_new(&Map, out_opt->answer, 1) < 0)
	G_fatal_error(_("Unable to create vector map <%s>"), out_opt->answer);

    Vect_hist_command(&Map);

    /* libLAS uses uint32_t according to the source code
     * or unsigned int according to the online doc,
     * so just storing in long doesn't help.
     * Thus, we use this just for the messages and percents.
     */
    n_features = LASHeader_GetPointRecordsCount(LAS_header);
    las_point_format = LASHeader_GetDataFormatId(LAS_header);

    have_time = (las_point_format == 1 || las_point_format == 3 || 
		 las_point_format == 4 || las_point_format == 5);

    have_color = (las_point_format == 2 || las_point_format == 3 || 
		   las_point_format == 5);

    /* Add DB link */
    if (!notab_flag->answer) {
        create_table_for_lidar(&Map, out_opt->answer, id_layer, &driver,
                               &Fi, have_time, have_color);
    }

    struct VectorMask vector_mask;
    if (vector_mask_opt->answer) {
        VectorMask_init(&vector_mask, vector_mask_opt->answer,
                        vector_mask_field_opt->answer, (int)invert_mask_flag->answer);
    }

    /* Import feature */
    points_imported = 0;
    cat = 1;
    not_valid = 0;
    feature_count = 0;
    n_outside = 0;
    n_filtered = 0;
    n_class_filtered = 0;
    n_outside_mask = 0;
    zrange_filtered = 0;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    struct CountDecimationControl count_decimation_control;

    count_decimation_init_from_str(&count_decimation_control,
                                   skip_opt->answer, preserve_opt->answer,
                                   offset_opt->answer, limit_opt->answer);
    if (!count_decimation_is_valid(&count_decimation_control))
        G_fatal_error(_("Settings for count-based decimation are not valid"));
    /* we don't check if the decimation is noop */

#ifdef HAVE_LONG_LONG_INT
    G_important_message(_("Scanning %llu points..."), n_features);
#else
    G_important_message(_("Scanning %lu points..."), n_features);
#endif
    while ((LAS_point = LASReader_GetNextPoint(LAS_reader)) != NULL) {
	double x, y, z;

	G_percent(feature_count++, n_features, 1);	/* show something happens */

        /* We always count them and report because r.in.lidar behavior
         * changed in between 7.0 and 7.2 from undefined (but skipping
         * invalid points) to filtering them out only when requested. */
        if (!LASPoint_IsValid(LAS_point)) {
            not_valid++;
            if (only_valid)
                continue;
        }

	Vect_reset_line(Points);
	Vect_reset_cats(Cats);

	x = LASPoint_GetX(LAS_point);
	y = LASPoint_GetY(LAS_point);
	z = LASPoint_GetZ(LAS_point);

	if (spat_opt->answer || region_flag->answer) {
	    if (x < xmin || x > xmax || y < ymin || y > ymax) {
		n_outside++;
		continue;
	    }
	}
        if (use_zrange) {
            if (z < zrange_min || z > zrange_max) {
                zrange_filtered++;
                continue;
            }
        }
        int return_n = LASPoint_GetReturnNumber(LAS_point);
        int n_returns = LASPoint_GetNumberOfReturns(LAS_point);
        if (return_filter_is_out(&return_filter_struct, return_n, n_returns)) {
            n_filtered++;
            continue;
        }
        point_class = (int) LASPoint_GetClassification(LAS_point);
        if (class_filter_is_out(&class_filter, point_class)) {
            n_class_filtered++;
            continue;
        }
        if (vector_mask_opt->answer) {
            if (!VectorMask_point_in(&vector_mask, x, y)) {
                n_outside_mask++;
                continue;
            }
        }
        if (count_decimation_is_out(&count_decimation_control))
            continue;

	Vect_append_point(Points, x, y, z);
        if (id_layer)
            Vect_cat_set(Cats, id_layer, cat);
        if (return_layer) {
            int return_c = return_to_cat(return_n, n_returns);
            Vect_cat_set(Cats, return_layer, return_c);
        }
        if (class_layer) {
            /* 0 is not a valid category and
             * classes 0 and 1 as practically the same */
            if (point_class == 0)
                Vect_cat_set(Cats, class_layer, 1);
            else
                Vect_cat_set(Cats, class_layer, point_class);
        }
        if (have_color && rgb_layer) {
            /* TODO: if attr table, acquired again, performance difference? */
            LASColorH LAS_color = LASPoint_GetColor(LAS_point);
            if (rgb_layer) {
                int red = LASColor_GetRed(LAS_color);
                int green = LASColor_GetGreen(LAS_color);
                int blue = LASColor_GetBlue(LAS_color);
                int rgb = red;
                rgb = (rgb << 8) + green;
                rgb = (rgb << 8) + blue;
                rgb++;  /* cat 0 is not valid, add one */
                Vect_cat_set(Cats, rgb_layer, rgb);
            }
        }
	Vect_write_line(&Map, GV_POINT, Points, Cats);

	/* Attributes */
	if (!notab_flag->answer) {
        las_point_to_attributes(Fi, driver, cat, LAS_point, x, y, z,
                                have_time, have_color);
	}

        if (count_decimation_is_end(&count_decimation_control))
            break;
        if (id_layer && cat == GV_CAT_MAX) {
            cat_max_reached = TRUE;
            break;
        }
	cat++;
        points_imported++;
    }
    G_percent(n_features, n_features, 1);	/* finish it */

    if (!notab_flag->answer) {
	db_commit_transaction(driver);
	db_close_database_shutdown_driver(driver);
    }
    
    if (vector_mask_opt->answer) {
        VectorMask_destroy(&vector_mask);
    }
    
    LASSRS_Destroy(LAS_srs);
    LASHeader_Destroy(LAS_header);
    LASReader_Destroy(LAS_reader);

    /* close map */
    if (!notopo_flag->answer)
	Vect_build(&Map);
    Vect_close(&Map);

#ifdef HAVE_LONG_LONG_INT
    unsigned long long not_valid_filtered = 0;
#else
    unsigned long not_valid_filtered = 0;
#endif
    if (only_valid)
        not_valid_filtered = not_valid;

    /* can be easily determined only when iterated over all points */
    if (!count_decimation_control.limit_n && !cat_max_reached
            && points_imported != n_features
            - not_valid_filtered - n_outside - n_filtered - n_class_filtered
            - n_outside_mask - count_decimation_control.offset_n_counter
            - count_decimation_control.n_count_filtered - zrange_filtered)
        G_warning(_("The underlying libLAS library is at its limits."
                    " Previously reported counts might have been distorted."
                    " However, the import itself should be unaffected."));

#ifdef HAVE_LONG_LONG_INT
    if (count_decimation_control.limit_n) {
        G_message(_("%llu points imported (limit was %llu)"),
                  count_decimation_control.limit_n_counter,
                  count_decimation_control.limit_n);
    }
    else {
        G_message(_("%llu points imported"), points_imported);
    }
    if (not_valid && only_valid)
        G_message(_("%llu input points were not valid and filtered out"), not_valid);
    if (n_outside)
	G_message(_("%llu input points were outside of the selected area"), n_outside);
    if (n_outside_mask)
        G_message(_("%llu input points were outside of the area specified by mask"), n_outside_mask);
    if (n_filtered)
	G_message(_("%llu input points were filtered out by return number"), n_filtered);
    if (n_class_filtered)
        G_message(_("%llu input points were filtered out by class number"), n_class_filtered);
    if (zrange_filtered)
        G_message(_("%llu input points were filtered outsite the range for z coordinate"), zrange_filtered);
    if (count_decimation_control.offset_n_counter)
        G_message(_("%llu input points were skipped at the begging using offset"),
                  count_decimation_control.offset_n_counter);
    if (count_decimation_control.n_count_filtered)
        G_message(_("%llu input points were skipped by count-based decimation"),
                  count_decimation_control.n_count_filtered);
#else
    if (count_decimation_control.limit_n)
        G_message(_("%lu points imported (limit was %d)"),
                  count_decimation_control.limit_n_counter,
                  count_decimation_control.limit_n);
    else
        G_message(_("%lu points imported"), points_imported);
    if (not_valid && only_valid)
        G_message(_("%lu input points were not valid and filtered out"), not_valid);
    if (n_outside)
	G_message(_("%lu input points were outside of the selected area"), n_outside);
    if (n_outside_mask)
        G_message(_("%lu input points were outside of the area specified by mask"), n_outside_mask);
    if (n_filtered)
	G_message(_("%lu input points were filtered out by return number"), n_filtered);
    if (n_class_filtered)
        G_message(_("%lu input points were filtered out by class number"), n_class_filtered);
    if (zrange_filtered)
        G_message(_("%lu input points were filtered outsite the range for z coordinate"), zrange_filtered);
    if (count_decimation_control.offset_n_counter)
        G_message(_("%lu input points were skipped at the begging using offset"),
                  count_decimation_control.offset_n_counter);
    if (count_decimation_control.n_count_filtered)
        G_message(_("%lu input points were skipped by count-based decimation"),
                  count_decimation_control.n_count_filtered);
    G_message(_("Accuracy of the printed point counts might be limited by your computer architecture."));
#endif
    if (count_decimation_control.limit_n)
        G_message(_("The rest of points was ignored"));

#ifdef HAVE_LONG_LONG_INT
    if (not_valid && !only_valid)
        G_message(_("%llu input points were not valid, use -%c flag to filter"
                    " them out"), not_valid, only_valid_flag->key);
#else
    if (not_valid && !only_valid)
        G_message(_("%lu input points were not valid, use -%c flag to filter"
                    " them out"), not_valid, only_valid_flag->key);
#endif

    if (cat_max_reached)
        G_warning(_("Maximum number of categories reached (%d). Import ended prematurely."
                    " Try to import without using category as an ID."), GV_CAT_MAX);

    /* -------------------------------------------------------------------- */
    /*      Extend current window based on dataset.                         */
    /* -------------------------------------------------------------------- */
    if (extend_flag->answer) {
	G_get_set_window(&loc_wind);

	loc_wind.north = MAX(loc_wind.north, cellhd.north);
	loc_wind.south = MIN(loc_wind.south, cellhd.south);
	loc_wind.west = MIN(loc_wind.west, cellhd.west);
	loc_wind.east = MAX(loc_wind.east, cellhd.east);

	loc_wind.rows = (int)ceil((loc_wind.north - loc_wind.south)
				  / loc_wind.ns_res);
	loc_wind.south = loc_wind.north - loc_wind.rows * loc_wind.ns_res;

	loc_wind.cols = (int)ceil((loc_wind.east - loc_wind.west)
				  / loc_wind.ew_res);
	loc_wind.east = loc_wind.west + loc_wind.cols * loc_wind.ew_res;

	G_put_window(&loc_wind);
    }

    exit(EXIT_SUCCESS);
}
