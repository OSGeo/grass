
/****************************************************************
 *
 * MODULE:       v.in.lidar
 *
 * AUTHOR(S):    Markus Metz
 *               based on v.in.ogr
 *
 * PURPOSE:      Import LiDAR LAS points
 *
 * COPYRIGHT:    (C) 2003 by the GRASS Development Team
 *
 *               This program is free software under the
 *               GNU General Public License (>=v2).
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 * TODO: - make fixed field length of OFTIntegerList dynamic
 *       - several other TODOs below
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

#ifndef MAX
#  define MIN(a,b)      ((a<b) ? a : b)
#  define MAX(a,b)      ((a>b) ? a : b)
#endif

#define LAS_ALL 0
#define LAS_FIRST 1
#define LAS_LAST 2
#define LAS_MID 3

/*
 * ASPRS Standard LIDAR Point Classes
 * Classification Value (bits 0:4) : Meaning
 *      0 : Created, never classified
 *      1 : Unclassified
 *      2 : Ground
 *      3 : Low Vegetation
 *      4 : Medium Vegetation
 *      5 : High Vegetation
 *      6 : Building
 *      7 : Low Point (noise)
 *      8 : Model Key-point (mass point)
 *      9 : Water
 *     10 : Reserved for ASPRS Definition
 *     11 : Reserved for ASPRS Definition
 *     12 : Overlap Points
 *  13-31 : Reserved for ASPRS Definition
 */

/* Classification Bit Field Encoding
 * Bits | Field Name     | Description
 *  0-4 | Classification | Standard ASPRS classification as defined in the
 *                         above classification table.
 *    5 | Synthetic      | If set then this point was created by a technique
 *                         other than LIDAR collection such as digitized from
 *	                   a photogrammetric stereo model or by traversing
 *                         a waveform.
 *    6 | Key-point      | If set, this point is considered to be a model 
 *                         key-point and thus generally should not be withheld
 *                         in a thinning algorithm.
 *    7 | Withheld       | If set, this point should not be included in
 *                         processing (synonymous with Deleted).
*/

struct class_table
{
    int code;
    char *name;
};

static struct class_table class_val[] = {
    {0, "Created, never classified"},
    {1, "Unclassified"},
    {2, "Ground"},
    {3, "Low Vegetation"},
    {4, "Medium Vegetation"},
    {5, "High Vegetation"},
    {6, "Building"},
    {7, "Low Point (noise)"},
    {8, "Model Key-point (mass point)"},
    {9, "Water"},
    {10, "Reserved for ASPRS Definition"},
    {11, "Reserved for ASPRS Definition"},
    {12, "Overlap Points"},
    {13 /* 13 - 31 */, "Reserved for ASPRS Definition"},
    {0, 0}
};

static struct class_table class_type[] = {
    {5, "Synthetic"},
    {6, "Key-point"},
    {7, "Withheld"},
    {0, 0}
};

void print_lasinfo(LASHeaderH LAS_header, LASSRSH LAS_srs);

