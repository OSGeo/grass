
/****************************************************************
 *
 * MODULE:       v.in.ogr
 *
 * AUTHOR(S):    Radim Blazek
 *               Markus Neteler (spatial parm, projection support)
 *               Paul Kelly (projection support)
 *
 * PURPOSE:      Import OGR vectors
 *
 * COPYRIGHT:    (C) 2003, 2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 *
 * TODO: - make fixed field length of OFTIntegerList dynamic
 *       - several other TODOs below
**************************************************************/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include <gdal_version.h>	/* needed for OFTDate */
#include "ogr_api.h"
#include "global.h"

#ifndef MAX
#  define MIN(a,b)      ((a<b) ? a : b)
#  define MAX(a,b)      ((a>b) ? a : b)
#endif

int n_polygons;
int n_polygon_boundaries;
double split_distance;

int geom(OGRGeometryH hGeom, struct Map_info *Map, int field, int cat,
	 double min_area, int type, int mk_centr);
int centroid(OGRGeometryH hGeom, CENTR * Centr, struct spatial_index * Sindex,
	     int field, int cat, double min_area, int type);
int poly_count(OGRGeometryH hGeom, int line2boundary);

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct _param {
	struct Option *dsn, *out, *layer, *spat, *where,
	    *min_area;
	struct Option *snap, *type, *outloc, *cnames, *encoding;
    } param;
    struct _flag {
	struct Flag *list, *no_clean, *force2d, *notab,
	    *region;
	struct Flag *over, *extend, *formats, *tolower, *no_import;
    } flag;

    char *desc;

    int i, j, layer, nogeom, ncnames;
    double xmin, ymin, xmax, ymax;
    int ncols = 0, type;
    double min_area, snap;
    char buf[2000], namebuf[2000], tempvect[GNAME_MAX];
    char *separator;
    
    struct Key_Value *loc_proj_info, *loc_proj_units;
    struct Key_Value *proj_info, *proj_units;
    struct Cell_head cellhd, loc_wind, cur_wind;
    char error_msg[8192];

    /* Vector */
    struct Map_info Map, Tmp, *Out;
    int cat;

    /* Attributes */
    struct field_info *Fi = NULL;
    dbDriver *driver = NULL;
    dbString sql, strval;
    int with_z, input3d;

    /* OGR */
    OGRDataSourceH Ogr_ds;
    OGRLayerH Ogr_layer;
    OGRFieldDefnH Ogr_field;
    char *Ogr_fieldname;
    OGRFieldType Ogr_ftype;
    OGRFeatureH Ogr_feature;
    OGRFeatureDefnH Ogr_featuredefn;
    OGRGeometryH Ogr_geometry, Ogr_oRing, poSpatialFilter;
    OGRSpatialReferenceH Ogr_projection;
    OGREnvelope oExt;
    int have_ogr_extent = 0;

    int OFTIntegerListlength;

    char *output;
    char **layer_names;		/* names of layers to be imported */
    int *layers;		/* layer indexes */
    int nlayers;		/* number of layers to import */
    char **available_layer_names;	/* names of layers to be imported */
    int navailable_layers;
    int layer_id;
    unsigned int n_features, feature_count;
    int overwrite;
    double area_size;
    int use_tmp_vect;
    int ncentr, n_overlaps;
    struct bound_box box;

    xmin = ymin = xmax = ymax = 0.0;
    loc_proj_info = loc_proj_units = NULL;
    Ogr_ds = Ogr_oRing = poSpatialFilter = NULL;
    OFTIntegerListlength = 40;	/* hack due to limitation in OGR */
    area_size = 0.0;
    use_tmp_vect = FALSE;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("import"));
    G_add_keyword("OGR");
    module->description = _("Imports vector data into a GRASS vector map using OGR library.");

    param.dsn = G_define_option();
    param.dsn->key = "dsn";
    param.dsn->type = TYPE_STRING;
    param.dsn->required =YES;
    param.dsn->label = _("OGR datasource name");
    param.dsn->description = _("Examples:\n"
				   "\t\tESRI Shapefile: directory containing shapefiles\n"
				   "\t\tMapInfo File: directory containing mapinfo files");
    
    param.layer = G_define_option();
    param.layer->key = "layer";
    param.layer->type = TYPE_STRING;
    param.layer->required = NO;
    param.layer->multiple = YES;
    param.layer->label =
	_("OGR layer name. If not given, all available layers are imported");
    param.layer->description =
	_("Examples:\n" "\t\tESRI Shapefile: shapefile name\n"
	  "\t\tMapInfo File: mapinfo file name");
    param.layer->guisection = _("Selection");

    param.out = G_define_standard_option(G_OPT_V_OUTPUT);
    param.out->required = NO;
    param.out->guisection = _("Output");
    
    param.spat = G_define_option();
    param.spat->key = "spatial";
    param.spat->type = TYPE_DOUBLE;
    param.spat->multiple = YES;
    param.spat->required = NO;
    param.spat->key_desc = "xmin,ymin,xmax,ymax";
    param.spat->label = _("Import subregion only");
    param.spat->guisection = _("Selection");
    param.spat->description =
	_("Format: xmin,ymin,xmax,ymax - usually W,S,E,N");

    param.where = G_define_standard_option(G_OPT_DB_WHERE);
    param.where->guisection = _("Selection");

    param.min_area = G_define_option();
    param.min_area->key = "min_area";
    param.min_area->type = TYPE_DOUBLE;
    param.min_area->required = NO;
    param.min_area->answer = "0.0001";
    param.min_area->label =
	_("Minimum size of area to be imported (square units)");
    param.min_area->guisection = _("Selection");
    param.min_area->description = _("Smaller areas and "
				  "islands are ignored. Should be greater than snap^2");

    param.type = G_define_standard_option(G_OPT_V_TYPE);
    param.type->options = "point,line,boundary,centroid";
    param.type->answer = "";
    param.type->description = _("Optionally change default input type");
    desc = NULL;
    G_asprintf(&desc,
	       "point;%s;line;%s;boundary;%s;centroid;%s",
	       _("import area centroids as points"),
	       _("import area boundaries as lines"),
	       _("import lines as area boundaries"),
	       _("import points as centroids"));
    param.type->descriptions = desc;
    param.type->guisection = _("Selection");

    param.snap = G_define_option();
    param.snap->key = "snap";
    param.snap->type = TYPE_DOUBLE;
    param.snap->required = NO;
    param.snap->answer = "-1";
    param.snap->label = _("Snapping threshold for boundaries");
    param.snap->description = _("'-1' for no snap");

    param.outloc = G_define_option();
    param.outloc->key = "location";
    param.outloc->type = TYPE_STRING;
    param.outloc->required = NO;
    param.outloc->description = _("Name for new location to create");
    param.outloc->key_desc = "name";
    
    param.cnames = G_define_option();
    param.cnames->key = "cnames";
    param.cnames->type = TYPE_STRING;
    param.cnames->required = NO;
    param.cnames->multiple = YES;
    param.cnames->description =
	_("List of column names to be used instead of original names, "
	  "first is used for category column");
    param.cnames->guisection = _("Attributes");

    param.encoding = G_define_option();
    param.encoding->key = "encoding";
    param.encoding->type = TYPE_STRING;
    param.encoding->required = NO;
    param.encoding->label =
        _("Encoding value for attribute data");
    param.encoding->description = 
        _("Overrides encoding interpretation, useful when importing ESRI Shapefile");
    param.encoding->guisection = _("Attributes");

    flag.formats = G_define_flag();
    flag.formats->key = 'f';
    flag.formats->description = _("List supported OGR formats and exit");
    flag.formats->guisection = _("Print");
    flag.formats->suppress_required = YES;

    flag.list = G_define_flag();
    flag.list->key = 'l';
    flag.list->description = _("List available OGR layers in data source and exit"); 
    flag.list->guisection = _("Print");
    flag.list->suppress_required = YES;

    /* if using -c, you lose topological information ! */
    flag.no_clean = G_define_flag();
    flag.no_clean->key = 'c';
    flag.no_clean->description = _("Do not clean polygons (not recommended)");
    flag.no_clean->guisection = _("Output");

    flag.force2d = G_define_flag();
    flag.force2d->key = '2';
    flag.force2d->label = _("Force 2D output even if input is 3D");
    flag.force2d->description = _("Useful if input is 3D but all z coordinates are identical");
    flag.force2d->guisection = _("Output");

    flag.notab = G_define_standard_flag(G_FLG_V_TABLE);
    flag.notab->guisection = _("Attributes");

    flag.over = G_define_flag();
    flag.over->key = 'o';
    flag.over->description =
	_("Override dataset projection (use location's projection)");

    flag.region = G_define_flag();
    flag.region->key = 'r';
    flag.region->guisection = _("Selection");
    flag.region->description = _("Limit import to the current region");

    flag.extend = G_define_flag();
    flag.extend->key = 'e';
    flag.extend->label =
	_("Extend region extents based on new dataset");
    flag.extend->description =
	_("Also updates the default region if in the PERMANENT mapset");

    flag.tolower = G_define_flag();
    flag.tolower->key = 'w';
    flag.tolower->description =
	_("Change column names to lowercase characters");
    flag.tolower->guisection = _("Attributes");

    flag.no_import = G_define_flag();
    flag.no_import->key = 'i';
    flag.no_import->description =
	_("Create the location specified by the \"location\" parameter and exit."
          " Do not import the vector data.");
    
    /* The parser checks if the map already exists in current mapset, this is
     * wrong if location options is used, so we switch out the check and do it
     * in the module after the parser */
    overwrite = G_check_overwrite(argc, argv);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    G_begin_polygon_area_calculations();	/* Used in geom() */

    OGRRegisterAll();

    /* list supported formats */
    if (flag.formats->answer) {
	int iDriver;

	G_message(_("Available OGR Drivers:"));

	for (iDriver = 0; iDriver < OGRGetDriverCount(); iDriver++) {
	    OGRSFDriverH poDriver = OGRGetDriver(iDriver);
	    const char *pszRWFlag;
	    
	    if (OGR_Dr_TestCapability(poDriver, ODrCCreateDataSource))
		pszRWFlag = "rw";
	    else
		pszRWFlag = "ro";

	    fprintf(stdout, " %s (%s): %s\n",
		    OGR_Dr_GetName(poDriver),
		    pszRWFlag, OGR_Dr_GetName(poDriver));
	}
	exit(EXIT_SUCCESS);
    }

    if (param.dsn->answer == NULL) {
	G_fatal_error(_("Required parameter <%s> not set"), param.dsn->key);
    }

    min_area = atof(param.min_area->answer);
    snap = atof(param.snap->answer);
    type = Vect_option_to_types(param.type);

    ncnames = 0;
    if (param.cnames->answers) {
	i = 0;
	while (param.cnames->answers[i]) {
	    G_strip(param.cnames->answers[i]);
	    G_strchg(param.cnames->answers[i], ' ', '\0');
	    ncnames++;
	    i++;
	}
    }

    /* set up encoding for attribute data */
    if (param.encoding->answer) {
	char *encbuf, *encp;
	int len;
	
	len = strlen("SHAPE_ENCODING") + strlen(param.encoding->answer) + 2;
	encbuf = G_malloc(len * sizeof(char));
        /* -> Esri Shapefile */
	sprintf(encbuf, "SHAPE_ENCODING=%s", param.encoding->answer);
	encp = G_store(encbuf);
	putenv(encp);
        /* -> DXF */
	sprintf(encbuf, "DXF_ENCODING=%s", param.encoding->answer);
	encp = G_store(encbuf);
	putenv(encp);
        /* todo: others ? */
	G_free(encbuf);
    }

    /* open OGR DSN */
    Ogr_ds = NULL;
    if (strlen(param.dsn->answer) > 0)
	Ogr_ds = OGROpen(param.dsn->answer, FALSE, NULL);
    if (Ogr_ds == NULL)
	G_fatal_error(_("Unable to open data source <%s>"), param.dsn->answer);

    /* check encoding for given driver */
    if (param.encoding->answer) {
        const char *driver_name;

        driver_name = OGR_Dr_GetName(OGR_DS_GetDriver(Ogr_ds));
        if (strcmp(driver_name, "ESRI Shapefile") != 0 &&
            strcmp(driver_name, "DXF") != 0)
            G_warning(_("Encoding value not supported by OGR driver <%s>"), driver_name);
    }

    /* make a list of available layers */
    navailable_layers = OGR_DS_GetLayerCount(Ogr_ds);
    available_layer_names =
	(char **)G_malloc(navailable_layers * sizeof(char *));

    if (flag.list->answer)
	G_message(_("Data source <%s> (format '%s') contains %d layers:"),
		  param.dsn->answer,
		  OGR_Dr_GetName(OGR_DS_GetDriver(Ogr_ds)), navailable_layers);
    for (i = 0; i < navailable_layers; i++) {
	Ogr_layer = OGR_DS_GetLayer(Ogr_ds, i);
	Ogr_featuredefn = OGR_L_GetLayerDefn(Ogr_layer);
        
	available_layer_names[i] =
	    G_store((char *)OGR_FD_GetName(Ogr_featuredefn));

	if (flag.list->answer)
	    fprintf(stdout, "%s\n", available_layer_names[i]);
    }
    if (flag.list->answer) {
	fflush(stdout);
	OGR_DS_Destroy(Ogr_ds);
	exit(EXIT_SUCCESS);
    }
    
    /* Make a list of layers to be imported */
    if (param.layer->answer) {	/* From option */
	nlayers = 0;
	while (param.layer->answers[nlayers])
	    nlayers++;

	layer_names = (char **)G_malloc(nlayers * sizeof(char *));
	layers = (int *)G_malloc(nlayers * sizeof(int));

	for (i = 0; i < nlayers; i++) {
	    layer_names[i] = G_store(param.layer->answers[i]);
	    /* Find it in the source */
	    layers[i] = -1;
	    for (j = 0; j < navailable_layers; j++) {
		if (strcmp(available_layer_names[j], layer_names[i]) == 0) {
		    layers[i] = j;
		    break;
		}
	    }
	    if (layers[i] == -1)
		G_fatal_error(_("Layer <%s> not available"), layer_names[i]);
	}
    }
    else {			/* use list of all layers */
	nlayers = navailable_layers;
	layer_names = available_layer_names;
	layers = (int *)G_malloc(nlayers * sizeof(int));
	for (i = 0; i < nlayers; i++)
	    layers[i] = i;
    }

    if (param.out->answer) {
	output = G_store(param.out->answer);
    }
    else {
	if (nlayers < 1)
	    G_fatal_error(_("No OGR layers available"));
	output = G_store(layer_names[0]);
	if (param.layer->answer == NULL)
	    G_warning(_("All available OGR layers will be imported into vector map <%s>"), output);
    }
    
    if (!param.outloc->answer) {	/* Check if the map exists */
	if (G_find_vector2(output, G_mapset()) && !overwrite)
	    G_fatal_error(_("Vector map <%s> already exists"),
			  output);
    }

    /* Get first imported layer to use for extents and projection check */
    Ogr_layer = OGR_DS_GetLayer(Ogr_ds, layers[0]);

    /* fetch boundaries */
    G_get_window(&cellhd);
    if ((OGR_L_GetExtent(Ogr_layer, &oExt, 1)) == OGRERR_NONE) {
	cellhd.north = ymax = oExt.MaxY;
	cellhd.south = ymin = oExt.MinY;
	cellhd.west = xmin = oExt.MinX;
	cellhd.east = xmax = oExt.MaxX;
	cellhd.rows = 20;	/* TODO - calculate useful values */
	cellhd.cols = 20;
	cellhd.ns_res = (cellhd.north - cellhd.south) / cellhd.rows;
	cellhd.ew_res = (cellhd.east - cellhd.west) / cellhd.cols;

	/* use OGR extents if possible, needed to skip corrupted data
	 * in OGR dsn/layer */
	have_ogr_extent = 1;
    }
    
    if (!have_ogr_extent) {
	cellhd.north = 1.;
	cellhd.south = 0.;
	cellhd.west = 0.;
	cellhd.east = 1.;
	cellhd.top = 1.;
	cellhd.bottom = 1.;
	cellhd.rows = 1;
	cellhd.rows3 = 1;
	cellhd.cols = 1;
	cellhd.cols3 = 1;
	cellhd.depths = 1;
	cellhd.ns_res = 1.;
	cellhd.ns_res3 = 1.;
	cellhd.ew_res = 1.;
	cellhd.ew_res3 = 1.;
	cellhd.tb_res = 1.;
    }

    /* set spatial filter */
    if (flag.region->answer) {
	if (param.spat->answer)
	    G_fatal_error(_("Select either the current region flag or the spatial option, not both"));
	if (nlayers > 1)
	    G_warning(_("The region flag is applied only to the first OGR layer"));

	G_get_window(&cur_wind);
	if (have_ogr_extent) {
	    /* check for any overlap */
	    if (cur_wind.west > xmax || cur_wind.east < xmin ||
	        cur_wind.south > ymax || cur_wind.north < ymin) {
		G_warning(_("The current region does not overlap with OGR input. Nothing to import."));
		OGR_DS_Destroy(Ogr_ds);
		exit(EXIT_SUCCESS);
	    }
	    if (xmin < cur_wind.west)
		xmin = cur_wind.west;
	    if (xmax > cur_wind.east)
		xmax = cur_wind.east;
	    if (ymin < cur_wind.south)
		ymin = cur_wind.south;
	    if (ymax > cur_wind.north)
		ymax = cur_wind.north;
	}
	else {
	    xmin = cur_wind.west;
	    xmax = cur_wind.east;
	    ymin = cur_wind.south;
	    ymax = cur_wind.north;
	}
    }
    if (param.spat->answer) {
	double spatxmin = xmin,
	       spatxmax = xmax,
	       spatymin = ymin,
	       spatymax = ymax;

	if (nlayers > 1)
	    G_warning(_("The 'spatial' option is applied only to the first OGR layer"));

	/* See as reference: gdal/ogr/ogr_capi_test.c */

	/* cut out a piece of the map */
	/* order: xmin,ymin,xmax,ymax */
	i = 0;
	while (param.spat->answers[i]) {
	    if (i == 0)
		spatxmin = atof(param.spat->answers[i]);
	    if (i == 1)
		spatymin = atof(param.spat->answers[i]);
	    if (i == 2)
		spatxmax = atof(param.spat->answers[i]);
	    if (i == 3)
		spatymax = atof(param.spat->answers[i]);
	    i++;
	}
	if (i != 4)
	    G_fatal_error(_("4 parameters required for 'spatial' parameter"));

	if (!have_ogr_extent) {
	    xmin = spatxmin;
	    ymin = spatymin;
	    xmax = spatxmax;
	    ymax = spatymax;
	}
	else {
	    /* check for any overlap */
	    if (spatxmin > xmax || spatxmax < xmin ||
	        spatymin > ymax || spatymax < ymin) {
		G_warning(_("The 'spatial' parameters do not overlap with OGR input. Nothing to import."));
		OGR_DS_Destroy(Ogr_ds);
		exit(EXIT_SUCCESS);
	    }
	    if (xmin < spatxmin)
		xmin = spatxmin;
	    if (ymin < spatymin)
		ymin = spatymin;
	    if (xmax > spatxmax)
		xmax = spatxmax;
	    if (ymax > spatymax)
		ymax = spatymax;
	}
    }
    if (param.spat->answer || flag.region->answer || have_ogr_extent) {
	G_debug(2, "cut out with boundaries: xmin:%f ymin:%f xmax:%f ymax:%f",
		xmin, ymin, xmax, ymax);

	/* in theory this could be an irregular polygon */
	poSpatialFilter = OGR_G_CreateGeometry(wkbPolygon);
	Ogr_oRing = OGR_G_CreateGeometry(wkbLinearRing);
	OGR_G_AddPoint(Ogr_oRing, xmin, ymin, 0.0);
	OGR_G_AddPoint(Ogr_oRing, xmin, ymax, 0.0);
	OGR_G_AddPoint(Ogr_oRing, xmax, ymax, 0.0);
	OGR_G_AddPoint(Ogr_oRing, xmax, ymin, 0.0);
	OGR_G_AddPoint(Ogr_oRing, xmin, ymin, 0.0);
	OGR_G_AddGeometryDirectly(poSpatialFilter, Ogr_oRing);

	OGR_L_SetSpatialFilter(Ogr_layer, poSpatialFilter);
    }

    if (param.where->answer) {
	/* select by attribute */
	if (nlayers > 1)
	    G_warning(_("The 'where' option is applied only to the first OGR layer"));

	OGR_L_SetAttributeFilter(Ogr_layer, param.where->answer);
    }

    /* suppress boundary splitting ? */
    if (flag.no_clean->answer) {
	split_distance = -1.;
    }
    else {
	split_distance = 0.;
	area_size =
	    sqrt((cellhd.east - cellhd.west) * (cellhd.north - cellhd.south));
    }

    /* Fetch input map projection in GRASS form. */
    proj_info = NULL;
    proj_units = NULL;
    Ogr_projection = OGR_L_GetSpatialRef(Ogr_layer);	/* should not be freed later */

    /* Do we need to create a new location? */
    if (param.outloc->answer != NULL) {
	/* Convert projection information non-interactively as we can't
	 * assume the user has a terminal open */
	if (GPJ_osr_to_grass(&cellhd, &proj_info,
			     &proj_units, Ogr_projection, 0) < 0) {
	    G_fatal_error(_("Unable to convert input map projection to GRASS "
			    "format; cannot create new location."));
	}
	else {
            if (0 != G_make_location(param.outloc->answer, &cellhd,
                                     proj_info, proj_units)) {
                G_fatal_error(_("Unable to create new location <%s>"),
                              param.outloc->answer);
            }
	    G_message(_("Location <%s> created"), param.outloc->answer);
	}

        /* If the i flag is set, clean up? and exit here */
        if(flag.no_import->answer)
        {
	    OGR_DS_Destroy(Ogr_ds);
            exit(EXIT_SUCCESS);
        }
    }
    else {
	int err = 0;

	/* Projection only required for checking so convert non-interactively */
	if (GPJ_osr_to_grass(&cellhd, &proj_info,
			     &proj_units, Ogr_projection, 0) < 0)
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

	if (flag.over->answer) {
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
	    G_verbose_message(_("Projection of input dataset and current location "
				"appear to match"));
	}
    }

    db_init_string(&sql);
    db_init_string(&strval);

    n_polygon_boundaries = 0;
    input3d = 0;

    /* check if input id 3D and if we need a tmp vector */
    /* estimate distance for boundary splitting --> */
    for (layer = 0; layer < nlayers; layer++) {
	layer_id = layers[layer];

	Ogr_layer = OGR_DS_GetLayer(Ogr_ds, layer_id);
	Ogr_featuredefn = OGR_L_GetLayerDefn(Ogr_layer);

	n_features = feature_count = 0;

	n_features = OGR_L_GetFeatureCount(Ogr_layer, 1);
	OGR_L_ResetReading(Ogr_layer);

	/* count polygons and isles */
	G_message(_("Counting polygons for %d features (OGR layer <%s>)..."),
		  n_features, layer_names[layer]);
	while ((Ogr_feature = OGR_L_GetNextFeature(Ogr_layer)) != NULL) {
	    G_percent(feature_count++, n_features, 1);	/* show something happens */
	    /* Geometry */
	    Ogr_geometry = OGR_F_GetGeometryRef(Ogr_feature);
	    if (Ogr_geometry != NULL) {
		if (!flag.no_clean->answer)
		    poly_count(Ogr_geometry, (type & GV_BOUNDARY));
		if (OGR_G_GetCoordinateDimension(Ogr_geometry) > 2)
		    input3d = 1;
	    }
	    OGR_F_Destroy(Ogr_feature);
	}
	G_percent(1, 1, 1);
    }

    G_debug(1, "n polygon boundaries: %d", n_polygon_boundaries);
    if (n_polygon_boundaries > 50) {
	split_distance =
	    area_size / log(n_polygon_boundaries);
	/* divisor is the handle: increase divisor to decrease split_distance */
	split_distance = split_distance / 16.;
	G_debug(1, "root of area size: %f", area_size);
	G_verbose_message(_("Boundary splitting distance in map units: %G"),
		  split_distance);
    }
    /* <-- estimate distance for boundary splitting */

    use_tmp_vect = n_polygon_boundaries > 0;

    G_debug(1, "Input is 3D ? %s", (input3d == 0 ? "yes" : "no"));
    with_z = input3d;
    if (with_z)
	with_z = !flag.force2d->answer;

    /* open output vector */
    /* strip any @mapset from vector output name */
    G_find_vector(output, G_mapset());
    Vect_open_new(&Map, output, with_z);
    Out = &Map;

    if (!flag.no_clean->answer) {
	if (use_tmp_vect) {
	    /* open temporary vector, do the work in the temporary vector
	     * at the end copy alive lines to output vector
	     * in case of polygons this reduces the coor file size by a factor of 2 to 5
	     * only needed when cleaning polygons */
	    sprintf(tempvect, "%s_tmp", output);
	    G_verbose_message(_("Using temporary vector <%s>"), tempvect);
	    Vect_open_new(&Tmp, tempvect, with_z);
	    Out = &Tmp;
	}
    }

    Vect_hist_command(&Map);

    ncentr = n_overlaps = n_polygons = 0;

    /* Points and lines are written immediately with categories. Boundaries of polygons are
     * written to the vector then cleaned and centroids are calculated for all areas in cleaan vector.
     * Then second pass through finds all centroids in each polygon feature and adds its category
     * to the centroid. The result is that one centroids may have 0, 1 ore more categories
     * of one ore more (more input layers) fields. */
    for (layer = 0; layer < nlayers; layer++) {
	layer_id = layers[layer];

	Ogr_layer = OGR_DS_GetLayer(Ogr_ds, layer_id);
	Ogr_featuredefn = OGR_L_GetLayerDefn(Ogr_layer);

	/* Add DB link */
	if (!flag.notab->answer) {
	    char *cat_col_name = GV_KEY_COLUMN;

	    if (nlayers == 1) {	/* one layer only */
		Fi = Vect_default_field_info(&Map, layer + 1, NULL,
					     GV_1TABLE);
	    }
	    else {
		Fi = Vect_default_field_info(&Map, layer + 1, NULL,
					     GV_MTABLE);
	    }

	    if (ncnames > 0) {
		cat_col_name = param.cnames->answers[0];
	    }
	    Vect_map_add_dblink(&Map, layer + 1, layer_names[layer], Fi->table,
				cat_col_name, Fi->database, Fi->driver);

	    ncols = OGR_FD_GetFieldCount(Ogr_featuredefn);
	    G_debug(2, "%d columns", ncols);

	    /* Create table */
	    sprintf(buf, "create table %s (%s integer", Fi->table,
		    cat_col_name);
	    db_set_string(&sql, buf);
	    for (i = 0; i < ncols; i++) {

		Ogr_field = OGR_FD_GetFieldDefn(Ogr_featuredefn, i);
		Ogr_ftype = OGR_Fld_GetType(Ogr_field);

		G_debug(3, "Ogr_ftype: %i", Ogr_ftype);	/* look up below */

		if (i < ncnames - 1) {
		    Ogr_fieldname = G_store(param.cnames->answers[i + 1]);
		}
		else {
		    /* Change column names to [A-Za-z][A-Za-z0-9_]* */
		    Ogr_fieldname = G_store(OGR_Fld_GetNameRef(Ogr_field));
		    G_debug(3, "Ogr_fieldname: '%s'", Ogr_fieldname);

		    G_str_to_sql(Ogr_fieldname);

		    G_debug(3, "Ogr_fieldname: '%s'", Ogr_fieldname);

		}

		/* avoid that we get the 'cat' column twice */
		if (strcmp(Ogr_fieldname, GV_KEY_COLUMN) == 0) {
		    sprintf(namebuf, "%s_", Ogr_fieldname);
		    Ogr_fieldname = G_store(namebuf);
		}

		/* captial column names are a pain in SQL */
		if (flag.tolower->answer)
		    G_str_to_lower(Ogr_fieldname);

		if (strcmp(OGR_Fld_GetNameRef(Ogr_field), Ogr_fieldname) != 0) {
		    G_important_message(_("Column name <%s> renamed to <%s>"),
			      OGR_Fld_GetNameRef(Ogr_field), Ogr_fieldname);
		}

		/** Simple 32bit integer                     OFTInteger = 0        **/
		/** List of 32bit integers                   OFTIntegerList = 1    **/
		/** Double Precision floating point          OFTReal = 2           **/
		/** List of doubles                          OFTRealList = 3       **/
		/** String of ASCII chars                    OFTString = 4         **/
		/** Array of strings                         OFTStringList = 5     **/
		/** Double byte string (unsupported)         OFTWideString = 6     **/
		/** List of wide strings (unsupported)       OFTWideStringList = 7 **/
		/** Raw Binary data (unsupported)            OFTBinary = 8         **/
		/**                                          OFTDate = 9           **/
		/**                                          OFTTime = 10          **/
		/**                                          OFTDateTime = 11      **/

		if (Ogr_ftype == OFTInteger) {
		    sprintf(buf, ", %s integer", Ogr_fieldname);
		}
		else if (Ogr_ftype == OFTIntegerList) {
		    /* hack: treat as string */
		    sprintf(buf, ", %s varchar ( %d )", Ogr_fieldname,
			    OFTIntegerListlength);
		    G_warning(_("Writing column <%s> with fixed length %d chars (may be truncated)"),
			      Ogr_fieldname, OFTIntegerListlength);
		}
		else if (Ogr_ftype == OFTReal) {
		    sprintf(buf, ", %s double precision", Ogr_fieldname);
#if GDAL_VERSION_NUM >= 1320
		}
		else if (Ogr_ftype == OFTDate) {
		    sprintf(buf, ", %s date", Ogr_fieldname);
		}
		else if (Ogr_ftype == OFTTime) {
		    sprintf(buf, ", %s time", Ogr_fieldname);
		}
		else if (Ogr_ftype == OFTDateTime) {
		    sprintf(buf, ", %s datetime", Ogr_fieldname);
#endif
		}
		else if (Ogr_ftype == OFTString) {
		    int fwidth;

		    fwidth = OGR_Fld_GetWidth(Ogr_field);
		    /* TODO: read all records first and find the longest string length */
		    if (fwidth == 0) {
			G_warning(_("Width for column %s set to 255 (was not specified by OGR), "
				   "some strings may be truncated!"),
				  Ogr_fieldname);
			fwidth = 255;
		    }
		    sprintf(buf, ", %s varchar ( %d )", Ogr_fieldname,
			    fwidth);
		}
		else if (Ogr_ftype == OFTStringList) {
		    /* hack: treat as string */
		    sprintf(buf, ", %s varchar ( %d )", Ogr_fieldname,
			    OFTIntegerListlength);
		    G_warning(_("Writing column %s with fixed length %d chars (may be truncated)"),
			      Ogr_fieldname, OFTIntegerListlength);
		}
		else {
		    G_warning(_("Column type not supported (%s)"),
			      Ogr_fieldname);
		    buf[0] = 0;
		}
		db_append_string(&sql, buf);
		G_free(Ogr_fieldname);
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
	nogeom = 0;
	OGR_L_ResetReading(Ogr_layer);
	n_features = feature_count = 0;

	n_features = OGR_L_GetFeatureCount(Ogr_layer, 1);

	G_important_message(_("Importing %d features (OGR layer <%s>)..."),
			    n_features, layer_names[layer]);
	while ((Ogr_feature = OGR_L_GetNextFeature(Ogr_layer)) != NULL) {
	    G_percent(feature_count++, n_features, 1);	/* show something happens */
	    /* Geometry */
	    Ogr_geometry = OGR_F_GetGeometryRef(Ogr_feature);
	    if (Ogr_geometry == NULL) {
		nogeom++;
	    }
	    else {
		geom(Ogr_geometry, Out, layer + 1, cat, min_area, type,
		     flag.no_clean->answer);
	    }

	    /* Attributes */
	    if (!flag.notab->answer) {
		sprintf(buf, "insert into %s values ( %d", Fi->table, cat);
		db_set_string(&sql, buf);
		for (i = 0; i < ncols; i++) {
		    Ogr_field = OGR_FD_GetFieldDefn(Ogr_featuredefn, i);
		    Ogr_ftype = OGR_Fld_GetType(Ogr_field);
		    if (OGR_F_IsFieldSet(Ogr_feature, i)) {
			if (Ogr_ftype == OFTInteger || Ogr_ftype == OFTReal) {
			    sprintf(buf, ", %s",
				    OGR_F_GetFieldAsString(Ogr_feature, i));
#if GDAL_VERSION_NUM >= 1320
			    /* should we use OGR_F_GetFieldAsDateTime() here ? */
			}
			else if (Ogr_ftype == OFTDate || Ogr_ftype == OFTTime
				 || Ogr_ftype == OFTDateTime) {
			    char *newbuf;

			    db_set_string(&strval, (char *)
					  OGR_F_GetFieldAsString(Ogr_feature,
								 i));
			    db_double_quote_string(&strval);
			    sprintf(buf, ", '%s'", db_get_string(&strval));
			    newbuf = G_str_replace(buf, "/", "-");	/* fix 2001/10/21 to 2001-10-21 */
			    sprintf(buf, "%s", newbuf);
#endif
			}
			else if (Ogr_ftype == OFTString ||
				 Ogr_ftype == OFTIntegerList) {
			    db_set_string(&strval, (char *)
					  OGR_F_GetFieldAsString(Ogr_feature,
								 i));
			    db_double_quote_string(&strval);
			    sprintf(buf, ", '%s'", db_get_string(&strval));
			}

		    }
		    else {
			/* G_warning (_("Column value not set" )); */
			if (Ogr_ftype == OFTInteger || Ogr_ftype == OFTReal) {
			    sprintf(buf, ", NULL");
#if GDAL_VERSION_NUM >= 1320
			}
			else if (Ogr_ftype == OFTString ||
				 Ogr_ftype == OFTIntegerList ||
				 Ogr_ftype == OFTDate) {
#else
			}
			else if (Ogr_ftype == OFTString ||
				 Ogr_ftype == OFTIntegerList) {
#endif
			    sprintf(buf, ", ''");
			}
		    }
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

	    OGR_F_Destroy(Ogr_feature);
	    cat++;
	}
	G_percent(1, 1, 1);	/* finish it */

	if (!flag.notab->answer) {
	    db_commit_transaction(driver);
	    db_close_database_shutdown_driver(driver);
	}

	if (nogeom > 0)
	    G_warning(_("%d %s without geometry"), nogeom,
		      nogeom == 1 ? "feature" : "features");
    }


    separator = "-----------------------------------------------------";
    G_message("%s", separator);

    if (use_tmp_vect) {
	/* TODO: is it necessary to build here? probably not, consumes time */
	/* GV_BUILD_BASE is sufficient to toggle boundary cleaning */
	Vect_build_partial(&Tmp, GV_BUILD_BASE);
    }

    if (use_tmp_vect && !flag.no_clean->answer &&
	Vect_get_num_primitives(Out, GV_BOUNDARY) > 0) {
	int ret, centr, otype, n_nocat;
	CENTR *Centr;
	struct spatial_index si;
	double x, y, total_area, overlap_area, nocat_area;
	struct line_pnts *Points;
	int nmodif;

	Points = Vect_new_line_struct();

	G_message("%s", separator);

	G_message(_("Cleaning polygons"));

	if (snap >= 0) {
	    G_message("%s", separator);
	    G_message(_("Snapping boundaries (threshold = %.3e)..."), snap);
	    Vect_snap_lines(&Tmp, GV_BOUNDARY, snap, NULL);
	}

	/* It is not to clean to snap centroids, but I have seen data with 2 duplicate polygons
	 * (as far as decimal places were printed) and centroids were not identical */
	/* Disabled, because overlapping polygons result in many duplicate centroids anyway */
	/*
	   fprintf ( stderr, separator );
	   fprintf ( stderr, "Snap centroids (threshold 0.000001):\n" );
	   Vect_snap_lines ( &Map, GV_CENTROID, 0.000001, NULL, stderr );
	 */

	G_message("%s", separator);
	G_message(_("Breaking polygons..."));
	Vect_break_polygons(&Tmp, GV_BOUNDARY, NULL);

	/* It is important to remove also duplicate centroids in case of duplicate input polygons */
	G_message("%s", separator);
	G_message(_("Removing duplicates..."));
	Vect_remove_duplicates(&Tmp, GV_BOUNDARY | GV_CENTROID, NULL);

	/* in non-pathological cases, the bulk of the cleaning is now done */

	/* Vect_clean_small_angles_at_nodes() can change the geometry so that new intersections
	 * are created. We must call Vect_break_lines(), Vect_remove_duplicates()
	 * and Vect_clean_small_angles_at_nodes() until no more small angles are found */
	do {
	    G_message("%s", separator);
	    G_message(_("Breaking boundaries..."));
	    Vect_break_lines(&Tmp, GV_BOUNDARY, NULL);

	    G_message("%s", separator);
	    G_message(_("Removing duplicates..."));
	    Vect_remove_duplicates(&Tmp, GV_BOUNDARY, NULL);

	    G_message("%s", separator);
	    G_message(_("Cleaning boundaries at nodes..."));
	    nmodif =
		Vect_clean_small_angles_at_nodes(&Tmp, GV_BOUNDARY, NULL);
	} while (nmodif > 0);

	/* merge boundaries */
	G_message("%s", separator);
	G_message(_("Merging boundaries..."));
	Vect_merge_lines(&Tmp, GV_BOUNDARY, NULL, NULL);

	G_message("%s", separator);
	if (type & GV_BOUNDARY) {	/* that means lines were converted to boundaries */
	    G_message(_("Changing boundary dangles to lines..."));
	    Vect_chtype_dangles(&Tmp, -1.0, NULL);
	}
	else {
	    G_message(_("Removing dangles..."));
	    Vect_remove_dangles(&Tmp, GV_BOUNDARY, -1.0, NULL);
	}

	G_message("%s", separator);
	Vect_build_partial(&Tmp, GV_BUILD_AREAS);

	G_message("%s", separator);
	if (type & GV_BOUNDARY) {
	    G_message(_("Changing boundary bridges to lines..."));
	    Vect_chtype_bridges(&Tmp, NULL, &nmodif, NULL);
	    if (nmodif)
		Vect_build_partial(&Tmp, GV_BUILD_NONE);
	}
	else {
	    G_message(_("Removing bridges..."));
	    Vect_remove_bridges(&Tmp, NULL, &nmodif, NULL);
	    if (nmodif)
		Vect_build_partial(&Tmp, GV_BUILD_NONE);
	}

	/* Boundaries are hopefully clean, build areas */
	G_message("%s", separator);
	Vect_build_partial(&Tmp, GV_BUILD_NONE);
	Vect_build_partial(&Tmp, GV_BUILD_ATTACH_ISLES);

	/* Calculate new centroids for all areas, centroids have the same id as area */
	ncentr = Vect_get_num_areas(&Tmp);
	G_debug(3, "%d centroids/areas", ncentr);

	Centr = (CENTR *) G_calloc(ncentr + 1, sizeof(CENTR));
	Vect_spatial_index_init(&si, 0);
	for (centr = 1; centr <= ncentr; centr++) {
	    Centr[centr].valid = 0;
	    Centr[centr].cats = Vect_new_cats_struct();
	    ret = Vect_get_point_in_area(&Tmp, centr, &x, &y);
	    if (ret < 0) {
		G_warning(_("Unable to calculate area centroid"));
		continue;
	    }

	    Centr[centr].x = x;
	    Centr[centr].y = y;
	    Centr[centr].valid = 1;
	    box.N = box.S = y;
	    box.E = box.W = x;
	    box.T = box.B = 0;
	    Vect_spatial_index_add_item(&si, centr, &box);
	}

	/* Go through all layers and find centroids for each polygon */
	for (layer = 0; layer < nlayers; layer++) {
	    G_message("%s", separator);
	    G_message(_("Finding centroids for OGR layer <%s>..."), layer_names[layer]);
	    layer_id = layers[layer];
	    Ogr_layer = OGR_DS_GetLayer(Ogr_ds, layer_id);
	    n_features = OGR_L_GetFeatureCount(Ogr_layer, 1);
	    OGR_L_ResetReading(Ogr_layer);

	    cat = 0;		/* field = layer + 1 */
	    G_percent(cat, n_features, 2);
	    while ((Ogr_feature = OGR_L_GetNextFeature(Ogr_layer)) != NULL) {
		cat++;
		G_percent(cat, n_features, 2);
		/* Geometry */
		Ogr_geometry = OGR_F_GetGeometryRef(Ogr_feature);
		if (Ogr_geometry != NULL) {
		    centroid(Ogr_geometry, Centr, &si, layer + 1, cat,
			     min_area, type);
		}

		OGR_F_Destroy(Ogr_feature);
	    }
	}

	/* Write centroids */
	G_message("%s", separator);
	G_message(_("Writing centroids..."));

	n_overlaps = n_nocat = 0;
	total_area = overlap_area = nocat_area = 0.0;
	for (centr = 1; centr <= ncentr; centr++) {
	    double area;
	    
	    G_percent(centr, ncentr, 2);

	    area = Vect_get_area_area(&Tmp, centr);
	    total_area += area;

	    if (!(Centr[centr].valid)) {
		continue;
	    }

	    if (Centr[centr].cats->n_cats == 0) {
		nocat_area += area;
		n_nocat++;
		continue;
	    }

	    if (Centr[centr].cats->n_cats > 1) {
		Vect_cat_set(Centr[centr].cats, nlayers + 1,
			     Centr[centr].cats->n_cats);
		overlap_area += area;
		n_overlaps++;
	    }

	    Vect_reset_line(Points);
	    Vect_append_point(Points, Centr[centr].x, Centr[centr].y, 0.0);
	    if (type & GV_POINT)
		otype = GV_POINT;
	    else
		otype = GV_CENTROID;
	    Vect_write_line(&Tmp, otype, Points, Centr[centr].cats);
	}
	if (Centr)
	    G_free(Centr);
	    
	Vect_spatial_index_destroy(&si);

	if (n_overlaps > 0) {
	    G_warning(_("%d areas represent more (overlapping) features, because polygons overlap "
		       "in input layer(s). Such areas are linked to more than 1 row in attribute table. "
		       "The number of features for those areas is stored as category in layer %d"),
		      n_overlaps, nlayers + 1);
	}

	G_message("%s", separator);

	Vect_hist_write(&Map, separator);
	Vect_hist_write(&Map, "\n");
	sprintf(buf, _("%d input polygons\n"), n_polygons);
	G_message(_("%d input polygons"), n_polygons);
	Vect_hist_write(&Map, buf);

	sprintf(buf, _("Total area: %G (%d areas)\n"), total_area, ncentr);
	G_message(_("Total area: %G (%d areas)"), total_area, ncentr);
	Vect_hist_write(&Map, buf);

	sprintf(buf, _("Overlapping area: %G (%d areas)\n"), overlap_area,
		n_overlaps);
	if (n_overlaps) {
	    G_message(_("Overlapping area: %G (%d areas)"), overlap_area,
		      n_overlaps);
	}
	Vect_hist_write(&Map, buf);

	sprintf(buf, _("Area without category: %G (%d areas)\n"), nocat_area,
		n_nocat);
	if (n_nocat) {
	    G_message(_("Area without category: %G (%d areas)"), nocat_area,
		      n_nocat);
	}
	Vect_hist_write(&Map, buf);
	G_message("%s", separator);
    }

    OGR_DS_Destroy(Ogr_ds);

    if (use_tmp_vect) {
	/* Copy temporary vector to output vector */
	Vect_copy_map_lines(&Tmp, &Map);
	/* release memory occupied by topo, we may need that memory for main output */
	Vect_set_release_support(&Tmp);
	Vect_close(&Tmp);
	Vect_delete(tempvect);
    }

    Vect_build(&Map);
    if (0 && flag.no_clean->answer)
	Vect_topo_check(&Map, NULL);

    if (n_polygons) {
	/* test for topological errors */
	/* this test is not perfect:
	 * small gaps (areas without centroid) are not detected
	 * small gaps may also be true gaps */
	ncentr = Vect_get_num_primitives(&Map, GV_CENTROID);
	if (ncentr != n_polygons || n_overlaps) {
	    double min_snap, max_snap;

	    Vect_get_map_box(&Map, &box);
	    
	    if (abs(box.E) > abs(box.W))
		xmax = abs(box.E);
	    else
		xmax = abs(box.W);
	    if (abs(box.N) > abs(box.S))
		xmax = abs(box.N);
	    else
		xmax = abs(box.S);

	    if (xmax < ymax)
		xmax = ymax;

	    /* double precision ULP */
	    min_snap = log2(xmax) - 52;
	    min_snap = pow(2, min_snap);
	    /* human readable */
	    min_snap = log10(min_snap);
	    if (min_snap < 0)
		min_snap = (int)min_snap;
	    else
		min_snap = (int)min_snap + 1;
	    min_snap = pow(10, min_snap);

	    /* single precision ULP */
	    max_snap = log2(xmax) - 23;
	    max_snap = pow(2, max_snap);
	    /* human readable */
	    max_snap = log10(max_snap);
	    if (max_snap < 0)
		max_snap = (int)max_snap;
	    else
		max_snap = (int)max_snap + 1;
	    max_snap = pow(10, max_snap);

	    G_important_message("%s", separator);
	    G_warning(_("Errors were encountered during the import"));

	    if (snap < min_snap) {
		G_important_message(_("Try to import again, snapping with at least %g: 'snap=%g'"), min_snap, min_snap);
	    }
	    else if (snap < max_snap) {
		min_snap = snap * 10;
		G_important_message(_("Try to import again, snapping with %g: 'snap=%g'"), min_snap, min_snap);
	    }
	    /* else assume manual cleaning is required */
	}
    }

    Vect_close(&Map);

    /* -------------------------------------------------------------------- */
    /*      Extend current window based on dataset.                         */
    /* -------------------------------------------------------------------- */
    if (flag.extend->answer) {
	if (strcmp(G_mapset(), "PERMANENT") == 0)
	    /* fixme: expand WIND and DEFAULT_WIND independently. (currently 
		WIND gets forgotten and DEFAULT_WIND is expanded for both) */
	    G_get_default_window(&cur_wind);
	else
	    G_get_window(&cur_wind);

	cur_wind.north = MAX(cur_wind.north, cellhd.north);
	cur_wind.south = MIN(cur_wind.south, cellhd.south);
	cur_wind.west = MIN(cur_wind.west, cellhd.west);
	cur_wind.east = MAX(cur_wind.east, cellhd.east);

	cur_wind.rows = (int)ceil((cur_wind.north - cur_wind.south)
				  / cur_wind.ns_res);
	cur_wind.south = cur_wind.north - cur_wind.rows * cur_wind.ns_res;

	cur_wind.cols = (int)ceil((cur_wind.east - cur_wind.west)
				  / cur_wind.ew_res);
	cur_wind.east = cur_wind.west + cur_wind.cols * cur_wind.ew_res;

	if (strcmp(G_mapset(), "PERMANENT") == 0) {
	    G__put_window(&cur_wind, "", "DEFAULT_WIND");
	    G_message(_("Default region for this location updated"));
	}
	G_put_window(&cur_wind);
	G_message(_("Region for the current mapset updated"));
    }

    if (input3d && flag.force2d->answer)
	G_warning(_("Input data contains 3D features. Created vector is 2D only, "
		   "disable -2 flag to import 3D vector."));

    exit(EXIT_SUCCESS);
}