int main(int argc, char *argv[])
{
    int i;
    float xmin = 0., ymin = 0., xmax = 0., ymax = 0.;
    struct GModule *module;
    struct Option *in_opt, *out_opt, *spat_opt, *filter_opt;
    struct Option *outloc_opt;
    struct Flag *print_flag, *notab_flag, *region_flag, *notopo_flag;
    struct Flag *over_flag, *extend_flag, *no_import_flag;
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
    dbString sql, strval;
    
    /* LAS */
    LASReaderH LAS_reader;
    LASHeaderH LAS_header;
    LASSRSH LAS_srs;
    LASPointH LAS_point;
    double scale_x, scale_y, scale_z, offset_x, offset_y, offset_z;
    int las_point_format;
    int have_time, have_color;
    int return_filter;
    unsigned int not_valid;	

    struct line_pnts *Points;
    struct line_cats *Cats;

    unsigned int n_features, feature_count, n_outside, n_filtered;
    int overwrite;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("import"));
    G_add_keyword(_("LIDAR"));
    module->description = _("Converts LAS LiDAR point clouds to a GRASS vector map with libLAS.");

    in_opt = G_define_standard_option(G_OPT_F_INPUT);
    in_opt->label = _("LAS input file");
    in_opt->description = _("LiDAR input files in LAS format (*.las or *.laz)");

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    
    spat_opt = G_define_option();
    spat_opt->key = "spatial";
    spat_opt->type = TYPE_DOUBLE;
    spat_opt->multiple = YES;
    spat_opt->required = NO;
    spat_opt->key_desc = "xmin,ymin,xmax,ymax";
    spat_opt->label = _("Import subregion only");
    spat_opt->guisection = _("Subregion");
    spat_opt->description =
	_("Format: xmin,ymin,xmax,ymax - usually W,S,E,N");

    outloc_opt = G_define_option();
    outloc_opt->key = "location";
    outloc_opt->type = TYPE_STRING;
    outloc_opt->required = NO;
    outloc_opt->description = _("Name for new location to create");
    outloc_opt->key_desc = "name";
    
    filter_opt = G_define_option();
    filter_opt->key = "filter";
    filter_opt->type = TYPE_STRING;
    filter_opt->required = NO;
    filter_opt->label = _("Only import points of selected return type");
    filter_opt->description = _("If not specified, all points are imported");
    filter_opt->options = "first,last,mid";

    print_flag = G_define_flag();
    print_flag->key = 'p';
    print_flag->description =
	_("Print LAS file info and exit");
    print_flag->suppress_required = YES;
    
    notab_flag = G_define_standard_flag(G_FLG_V_TABLE);
    notab_flag->guisection = _("Attributes");

    over_flag = G_define_flag();
    over_flag->key = 'o';
    over_flag->description =
	_("Override dataset projection (use location's projection)");

    region_flag = G_define_flag();
    region_flag->key = 'r';
    region_flag->guisection = _("Subregion");
    region_flag->description = _("Limit import to the current region");

    extend_flag = G_define_flag();
    extend_flag->key = 'e';
    extend_flag->description =
	_("Extend region extents based on new dataset");

    no_import_flag = G_define_flag();
    no_import_flag->key = 'i';
    no_import_flag->description =
	_("Create the location specified by the \"location\" parameter and exit."
          " Do not import the vector file.");
    no_import_flag->suppress_required = YES;

    notopo_flag = G_define_standard_flag(G_FLG_V_TOPO);

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
    LAS_header = LASReader_GetHeader(LAS_reader);

    if  (LAS_header == NULL) {
	G_fatal_error(_("Input file <%s> is not a LAS LiDAR point cloud"),
	                in_opt->answer);
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
	/* at this point the module should be using G__create_alt_env()
	    to change context to the newly created location; once done
	    it should switch back with G__switch_env(). See r.in.gdal */
    }
    else {
	int err = 0;

	/* Projection only required for checking so convert non-interactively */
	if (GPJ_wkt_to_grass(&cellhd, &proj_info,
			     &proj_units, projstr, 0) < 0)
	    G_warning(_("Unable to convert input map projection information to "
		       "GRASS format for checking"));

	/* Does the projection of the current location match the dataset? */
	/* G_get_window seems to be unreliable if the location has been changed */
	G_get_default_window(&loc_wind);
	/* fetch LOCATION PROJ info */
	if (loc_wind.proj != PROJECTION_XY) {
	    loc_proj_info = G_get_projinfo();
	    loc_proj_units = G_get_projunits();
	}

	if (over_flag->answer) {
	    cellhd.proj = loc_wind.proj;
	    cellhd.zone = loc_wind.zone;
	    G_message(_("Over-riding projection check"));
	}
	else if (loc_wind.proj != cellhd.proj
		 || (err =
		     G_compare_projections(loc_proj_info, loc_proj_units,
					   proj_info, proj_units)) != TRUE) {
	    int i_value;

	    strcpy(error_msg,
		   _("Projection of dataset does not"
		     " appear to match current location.\n\n"));

	    /* TODO: output this info sorted by key: */
	    if (loc_wind.proj != cellhd.proj || err != -2) {
		if (loc_proj_info != NULL) {
		    strcat(error_msg, _("GRASS LOCATION PROJ_INFO is:\n"));
		    for (i_value = 0; i_value < loc_proj_info->nitems;
			 i_value++)
			sprintf(error_msg + strlen(error_msg), "%s: %s\n",
				loc_proj_info->key[i_value],
				loc_proj_info->value[i_value]);
		    strcat(error_msg, "\n");
		}

		if (proj_info != NULL) {
		    strcat(error_msg, _("Import dataset PROJ_INFO is:\n"));
		    for (i_value = 0; i_value < proj_info->nitems; i_value++)
			sprintf(error_msg + strlen(error_msg), "%s: %s\n",
				proj_info->key[i_value],
				proj_info->value[i_value]);
		}
		else {
		    strcat(error_msg, _("Import dataset PROJ_INFO is:\n"));
		    if (cellhd.proj == PROJECTION_XY)
			sprintf(error_msg + strlen(error_msg),
				"Dataset proj = %d (unreferenced/unknown)\n",
				cellhd.proj);
		    else if (cellhd.proj == PROJECTION_LL)
			sprintf(error_msg + strlen(error_msg),
				"Dataset proj = %d (lat/long)\n",
				cellhd.proj);
		    else if (cellhd.proj == PROJECTION_UTM)
			sprintf(error_msg + strlen(error_msg),
				"Dataset proj = %d (UTM), zone = %d\n",
				cellhd.proj, cellhd.zone);
		    else if (cellhd.proj == PROJECTION_SP)
			sprintf(error_msg + strlen(error_msg),
				"Dataset proj = %d (State Plane), zone = %d\n",
				cellhd.proj, cellhd.zone);
		    else
			sprintf(error_msg + strlen(error_msg),
				"Dataset proj = %d (unknown), zone = %d\n",
				cellhd.proj, cellhd.zone);
		}
	    }
	    else {
		if (loc_proj_units != NULL) {
		    strcat(error_msg, "GRASS LOCATION PROJ_UNITS is:\n");
		    for (i_value = 0; i_value < loc_proj_units->nitems;
			 i_value++)
			sprintf(error_msg + strlen(error_msg), "%s: %s\n",
				loc_proj_units->key[i_value],
				loc_proj_units->value[i_value]);
		    strcat(error_msg, "\n");
		}

		if (proj_units != NULL) {
		    strcat(error_msg, "Import dataset PROJ_UNITS is:\n");
		    for (i_value = 0; i_value < proj_units->nitems; i_value++)
			sprintf(error_msg + strlen(error_msg), "%s: %s\n",
				proj_units->key[i_value],
				proj_units->value[i_value]);
		}
	    }
	    sprintf(error_msg + strlen(error_msg),
		    _("\nYou can use the -o flag to %s to override this projection check.\n"),
		    G_program_name());
	    strcat(error_msg,
		   _("Consider generating a new location with 'location' parameter"
		    " from input data set.\n"));
	    G_fatal_error(error_msg);
	}
	else {
	    G_verbose_message(_("Projection of input dataset and current "
				"location appear to match"));
	}
    }

    db_init_string(&sql);
    db_init_string(&strval);

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
    Vect_open_new(&Map, out_opt->answer, 1);

    Vect_hist_command(&Map);
    
    n_features = LASHeader_GetPointRecordsCount(LAS_header);
    las_point_format = LASHeader_GetDataFormatId(LAS_header);

    have_time = (las_point_format == 1 || las_point_format == 3 || 
		 las_point_format == 4 || las_point_format == 5);

    have_color = (las_point_format == 2 || las_point_format == 3 || 
		   las_point_format == 5);

    /* Add DB link */
    if (!notab_flag->answer) {
	char *cat_col_name = GV_KEY_COLUMN;

	Fi = Vect_default_field_info(&Map, 1, NULL, GV_1TABLE);

	Vect_map_add_dblink(&Map, 1, out_opt->answer, Fi->table,
			    cat_col_name, Fi->database, Fi->driver);

	/* check available LAS info, depends on POINT DATA RECORD FORMAT [0-5] */
	/* X (double),
	 * Y (double), 
	 * Z (double), 
	 * intensity (double), 
	 * return number (int), 
	 * number of returns (int),
	 * scan direction (int),
	 * flight line edge (int),
	 * classification type (char),
	 * class (char),
	 * time (double) (FORMAT 1, 3, 4, 5),
	 * scan angle rank (int),
	 * source ID (int),
	 * user data (char), ???
	 * red (int)  (FORMAT 2, 3, 5),
	 * green (int) (FORMAT 2, 3, 5),
	 * blue (int) (FORMAT 2, 3, 5)*/
	 
	/* Create table */
	sprintf(buf, "create table %s (%s integer", Fi->table,
		cat_col_name);
	db_set_string(&sql, buf);
	
	/* x, y, z */
	sprintf(buf, ", x_coord double precision");
	db_append_string(&sql, buf);
	sprintf(buf, ", y_coord double precision");
	db_append_string(&sql, buf);
	sprintf(buf, ", z_coord double precision");
	db_append_string(&sql, buf);
	/* intensity */
	sprintf(buf, ", intensity integer");
	db_append_string(&sql, buf);
	/* return number */
	sprintf(buf, ", return integer");
	db_append_string(&sql, buf);
	/* number of returns */
	sprintf(buf, ", n_returns integer");
	db_append_string(&sql, buf);
	/* scan direction */
	sprintf(buf, ", scan_dir integer");
	db_append_string(&sql, buf);
	/* flight line edge */
	sprintf(buf, ", edge integer");
	db_append_string(&sql, buf);
	/* classification type */
	sprintf(buf, ", cl_type varchar(20)");
	db_append_string(&sql, buf);
	/* classification class */
	sprintf(buf, ", class varchar(40)");
	db_append_string(&sql, buf);
	/* GPS time */
	if (have_time) {
	    sprintf(buf, ", gps_time double precision");
	    db_append_string(&sql, buf);
	}
	/* scan angle */
	sprintf(buf, ", angle integer");
	db_append_string(&sql, buf);
	/* source id */
	sprintf(buf, ", src_id integer");
	db_append_string(&sql, buf);
	/* user data */
	sprintf(buf, ", usr_data integer");
	db_append_string(&sql, buf);
	/* colors */
	if (have_color) {
	    sprintf(buf, ", red integer, green integer, blue integer");
	    db_append_string(&sql, buf);
	    sprintf(buf, ", GRASSRGB varchar(11)");
	    db_append_string(&sql, buf);
	}

	db_append_string(&sql, ")");
	G_debug(3, db_get_string(&sql));

	driver =
	    db_start_driver_open_database(Fi->driver,
					  Vect_subst_var(Fi->database,
							 &Map));
	if (driver == NULL) {
	    G_fatal_error(_("Unable open database <%s> by driver <%s>"),
			  Vect_subst_var(Fi->database, &Map), Fi->driver);
	}

	if (db_execute_immediate(driver, &sql) != DB_OK) {
	    db_close_database(driver);
	    db_shutdown_driver(driver);
	    G_fatal_error(_("Unable to create table: '%s'"),
			  db_get_string(&sql));
	}

	if (db_create_index2(driver, Fi->table, cat_col_name) != DB_OK)
	    G_warning(_("Unable to create index for table <%s>, key <%s>"),
		      Fi->table, cat_col_name);

	if (db_grant_on_table
	    (driver, Fi->table, DB_PRIV_SELECT,
	     DB_GROUP | DB_PUBLIC) != DB_OK)
	    G_fatal_error(_("Unable to grant privileges on table <%s>"),
			  Fi->table);

	db_begin_transaction(driver);
    }

    /* Import feature */
    cat = 1;
    not_valid = 0;
    feature_count = 0;
    n_outside = 0;
    n_filtered = 0;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    
    G_important_message(_("Importing %d points..."), n_features);
    while ((LAS_point = LASReader_GetNextPoint(LAS_reader)) != NULL) {
	double x, y, z;

	G_percent(feature_count++, n_features, 1);	/* show something happens */
	
	if (!LASPoint_IsValid(LAS_point)) {
	    not_valid++;
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
	if (return_filter != LAS_ALL) {
	    int return_no = LASPoint_GetReturnNumber(LAS_point);
	    int n_returns = LASPoint_GetNumberOfReturns(LAS_point);
	    int skipme = 1;

	    if (n_returns > 1) {

		switch (return_filter) {
		case LAS_FIRST:
		    if (return_no == 1)
			skipme = 0;
		    break;
		case LAS_LAST:
		    if (return_no == n_returns)
			skipme = 0;
		    break;
		case LAS_MID:
		    if (return_no > 1 && return_no < n_returns)
			skipme = 0;
		    break;
		}
	    }
	    if (skipme) {
		n_filtered++;
		continue;
	    }
	}

	Vect_append_point(Points, x, y, z);
	Vect_cat_set(Cats, 1, cat);
	Vect_write_line(&Map, GV_POINT, Points, Cats);

	/* Attributes */
	if (!notab_flag->answer) {
	    char class_flag;
	    int las_class_type, las_class;

	     /* use LASPoint_Validate (LASPointH hPoint) to check for
	      * return number, number of returns, scan direction, flight line edge,
	      * classification, scan angle rank */
	    sprintf(buf, "insert into %s values ( %d", Fi->table, cat);
	    db_set_string(&sql, buf);

	    /* x, y, z */
	    sprintf(buf, ", %f", x);
	    db_append_string(&sql, buf);
	    sprintf(buf, ", %f", y);
	    db_append_string(&sql, buf);
	    sprintf(buf, ", %f", z);
	    db_append_string(&sql, buf);
	    /* intensity */
	    sprintf(buf, ", %d", LASPoint_GetIntensity(LAS_point));
	    db_append_string(&sql, buf);
	    /* return number */
	    sprintf(buf, ", %d", LASPoint_GetReturnNumber(LAS_point));
	    db_append_string(&sql, buf);
	    /* number of returns */
	    sprintf(buf, ",  %d", LASPoint_GetNumberOfReturns(LAS_point));
	    db_append_string(&sql, buf);
	    /* scan direction */
	    sprintf(buf, ", %d",  LASPoint_GetScanDirection(LAS_point));
	    db_append_string(&sql, buf);
	    /* flight line edge */
	    sprintf(buf, ",  %d", LASPoint_GetFlightLineEdge(LAS_point));
	    db_append_string(&sql, buf);
	    class_flag = LASPoint_GetClassification(LAS_point);
	    /* classification type int or char ? */
	    las_class_type = class_flag / 32;
	    sprintf(buf, ", \"%s\"", class_type[las_class_type].name);
	    db_append_string(&sql, buf);
	    /* classification class int or char ? */
	    las_class = class_flag % 32;
	    if (las_class > 13)
		las_class = 13;
	    sprintf(buf, ", \"%s\"", class_val[las_class].name);
	    db_append_string(&sql, buf);
	    /* GPS time */
	    if (have_time) {
		sprintf(buf, ", %f", LASPoint_GetTime(LAS_point));
		db_append_string(&sql, buf);
	    }
	    /* scan angle */
	    sprintf(buf, ", %d", LASPoint_GetScanAngleRank(LAS_point));
	    db_append_string(&sql, buf);
	    /* source id */
	    sprintf(buf, ", %d", LASPoint_GetPointSourceId(LAS_point));
	    db_append_string(&sql, buf);
	    /* user data */
	    sprintf(buf, ", %d", LASPoint_GetUserData(LAS_point));
	    db_append_string(&sql, buf);
	    /* colors */
	    if (have_color) {
		LASColorH LAS_color = LASPoint_GetColor(LAS_point);
		int red = LASColor_GetRed(LAS_color);
		int green = LASColor_GetGreen(LAS_color);
		int blue = LASColor_GetBlue(LAS_color);

		sprintf(buf, ", %d, %d, %d", red, green, blue);
		db_append_string(&sql, buf);
		sprintf(buf, ", \"%03d:%03d:%03d\"", red, green, blue);
		db_append_string(&sql, buf);
	    }
	    db_append_string(&sql, " )");
	    G_debug(3, db_get_string(&sql));

	    if (db_execute_immediate(driver, &sql) != DB_OK) {
		db_close_database(driver);
		db_shutdown_driver(driver);
		G_fatal_error(_("Cannot insert new row: %s"),
			      db_get_string(&sql));
	    }
	}

	cat++;
    }
    G_percent(n_features, n_features, 1);	/* finish it */

    if (!notab_flag->answer) {
	db_commit_transaction(driver);
	db_close_database_shutdown_driver(driver);
    }
    
    LASSRS_Destroy(LAS_srs);
    LASHeader_Destroy(LAS_header);
    LASReader_Destroy(LAS_reader);

    /* close map */
    if (!notopo_flag->answer)
	Vect_build(&Map);
    Vect_close(&Map);
    
    G_message(_("%d points imported"), n_features - not_valid - n_outside - n_filtered);
    if (not_valid)
	G_message(_("%d input points were not valid"), not_valid);
    if (n_outside)
	G_message(_("%d input points were outside of the selected area"), n_outside);
    if (n_filtered)
	G_message(_("%d input points were filtered by return number"), n_filtered);

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

void print_lasinfo(LASHeaderH LAS_header, LASSRSH LAS_srs)
{
    char *las_srs_proj4 = LASSRS_GetProj4(LAS_srs);
    int las_point_format = LASHeader_GetDataFormatId(LAS_header);

    fprintf(stdout, "\nUsing LAS Library Version '%s'\n\n",
                    LAS_GetFullVersion());
    fprintf(stdout, "LAS File Version:                  %d.%d\n",
                    LASHeader_GetVersionMajor(LAS_header),
                    LASHeader_GetVersionMinor(LAS_header));
    fprintf(stdout, "System ID:                         '%s'\n",
                    LASHeader_GetSystemId(LAS_header));
    fprintf(stdout, "Generating Software:               '%s'\n",
                    LASHeader_GetSoftwareId(LAS_header));
    fprintf(stdout, "File Creation Day/Year:            %d/%d\n",
                    LASHeader_GetCreationDOY(LAS_header),
		    LASHeader_GetCreationYear(LAS_header));
    fprintf(stdout, "Point Data Format:                 %d\n",
                    las_point_format);
    fprintf(stdout, "Number of Point Records:           %d\n",
                    LASHeader_GetPointRecordsCount(LAS_header));
    fprintf(stdout, "Number of Points by Return:        %d %d %d %d %d\n",
                    LASHeader_GetPointRecordsByReturnCount(LAS_header, 0),
                    LASHeader_GetPointRecordsByReturnCount(LAS_header, 1),
                    LASHeader_GetPointRecordsByReturnCount(LAS_header, 2),
                    LASHeader_GetPointRecordsByReturnCount(LAS_header, 3),
                    LASHeader_GetPointRecordsByReturnCount(LAS_header, 4));
    fprintf(stdout, "Scale Factor X Y Z:                %g %g %g\n",
                    LASHeader_GetScaleX(LAS_header),
                    LASHeader_GetScaleY(LAS_header),
                    LASHeader_GetScaleZ(LAS_header));
    fprintf(stdout, "Offset X Y Z:                      %g %g %g\n",
                    LASHeader_GetOffsetX(LAS_header),
                    LASHeader_GetOffsetY(LAS_header),
                    LASHeader_GetOffsetZ(LAS_header));
    fprintf(stdout, "Min X Y Z:                         %g %g %g\n",
                    LASHeader_GetMinX(LAS_header),
                    LASHeader_GetMinY(LAS_header),
                    LASHeader_GetMinZ(LAS_header));
    fprintf(stdout, "Max X Y Z:                         %g %g %g\n",
                    LASHeader_GetMaxX(LAS_header),
                    LASHeader_GetMaxY(LAS_header),
                    LASHeader_GetMaxZ(LAS_header));
    if (las_srs_proj4 && strlen(las_srs_proj4) > 0) {
	fprintf(stdout, "Spatial Reference:\n");
	fprintf(stdout, "%s\n", las_srs_proj4);
    }
    else {
	fprintf(stdout, "Spatial Reference:                 None\n");
    }
    
    fprintf(stdout, "\nData Fields:\n");
    fprintf(stdout, "  'X'\n  'Y'\n  'Z'\n  'Intensity'\n  'Return Number'\n");
    fprintf(stdout, "  'Number of Returns'\n  'Scan Direction'\n");
    fprintf(stdout, "  'Flighline Edge'\n  'Classification'\n  'Scan Angle Rank'\n");
    fprintf(stdout, "  'User Data'\n  'Point Source ID'\n");
    if (las_point_format == 1 || las_point_format == 3 || las_point_format == 4 || las_point_format == 5) {
	fprintf(stdout, "  'GPS Time'\n");
    }
    if (las_point_format == 2 || las_point_format == 3 || las_point_format == 5) {
	fprintf(stdout, "  'Red'\n  'Green'\n  'Blue'\n");
    }
    fprintf(stdout, "\n");
    fflush(stdout);

    return;
}
