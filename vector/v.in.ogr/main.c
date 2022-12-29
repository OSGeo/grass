
/****************************************************************
 *
 * MODULE:       v.in.ogr
 *
 * AUTHOR(S):    Radim Blazek
 *               Markus Neteler (spatial parm, projection support)
 *               Paul Kelly (projection support)
 * 		 Markus Metz
 *
 * PURPOSE:      Import vector data with OGR
 *
 * COPYRIGHT:    (C) 2003-2016 by the GRASS Development Team
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
#include <math.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include <gdal_version.h>	/* needed for OFTDate */
#include <cpl_conv.h>
#include "global.h"
#include "pavl.h"

#ifndef MAX
#  define MIN(a,b)      ((a<b) ? a : b)
#  define MAX(a,b)      ((a>b) ? a : b)
#endif

int n_polygons;
int n_polygon_boundaries;
double split_distance;

int geom(OGRGeometryH hGeomAny, struct Map_info *Map, int field, int cat,
	 double min_area, int type, int mk_centr);
int centroid(OGRGeometryH hGeomAny, CENTR * Centr, struct spatial_index * Sindex,
	     int field, int cat, double min_area, int type);
int poly_count(OGRGeometryH hGeom, int line2boundary);

char *get_datasource_name(const char *, int);

void convert_osm_lines(struct Map_info *Map, double snap);

int cmp_layer_srs(ds_t, int, int *, char **, char *);
void check_projection(struct Cell_head *cellhd, ds_t hDS, int layer, char *geom_col,
                      char *outloc, int create_only, int override,
		      int check_only);

int create_spatial_filter(ds_t Ogr_ds, OGRGeometryH *,
                          int , int *, char **,
                          double *, double *,
			  double *, double *,
			  int , struct Option *);

struct OGR_iterator
{
    ds_t Ogr_ds;
    char *dsn;
    int nlayers;
    int has_nonempty_layers;
    int ogr_interleaved_reading;
    OGRLayerH Ogr_layer;
    OGRFeatureDefnH Ogr_featuredefn;
    int requested_layer;
    int curr_layer;
    int done;
};

void OGR_iterator_init(struct OGR_iterator *OGR_iter,
                       ds_t Ogr_ds, char *dsn, int nlayers,
		       int ogr_interleaved_reading);

void OGR_iterator_reset(struct OGR_iterator *OGR_iter);
OGRFeatureH ogr_getnextfeature(struct OGR_iterator *, int, char *,
			       OGRGeometryH , const char *);

struct grass_col_info
{
    int idx;	/* index for create table */
    const char *name;
    const char *type;
};

/* for qsort: compare columns by name */
int cmp_col_name(const void *a, const void *b)
{
    struct grass_col_info *ca = (struct grass_col_info *)a;
    struct grass_col_info *cb = (struct grass_col_info *)b;

    return strcmp(ca->name, cb->name);
}

/* for qsort: compare columns by index */
int cmp_col_idx(const void *a, const void *b)
{
    struct grass_col_info *ca = (struct grass_col_info *)a;
    struct grass_col_info *cb = (struct grass_col_info *)b;

    return (ca->idx - cb->idx);
}

struct fid_cat
{
    grass_int64 fid;
    int cat;
};

static int cmp_fid_cat(const void *a, const void *b)
{
    struct fid_cat *aa = (struct fid_cat *)a;
    struct fid_cat *bb = (struct fid_cat *)b;

    return (aa->fid < bb->fid ? -1 : (aa->fid > bb->fid));
}

static void free_fid_cat(void *p)
{
    G_free(p);
}

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct _param {
	struct Option *dsn, *out, *layer, *spat, *where,
	    *min_area, *cfg, *doo;
        struct Option *snap, *type, *outloc, *cnames, *encoding, *key, *geom;
    } param;
    struct _flag {
	struct Flag *list, *no_clean, *force2d, *notab,
	    *region, *over, *extend, *formats, *tolower, *no_import,
            *proj;
    } flag;

    char *desc;

    int i, j, layer, nogeom, ncnames, igeom;
    double xmin, ymin, xmax, ymax;
    int ncols = 0, type;
    double min_area, snap;
    char buf[DB_SQL_MAX], namebuf[1024];
    char *sqlbuf;
    size_t sqlbufsize;
    char *separator;

    struct Cell_head cellhd, cur_wind;

    /* Vector */
    struct Map_info Map, Tmp, *Out;
    int cat;
    int delete_table = FALSE; /* for external output format only */
    
    /* Attributes */
    struct field_info *Fi = NULL;
    dbDriver *driver = NULL;
    dbString sql, strval;
    int with_z, input3d;
    const char **key_column;
    int *key_idx;

    /* OGR */
    ds_t Ogr_ds;
    const char *ogr_driver_name;
    int ogr_interleaved_reading;
    OGRLayerH Ogr_layer;
    OGRFieldDefnH Ogr_field;
    char *Ogr_fieldname;
    OGRFieldType Ogr_ftype;
    OGRFeatureH Ogr_feature;
    OGRFeatureDefnH Ogr_featuredefn;
    OGRGeometryH Ogr_geometry, *poSpatialFilter;
    const char *attr_filter;
    struct OGR_iterator OGR_iter;

    int OFTIntegerListlength;

    char *dsn, **doo;
    const char *driver_name;
    const char *datetime_type;
    char *output;
    char **layer_names;		/* names of layers to be imported */
    int *layers;		/* layer indexes */
    int nlayers;		/* number of layers to import */
    char **available_layer_names;	/* names of layers to be imported */
    int navailable_layers;
    int layer_id;
    unsigned int feature_count;
    GIntBig *n_features, n_import_features;
    int overwrite;
    double area_size;
    int use_tmp_vect;
    int ncentr, n_overlaps;
    int failed_centr, err_boundaries, err_centr_out, err_centr_dupl;
    struct bound_box box;
    struct pavl_table **fid_cat_tree;
    struct fid_cat *new_fid_cat, find_fid_cat;

    xmin = ymin = 1.0;
    xmax = ymax = 0.0;
    Ogr_ds = NULL;
    poSpatialFilter = NULL;
    OFTIntegerListlength = 255;	/* hack due to limitation in OGR */
    area_size = 0.0;
    use_tmp_vect = FALSE;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("import"));
    G_add_keyword("OGR");
    G_add_keyword(_("topology"));
    G_add_keyword(_("geometry"));
    G_add_keyword(_("snapping"));
    G_add_keyword(_("create location"));
    module->description = _("Imports vector data into a GRASS vector map using OGR library.");

    param.dsn = G_define_option();
    param.dsn->key = "input";
    param.dsn->type = TYPE_STRING;
    param.dsn->required =YES;
    param.dsn->label = _("Name of OGR datasource to be imported");
    param.dsn->description = _("Examples:\n"
				   "\t\tESRI Shapefile: directory containing shapefiles\n"
				   "\t\tMapInfo File: directory containing mapinfo files");
    param.dsn->gisprompt = "old,datasource,datasource";
    
    param.cfg = G_define_option();
    param.cfg->key = "gdal_config";
    param.cfg->type = TYPE_STRING;
    param.cfg->required = NO;
    param.cfg->label = _("GDAL configuration options");
    param.cfg->description = _("Comma-separated list of key=value pairs");

    param.doo = G_define_option();
    param.doo->key = "gdal_doo";
    param.doo->type = TYPE_STRING;
    param.doo->required = NO;
    param.doo->label = _("GDAL dataset open options");
    param.doo->description = _("Comma-separated list of key=value pairs");

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
    param.layer->guisection = _("Input");
    param.layer->gisprompt = "old,datasource_layer,datasource_layer";

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
	_("Minimum size of area to be imported (square meters)");
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
    param.snap->label = _("Snapping threshold for boundaries (map units)");
    param.snap->description = _("'-1' for no snap");

    param.outloc = G_define_option();
    param.outloc->key = "location";
    param.outloc->type = TYPE_STRING;
    param.outloc->required = NO;
    param.outloc->description = _("Name for new location to create");
    param.outloc->key_desc = "name";
    param.outloc->guisection = _("Output");
    
    param.cnames = G_define_standard_option(G_OPT_DB_COLUMNS);
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

    param.key = G_define_option();
    param.key->key = "key";
    param.key->type = TYPE_STRING;
    param.key->required = NO;
    param.key->label =
        _("Name of column used for categories");
    param.key->description = 
        _("If not given, categories are generated as unique values and stored in 'cat' column");
    param.key->guisection = _("Attributes");

    param.geom = G_define_standard_option(G_OPT_DB_COLUMN);
    param.geom->key = "geometry";
    param.geom->label = _("Name of geometry column");
    param.geom->description = _("If not given, all geometry columns from the input are used");
    param.geom->guisection = _("Selection");

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
    flag.over->label =
	_("Override projection check (use current location's projection)");
    flag.over->description =
	_("Assume that the dataset has the same projection as the current location");

    flag.proj = G_define_flag();
    flag.proj->key = 'j';
    flag.proj->description =
	_("Perform projection check only and exit");
    flag.proj->suppress_required = YES;
    G_option_requires(flag.proj, param.dsn, NULL);
    
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
    flag.no_import->guisection = _("Output");
    
    /* The parser checks if the map already exists in current mapset, this is
     * wrong if location options is used, so we switch out the check and do it
     * in the module after the parser */
    overwrite = G_check_overwrite(argc, argv);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

#if GDAL_VERSION_NUM >= 2000000
    GDALAllRegister();
#else
    OGRRegisterAll();
#endif

    G_debug(1, "GDAL version %d", GDAL_VERSION_NUM);

    /* list supported formats */
    if (flag.formats->answer) {
	int iDriver;

	G_message(_("Supported formats:"));

#if GDAL_VERSION_NUM >= 2000000
	for (iDriver = 0; iDriver < GDALGetDriverCount(); iDriver++) {
	    GDALDriverH hDriver = GDALGetDriver(iDriver);
	    const char *pszRWFlag;

            if (!GDALGetMetadataItem(hDriver, GDAL_DCAP_VECTOR, NULL))
		continue;

	    if (GDALGetMetadataItem(hDriver, GDAL_DCAP_CREATE, NULL))
		pszRWFlag = "rw+";
	    else if (GDALGetMetadataItem(hDriver, GDAL_DCAP_CREATECOPY, NULL))
		pszRWFlag = "rw";
	    else
		pszRWFlag = "ro";

	    fprintf(stdout, " %s (%s): %s\n",
		    GDALGetDriverShortName(hDriver),
		    pszRWFlag, GDALGetDriverLongName(hDriver));
	}

#else
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
#endif
	exit(EXIT_SUCCESS);
    }

    if (param.dsn->answer == NULL) {
	G_fatal_error(_("Required parameter <%s> not set"), param.dsn->key);
    }

    driver_name = db_get_default_driver_name();

    if (driver_name && strcmp(driver_name, "pg") == 0)
	datetime_type = "timestamp";
    else if (driver_name && strcmp(driver_name, "dbf") == 0)
	datetime_type = "varchar(22)";
    else
	datetime_type = "datetime";

    dsn = NULL;
    if (param.dsn->answer)
        dsn = G_store(param.dsn->answer);
    
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

    /* GDAL configuration options */
    if (param.cfg->answer) {
	char **tokens, *tok, *key, *value;
	int ntokens;

	tokens = G_tokenize(param.cfg->answer, ",");
	ntokens = G_number_of_tokens(tokens);
	for (i = 0; i < ntokens; i++) {
	    G_debug(1, "%d=[%s]", i, tokens[i]);
	    tok = G_store(tokens[i]);
	    G_squeeze(tok);
	    key = tok;
	    value = strstr(tok, "=");
	    if (value) {
		*value = '\0';
		value++;
		CPLSetConfigOption(key, value);
	    }
	    G_free(tok);
	}
	G_free_tokens(tokens);
    }

    /* GDAL dataset open options */
    doo = NULL;
    if (param.doo->answer) {
	char **tokens;
	int ntokens;

	tokens = G_tokenize(param.doo->answer, ",");
	ntokens = G_number_of_tokens(tokens);
	doo = G_malloc(sizeof(char *) * (ntokens + 1));
	for (i = 0; i < ntokens; i++) {
	    G_debug(1, "%d=[%s]", i, tokens[i]);
	    doo[i] = G_store(tokens[i]);
	}
	G_free_tokens(tokens);
	doo[ntokens] = NULL;
    }

    /* open OGR DSN */
    Ogr_ds = NULL;
    if (strlen(dsn) > 0) {
#if GDAL_VERSION_NUM >= 2020000
	Ogr_ds = GDALOpenEx(dsn, GDAL_OF_VECTOR, NULL,
	                    (const char **) doo, NULL);
#else
	Ogr_ds = OGROpen(dsn, FALSE, NULL);
#endif
    }
    if (Ogr_ds == NULL)
	G_fatal_error(_("Unable to open data source <%s>"), dsn);

    /* driver name */
#if GDAL_VERSION_NUM >= 2020000
    ogr_driver_name = GDALGetDriverShortName(GDALGetDatasetDriver(Ogr_ds));
    G_verbose_message(_("Using OGR driver '%s/%s'"), ogr_driver_name,
                      GDALGetDriverLongName(GDALGetDatasetDriver(Ogr_ds)));
#else
    ogr_driver_name = OGR_Dr_GetName(OGR_DS_GetDriver(Ogr_ds));
    G_verbose_message(_("Using OGR driver '%s'"), ogr_driver_name);
#endif

    /* OGR interleaved reading */
    ogr_interleaved_reading = 0;
    if (strcmp(ogr_driver_name, "OSM") == 0) {

	/* re-open OGR DSN */
#if GDAL_VERSION_NUM < 2020000
	CPLSetConfigOption("OGR_INTERLEAVED_READING", "YES");
	OGR_DS_Destroy(Ogr_ds);
	Ogr_ds = OGROpen(dsn, FALSE, NULL);
#endif
	ogr_interleaved_reading = 1;
    }
    if (strcmp(ogr_driver_name, "GMLAS") == 0)
	ogr_interleaved_reading = 1;
    if (ogr_interleaved_reading)
	G_verbose_message(_("Using interleaved reading mode"));

    if (param.geom->answer) {
#if GDAL_VERSION_NUM >= 1110000
#if GDAL_VERSION_NUM >= 2020000
        if (!GDALDatasetTestCapability(Ogr_ds, ODsCCreateGeomFieldAfterCreateLayer)) {
            G_warning(_("Option <%s> will be ignored. OGR doesn't support it for selected format (%s)."),
                      param.geom->key, ogr_driver_name);
#else
        if (!OGR_DS_TestCapability(Ogr_ds, ODsCCreateGeomFieldAfterCreateLayer)) {
            G_warning(_("Option <%s> will be ignored. OGR doesn't support it for selected format (%s)."),
                      param.geom->key, ogr_driver_name);
#endif
            param.geom->answer = NULL;
        }
#else
        G_warning(_("Option <%s> will be ignored. Multiple geometry fields are supported by GDAL >= 1.11"),
                  param.geom->key);
        param.geom->answer = NULL;
#endif
    }

    /* check encoding for given driver */
    if (param.encoding->answer) {
        if (strcmp(ogr_driver_name, "ESRI Shapefile") != 0 &&
            strcmp(ogr_driver_name, "DXF") != 0)
            G_warning(_("Encoding value not supported by OGR driver <%s>"), ogr_driver_name);
    }

#if GDAL_VERSION_NUM >= 2020000
    navailable_layers = GDALDatasetGetLayerCount(Ogr_ds);
#else
    navailable_layers = OGR_DS_GetLayerCount(Ogr_ds);
#endif

    if (navailable_layers < 1)
	G_fatal_error(_("No OGR layers available"));

    /* make a list of available layers */
    available_layer_names =
	(char **)G_malloc(navailable_layers * sizeof(char *));

    if (flag.list->answer) {
	G_message(_("Data source <%s> (format '%s') contains %d layers:"),
		  dsn, ogr_driver_name, navailable_layers);
    }
    for (i = 0; i < navailable_layers; i++) {
	Ogr_layer = ds_getlayerbyindex(Ogr_ds, i);
	Ogr_featuredefn = OGR_L_GetLayerDefn(Ogr_layer);
        
	available_layer_names[i] =
	    G_store((char *)OGR_FD_GetName(Ogr_featuredefn));

	if (flag.list->answer)
	    fprintf(stdout, "%s\n", available_layer_names[i]);
    }
    if (flag.list->answer) {
	fflush(stdout);
	ds_close(Ogr_ds);
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

    /* compare SRS of the different layers to be imported */
    if (cmp_layer_srs(Ogr_ds, nlayers, layers, layer_names, param.geom->answer)) {
	ds_close(Ogr_ds);
	G_fatal_error(_("Detected different projections of input layers. "
	                "Input layers must be imported separately."));
    }

    G_get_window(&cellhd);

    cellhd.north = 1.;
    cellhd.south = 0.;
    cellhd.west = 0.;
    cellhd.east = 1.;
    cellhd.top = 1.;
    cellhd.bottom = 0.;
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

    /* check projection match */
    check_projection(&cellhd, Ogr_ds, layers[0], param.geom->answer,
		     param.outloc->answer,
                     flag.no_import->answer, flag.over->answer,
		     flag.proj->answer);

    /* get output name */
    if (param.out->answer) {
	output = G_store(param.out->answer);
    }
    else {
	output = G_store(layer_names[0]);
    }

    /* check output name */
    if (Vect_legal_filename(output) != 1) {
	ds_close(Ogr_ds);
	G_fatal_error(_("Illegal output name <%s>"), output);
    }

    /* Check if the output map exists */
    if (G_find_vector2(output, G_mapset()) && !overwrite) {
	ds_close(Ogr_ds);
	G_fatal_error(_("Vector map <%s> already exists"),
		      output);
    }

    /* report back if several layers will be imported because 
     * none has been selected */
    if (nlayers > 1 && param.layer->answer == NULL) {
	void (*msg_fn)(const char *, ...);

	/* make it a warning if output name has not been specified */
	if (param.out->answer)
	    msg_fn = G_important_message;
	else
	    msg_fn = G_warning;

	msg_fn(_("All available OGR layers will be imported into vector map <%s>"),
		  output);
    }

    /* attribute filter */
    attr_filter = param.where->answer;

    /* create spatial filters */
    if (param.outloc->answer && flag.region->answer) {
	G_warning(_("When creating a new location, the current region "
	          "can not be used as spatial filter, disabling"));
	flag.region->answer = 0;
    }
    if (flag.region->answer && param.spat->answer)
	G_fatal_error(_("Select either the current region flag or the spatial option, not both"));

    poSpatialFilter = G_malloc(nlayers * sizeof(OGRGeometryH));
    if (create_spatial_filter(Ogr_ds, poSpatialFilter,
                              nlayers, layers, layer_names,
			      &xmin, &ymin, &xmax, &ymax,
			      flag.region->answer, param.spat)
	|| attr_filter) {

	for (layer = 0; layer < nlayers; layer++) {
	    Ogr_layer = ds_getlayerbyindex(Ogr_ds, layers[layer]);
#if GDAL_VERSION_NUM >= 1110000
	    if (param.geom->answer) {
		Ogr_featuredefn = OGR_L_GetLayerDefn(Ogr_layer);
		igeom = OGR_FD_GetGeomFieldIndex(Ogr_featuredefn, param.geom->answer);
		if (igeom < 0)
		    G_fatal_error(_("Geometry column <%s> not found in input layer <%s>"),
				  param.geom->answer, OGR_L_GetName(Ogr_layer));

		OGR_L_SetSpatialFilterEx(Ogr_layer, igeom, poSpatialFilter[layer]);
	    }
	    else {
		OGR_L_SetSpatialFilter(Ogr_layer, poSpatialFilter[layer]);
	    }
#else
	    OGR_L_SetSpatialFilter(Ogr_layer, poSpatialFilter[layer]);
#endif
	    if (OGR_L_SetAttributeFilter(Ogr_layer, attr_filter) != OGRERR_NONE)
		G_fatal_error(_("Error setting attribute filter '%s'"),
			      attr_filter);
	}
    }

    /* suppress boundary splitting ? */
    if (flag.no_clean->answer || xmin >= xmax || ymin >= ymax) {
	split_distance = -1.;
	area_size = -1;
    }
    else {
	split_distance = 0.;
	area_size = sqrt((xmax - xmin) * (ymax - ymin));
    }

    db_init_string(&sql);
    db_init_string(&strval);

    n_features = (GIntBig *)G_malloc(nlayers * sizeof(GIntBig));

    OGR_iterator_init(&OGR_iter, Ogr_ds, dsn, navailable_layers,
		      ogr_interleaved_reading);

    /* check if input id 3D and if we need a tmp vector */
    /* estimate distance for boundary splitting --> */
    n_polygon_boundaries = 0;
    input3d = 0;

    for (layer = 0; layer < nlayers; layer++) {
	GIntBig ogr_feature_count;

	n_features[layer] = 0;
	layer_id = layers[layer];
	Ogr_layer = ds_getlayerbyindex(Ogr_ds, layer_id);
	Ogr_featuredefn = OGR_L_GetLayerDefn(Ogr_layer);
        igeom = -1;
#if GDAL_VERSION_NUM >= 1110000
        if (param.geom->answer) {
            igeom = OGR_FD_GetGeomFieldIndex(Ogr_featuredefn, param.geom->answer);
            if (igeom < 0)
                G_fatal_error(_("Geometry column <%s> not found in OGR layer <%s>"),
                              param.geom->answer, OGR_L_GetName(Ogr_layer));
        }
#endif
	feature_count = 0;

	ogr_feature_count = 0;
	if (n_features[layer] == 0)
	    ogr_feature_count = OGR_L_GetFeatureCount(Ogr_layer, 1);
	if (ogr_feature_count > 0)
	    n_features[layer] = ogr_feature_count;

	/* count polygons and isles */
	G_message(_("Check if OGR layer <%s> contains polygons..."),
		  layer_names[layer]);
	while ((Ogr_feature = ogr_getnextfeature(&OGR_iter, layer_id,
	                                         layer_names[layer],
						 poSpatialFilter[layer],
						 attr_filter)) != NULL) {
	    if (ogr_feature_count > 0)
		G_percent(feature_count++, n_features[layer], 1);	/* show something happens */

	    if (ogr_feature_count <= 0)
		n_features[layer]++;

            /* Geometry */
#if GDAL_VERSION_NUM >= 1110000
            Ogr_featuredefn = OGR_iter.Ogr_featuredefn;
            for (i = 0; i < OGR_FD_GetGeomFieldCount(Ogr_featuredefn); i++) {
                if (igeom > -1 && i != igeom)
                    continue; /* use only geometry defined via param.geom */
            
                Ogr_geometry = OGR_F_GetGeomFieldRef(Ogr_feature, i);
#else
                Ogr_geometry = OGR_F_GetGeometryRef(Ogr_feature);
#endif
                if (Ogr_geometry != NULL) {
#if GDAL_VERSION_NUM >= 2000000
		    Ogr_geometry = OGR_G_GetLinearGeometry(Ogr_geometry, 0, NULL);
		}
                if (Ogr_geometry != NULL) {
#endif
                    if (!flag.no_clean->answer)
                        poly_count(Ogr_geometry, (type & GV_BOUNDARY));
                    if (OGR_G_GetCoordinateDimension(Ogr_geometry) > 2)
                        input3d = 1;
#if GDAL_VERSION_NUM >= 2000000
		    OGR_G_DestroyGeometry(Ogr_geometry);
#endif
                }
#if GDAL_VERSION_NUM >= 1110000                
            }
#endif
            OGR_F_Destroy(Ogr_feature);
	}
	G_percent(1, 1, 1);
    }

    n_import_features = 0;
    for (i = 0; i < nlayers; i++)
	n_import_features += n_features[i];
    if (nlayers > 1)
	G_message("Importing %lld features", n_import_features);

    G_debug(1, "n polygon boundaries: %d", n_polygon_boundaries);
    if (area_size > 0 && n_polygon_boundaries > 50) {
	split_distance =
	    area_size / log(n_polygon_boundaries);
	/* divisor is the handle: increase divisor to decrease split_distance */
	split_distance = split_distance / 16.;
	G_debug(1, "root of area size: %f", area_size);
	G_verbose_message(_("Boundary splitting distance in map units: %G"),
		  split_distance);
    }
    /* <-- estimate distance for boundary splitting */

    use_tmp_vect = (n_polygon_boundaries > 0) || (strcmp(ogr_driver_name, "OSM") == 0);

    G_debug(1, "Input is 3D ? %s", (input3d == 0 ? "yes" : "no"));
    with_z = input3d;
    if (with_z)
	with_z = !flag.force2d->answer;

    /* open output vector */
    /* strip any @mapset from vector output name */
    G_find_vector(output, G_mapset());

    if (Vect_open_new(&Map, output, with_z) < 0)
	G_fatal_error(_("Unable to create vector map <%s>"), output);

    Out = &Map;

    if (!flag.no_clean->answer) {
	if (use_tmp_vect) {
	    /* open temporary vector, do the work in the temporary vector
	     * at the end copy alive lines to output vector
	     * in case of polygons this reduces the coor file size by a factor of 2 to 5
	     * only needed when cleaning polygons */
	    if (Vect_open_tmp_new(&Tmp, NULL, with_z) < 0)
		G_fatal_error(_("Unable to create temporary vector map"));

	    G_verbose_message(_("Using temporary vector <%s>"), Vect_get_name(&Tmp));
	    Out = &Tmp;
	}
    }

    Vect_hist_command(&Map);

    ncentr = n_overlaps = n_polygons = 0;
    failed_centr = 0;

    G_begin_polygon_area_calculations();	/* Used in geom() and centroid() */

    /* Points and lines are written immediately with categories. Boundaries of polygons are
     * written to the vector then cleaned and centroids are calculated for all areas in clean vector.
     * Then second pass through finds all centroids in each polygon feature and adds its category
     * to the centroid. The result is that one centroid may have 0, 1 ore more categories
     * of one ore more (more input layers) fields. */

    /* get input column to use for categoy values, create tables */
    OGR_iterator_reset(&OGR_iter);
    key_column = G_malloc(nlayers * sizeof(char *));
    key_idx = G_malloc(nlayers * sizeof(int));
    for (layer = 0; layer < nlayers; layer++) {

	key_column[layer] = GV_KEY_COLUMN;
	key_idx[layer] = -2; /* -1 for fid column */
	layer_id = layers[layer];
	Ogr_layer = ds_getlayerbyindex(Ogr_ds, layer_id);
	Ogr_featuredefn = OGR_L_GetLayerDefn(Ogr_layer);

        if (param.key->answer) {
	    /* use existing column for category values */
            const char *fid_column;

            fid_column = OGR_L_GetFIDColumn(Ogr_layer);
            if (fid_column) {
                key_column[layer] = G_store(fid_column);
                key_idx[layer] = -1;
            }
            if (!fid_column || strcmp(fid_column, param.key->answer) != 0) {
                key_idx[layer] = OGR_FD_GetFieldIndex(Ogr_featuredefn, param.key->answer);
                if (key_idx[layer] == -1)
                    G_fatal_error(_("Key column '%s' not found in input layer <%s>"),
		                  param.key->answer, layer_names[layer]);
            }

            if (key_idx[layer] > -1) {
                /* check if the field is integer */
                Ogr_field = OGR_FD_GetFieldDefn(Ogr_featuredefn, key_idx[layer]);
                Ogr_ftype = OGR_Fld_GetType(Ogr_field);
                if (!(Ogr_ftype == OFTInteger
#if GDAL_VERSION_NUM >= 2000000
                      || Ogr_ftype == OFTInteger64
#endif
		      )) {
                    G_fatal_error(_("Key column '%s' in input layer <%s> is not integer"),
		                  param.key->answer, layer_names[layer]);
                }
                key_column[layer] = G_store(OGR_Fld_GetNameRef(Ogr_field));
            }
        }

	/* Add DB link and create table */
	if (!flag.notab->answer) {
	    int i_out, ncols_out, done;
	    struct grass_col_info *col_info;

	    G_important_message(_("Creating attribute table for layer <%s>..."),
				  layer_names[layer]);

	    if (nlayers == 1) {	/* one layer only */
		Fi = Vect_default_field_info(&Map, layer + 1, NULL,
					     GV_1TABLE);
	    }
	    else {
		Fi = Vect_default_field_info(&Map, layer + 1, NULL,
					     GV_MTABLE);
	    }

	    if (ncnames > 0) {
		key_column[layer] = param.cnames->answers[0];
	    }
	    Vect_map_add_dblink(&Map, layer + 1, layer_names[layer], Fi->table,
				key_column[layer], Fi->database, Fi->driver);

	    ncols = OGR_FD_GetFieldCount(Ogr_featuredefn);
	    G_debug(2, "%d columns", ncols);

	    ncols_out = ncols;
	    if (key_idx[layer] < 0)
		ncols_out++;
	    
	    col_info = G_malloc(ncols_out * sizeof(struct grass_col_info));

	    /* Collect column names and types */
	    i_out = 0;
	    col_info[i_out].idx = i_out;
	    col_info[i_out].name = key_column[layer];
	    col_info[i_out].type = "integer";

	    for (i = 0; i < ncols; i++) {

                if (key_idx[layer] > -1 && key_idx[layer] == i)
                    continue; /* skip defined key (FID column) */

		i_out++;
                
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

		/* avoid that we get the key column twice */
		if (strcmp(Ogr_fieldname, key_column[layer]) == 0) {
		    sprintf(namebuf, "%s_", Ogr_fieldname);
		    Ogr_fieldname = G_store(namebuf);
		}

		/* capital column names are a pain in SQL */
		if (flag.tolower->answer)
		    G_str_to_lower(Ogr_fieldname);

		if (strcmp(OGR_Fld_GetNameRef(Ogr_field), Ogr_fieldname) != 0) {
		    G_important_message(_("Column name <%s> renamed to <%s>"),
			      OGR_Fld_GetNameRef(Ogr_field), Ogr_fieldname);
		}
		col_info[i_out].idx = i_out;
		col_info[i_out].name = G_store(Ogr_fieldname);

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
                /** GDAL 2.0+                                                      **/
                /** Simple 64bit integer                     OFTInteger64 = 12     **/
                /** List of 64bit integers                   OFTInteger64List = 13 **/

		if (Ogr_ftype == OFTInteger) {
		    col_info[i_out].type = "integer";
		}
#if GDAL_VERSION_NUM >= 2000000
		else if (Ogr_ftype == OFTInteger64) {
                    if (strcmp(Fi->driver, "pg") == 0) 
			col_info[i_out].type = "bigint";
                    else {
			col_info[i_out].type = "integer";
                        if (strcmp(Fi->driver, "sqlite") != 0) 
                            G_warning(_("Writing column <%s> with integer 64 as integer 32"),
                                      Ogr_fieldname);
                    }
                }
#endif
		else if (Ogr_ftype == OFTIntegerList
#if GDAL_VERSION_NUM >= 2000000
                         || Ogr_ftype == OFTInteger64List
#endif
                         ) {
		    /* hack: treat as string */
		    sprintf(buf, "varchar ( %d )", OFTIntegerListlength);
		    col_info[i_out].type = G_store(buf);
		    G_warning(_("Writing column <%s> with fixed length %d chars (may be truncated)"),
			      Ogr_fieldname, OFTIntegerListlength);
		}
		else if (Ogr_ftype == OFTReal) {
		    col_info[i_out].type = "double precision";
#if GDAL_VERSION_NUM >= 1320
		}
		else if (Ogr_ftype == OFTDate) {
		    col_info[i_out].type = "date";
		}
		else if (Ogr_ftype == OFTTime) {
		    col_info[i_out].type = "time";
		}
		else if (Ogr_ftype == OFTDateTime) {
		    sprintf(buf, "%s", datetime_type);
		    col_info[i_out].type = G_store(buf);
#endif
		}
		else if (Ogr_ftype == OFTString) {
		    int fwidth;

		    fwidth = OGR_Fld_GetWidth(Ogr_field);
		    /* TODO: read all records first and find the longest string length */
		    if (fwidth == 0 && strcmp(Fi->driver, "dbf") == 0) {
			G_warning(_("Width for column %s set to 255 (was not specified by OGR), "
				   "some strings may be truncated!"),
				  Ogr_fieldname);
			fwidth = 255;
		    }
		    if (fwidth == 0) {
			col_info[i_out].type = "text";
		    }
		    else {
			sprintf(buf, "varchar ( %d )", fwidth);
			col_info[i_out].type = G_store(buf);
		    }
		}
		else if (Ogr_ftype == OFTStringList) {
		    /* hack: treat as string */
		    sprintf(buf, "varchar ( %d )", OFTIntegerListlength);
		    col_info[i_out].type = G_store(buf);
		    G_warning(_("Writing column %s with fixed length %d chars (may be truncated)"),
			      Ogr_fieldname, OFTIntegerListlength);
		}
		else {
		    G_warning(_("Column type (Ogr_ftype: %d) not supported (Ogr_fieldname: %s)"),
			      Ogr_ftype, Ogr_fieldname);
		    buf[0] = 0;
		    col_info[i_out].type = G_store(buf);
		}
		G_free(Ogr_fieldname);
	    }

	    /* fix duplicate column names */
	    done = 0;

	    while (!done) {
		done = 1;
		qsort(col_info, ncols_out, sizeof(struct grass_col_info),
		      cmp_col_name);
		for (i = 0; i < ncols_out - 1; i++) {
		    int i_a;

		    i_a = 1;
		    while (i + i_a < ncols_out &&
			   strcmp(col_info[i].name, col_info[i + i_a].name) == 0) {
			G_important_message(_("Column name <%s> renamed to <%s_%d>"),
					    col_info[i + i_a].name, 
					    col_info[i + i_a].name, i_a);
			sprintf(buf, "%s_%d", col_info[i + i_a].name, i_a);
			col_info[i + i_a].name = G_store(buf);
			i_a++;
			done = 0;
		    }
		}
	    }
	    qsort(col_info, ncols_out, sizeof(struct grass_col_info),
		  cmp_col_idx);

	    /* Create table */
	    i = 0;
	    sprintf(buf, "create table %s (%s %s", Fi->table,
		    col_info[i].name, col_info[i].type);
	    db_set_string(&sql, buf);

	    for (i = 1; i < ncols_out; i++) {
		sprintf(buf, ", %s %s", col_info[i].name, col_info[i].type);
		db_append_string(&sql, buf);
	    }

	    db_append_string(&sql, ")");
	    G_debug(3, "%s", db_get_string(&sql));

	    driver =
		db_start_driver_open_database(Fi->driver,
					      Vect_subst_var(Fi->database,
							     &Map));
	    if (driver == NULL) {
		G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			      Vect_subst_var(Fi->database, &Map), Fi->driver);
	    }

	    if (db_execute_immediate(driver, &sql) != DB_OK) {
		db_close_database(driver);
		db_shutdown_driver(driver);
		G_fatal_error(_("Unable to create table: '%s'"),
			      db_get_string(&sql));
	    }

	    if (db_grant_on_table
		(driver, Fi->table, DB_PRIV_SELECT,
		 DB_GROUP | DB_PUBLIC) != DB_OK)
		G_fatal_error(_("Unable to grant privileges on table <%s>"),
			      Fi->table);

	    db_close_database_shutdown_driver(driver);
	    
	    G_free(col_info);
	}
    }

    /* import features */
    sqlbuf = NULL;
    sqlbufsize = 0;
    OGR_iterator_reset(&OGR_iter);
    fid_cat_tree = G_malloc(nlayers * sizeof(struct pavl_table *));
    for (layer = 0; layer < nlayers; layer++) {
	layer_id = layers[layer];
	/* Import features */
	cat = 1;
	nogeom = 0;
	feature_count = 0;

	G_important_message(_("Importing %lld features (OGR layer <%s>)..."),
			    n_features[layer], layer_names[layer]);

	/* balanced binary search tree to map FIDs to cats */
	fid_cat_tree[layer] = pavl_create(cmp_fid_cat, NULL);

	driver = NULL;
	if (!flag.notab->answer) {
	    /* one transaction per layer/table
	     * or better one transaction for all layers/tables together ?
	     */
	    Fi = Vect_get_field(&Map, layer + 1);
	    driver =
		db_start_driver_open_database(Fi->driver,
					      Vect_subst_var(Fi->database,
							     &Map));
	    if (driver == NULL) {
		G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			      Vect_subst_var(Fi->database, &Map), Fi->driver);
	    }
	    db_begin_transaction(driver);
	}

	Ogr_layer = ds_getlayerbyindex(Ogr_ds, layer_id);
	Ogr_featuredefn = OGR_L_GetLayerDefn(Ogr_layer);

        igeom = -1;
#if GDAL_VERSION_NUM >= 1110000
        if (param.geom->answer)
            igeom = OGR_FD_GetGeomFieldIndex(Ogr_featuredefn, param.geom->answer);
#endif

	while ((Ogr_feature = ogr_getnextfeature(&OGR_iter, layer_id,
	                                         layer_names[layer],
						 poSpatialFilter[layer],
						 attr_filter)) != NULL) {
	    grass_int64 ogr_fid;

	    G_percent(feature_count++, n_features[layer], 1);	/* show something happens */

	    /* get feature ID */
	    ogr_fid = OGR_F_GetFID(Ogr_feature);
	    if (ogr_fid != OGRNullFID) {
		/* map feature id to cat */
		new_fid_cat = G_malloc(sizeof(struct fid_cat));
		new_fid_cat->fid = ogr_fid;
		new_fid_cat->cat = cat;
		pavl_insert(fid_cat_tree[layer], new_fid_cat);
	    }

            /* Geometry */
            Ogr_featuredefn = OGR_iter.Ogr_featuredefn;
#if GDAL_VERSION_NUM >= 1110000
            for (i = 0; i < OGR_FD_GetGeomFieldCount(Ogr_featuredefn); i++) {
                if (igeom > -1 && i != igeom)
                    continue; /* use only geometry defined via param.geom */

		/* Ogr_geometry from OGR_F_GetGeomFieldRef() should not be modified. */
                Ogr_geometry = OGR_F_GetGeomFieldRef(Ogr_feature, i);
#else
                Ogr_geometry = OGR_F_GetGeometryRef(Ogr_feature);
#endif
                if (Ogr_geometry == NULL) {
                    nogeom++;
                }
                else {
                    if (key_idx[layer] > -1)
                        cat = OGR_F_GetFieldAsInteger(Ogr_feature, key_idx[layer]);
                    else if (key_idx[layer] == -1)
                        cat = OGR_F_GetFID(Ogr_feature);

                    geom(Ogr_geometry, Out, layer + 1, cat, min_area, type,
                         flag.no_clean->answer);
                }
#if GDAL_VERSION_NUM >= 1110000              
            }
#endif
	    /* Attributes */
	    ncols = OGR_FD_GetFieldCount(Ogr_featuredefn);
	    if (!flag.notab->answer) {
		G_rasprintf(&sqlbuf, &sqlbufsize, "insert into %s values ( %d", Fi->table, cat);
		db_set_string(&sql, sqlbuf);
		for (i = 0; i < ncols; i++) {
		    const char *Ogr_fstring = NULL;

                    if (key_idx[layer] > -1 && key_idx[layer] == i)
                        continue; /* skip defined key (FID column) */

		    Ogr_field = OGR_FD_GetFieldDefn(Ogr_featuredefn, i);
		    Ogr_ftype = OGR_Fld_GetType(Ogr_field);
		    if (OGR_F_IsFieldSet(Ogr_feature, i))
			Ogr_fstring = OGR_F_GetFieldAsString(Ogr_feature, i);
		    if (Ogr_fstring && *Ogr_fstring) {
			if (Ogr_ftype == OFTInteger ||
#if GDAL_VERSION_NUM >= 2000000
                            Ogr_ftype == OFTInteger64 ||
#endif
                            Ogr_ftype == OFTReal) {
			    G_rasprintf(&sqlbuf, &sqlbufsize, ", %s", Ogr_fstring);
			}
#if GDAL_VERSION_NUM >= 1320
			    /* should we use OGR_F_GetFieldAsDateTime() here ? */
			else if (Ogr_ftype == OFTDate || Ogr_ftype == OFTTime
				 || Ogr_ftype == OFTDateTime) {
			    char *newbuf;

			    db_set_string(&strval, (char *)Ogr_fstring);
			    db_double_quote_string(&strval);
			    G_rasprintf(&sqlbuf, &sqlbufsize, ", '%s'", db_get_string(&strval));
			    newbuf = G_str_replace(sqlbuf, "/", "-");	/* fix 2001/10/21 to 2001-10-21 */
			    G_rasprintf(&sqlbuf, &sqlbufsize, "%s", newbuf);
			    G_free(newbuf);
			}
#endif
			else if (Ogr_ftype == OFTString ||
			         Ogr_ftype == OFTStringList ||
				 Ogr_ftype == OFTIntegerList 
#if GDAL_VERSION_NUM >= 2000000
                                 || Ogr_ftype == OFTInteger64List
#endif
                                 ) {
			    db_set_string(&strval, (char *)Ogr_fstring);
			    db_double_quote_string(&strval);
			    G_rasprintf(&sqlbuf, &sqlbufsize, ", '%s'", db_get_string(&strval));
			}
			else {
			    /* column type not supported */
			    G_rasprintf(&sqlbuf, &sqlbufsize, "%c", '\0');
			}
		    }
		    else {
			/* G_warning (_("Column value not set" )); */
			if (Ogr_ftype == OFTInteger ||
#if GDAL_VERSION_NUM >= 2000000
                            Ogr_ftype == OFTInteger64 ||
#endif
                            Ogr_ftype == OFTReal) {
			    G_rasprintf(&sqlbuf, &sqlbufsize, ", NULL");
			}
#if GDAL_VERSION_NUM >= 1320
			else if (Ogr_ftype == OFTDate ||
				 Ogr_ftype == OFTTime || 
				 Ogr_ftype == OFTDateTime) {
			    G_rasprintf(&sqlbuf, &sqlbufsize, ", NULL");
			}
#endif
			else if (Ogr_ftype == OFTString ||
			         Ogr_ftype == OFTStringList ||
				 Ogr_ftype == OFTIntegerList
#if GDAL_VERSION_NUM >= 2000000
                                 || Ogr_ftype == OFTInteger64List
#endif
                                 ) {
			    G_rasprintf(&sqlbuf, &sqlbufsize, ", NULL");
			}
			else {
			    /* column type not supported */
			    G_rasprintf(&sqlbuf, &sqlbufsize, "%c", '\0');
			}
		    }
		    if (strlen(sqlbuf) >= DB_SQL_MAX) {
			G_debug(1, "%s", sqlbuf);
			G_debug(1, "Field %d is %ld long", i, strlen(sqlbuf));
		    }
		    db_append_string(&sql, sqlbuf);
		}
		db_append_string(&sql, " )");
		G_debug(3, "%s", db_get_string(&sql));

		if (db_execute_immediate(driver, &sql) != DB_OK) {
		    db_close_database(driver);
		    db_shutdown_driver(driver);
		    G_fatal_error(_("Cannot insert new row for input layer <%s>: %s"),
				  layer_names[layer], db_get_string(&sql));
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
	    G_warning(_("%d %s without geometry in input layer <%s> skipped"),
	              nogeom, nogeom == 1 ? _("feature") : _("features"),
		      layer_names[layer]);
    }

    delete_table = Vect_maptype(&Map) != GV_FORMAT_NATIVE;

    /* create index - must fail on non-unique categories */
    if (!flag.notab->answer) {
	for (layer = 0; layer < nlayers; layer++) {
	    Fi = Vect_get_field(&Map, layer + 1);
	    driver =
		db_start_driver_open_database(Fi->driver,
					      Vect_subst_var(Fi->database,
							     &Map));

	    if (!delete_table) {
		if (db_create_index2(driver, Fi->table, Fi->key) != DB_OK)
		    G_fatal_error(_("Unable to create index for table <%s>, key <%s>"),
			      Fi->table, Fi->key);
	    }
	    else {
		sprintf(buf, "drop table %s", Fi->table);
		db_set_string(&sql, buf);
		if (db_execute_immediate(driver, &sql) != DB_OK) {
		    G_fatal_error(_("Unable to drop table: '%s'"),
				  db_get_string(&sql));
		}
	    }
	    db_close_database_shutdown_driver(driver);
	}
    }
    /* attribute tables are now done */

    separator = "-----------------------------------------------------";
    G_message("%s", separator);

    if (use_tmp_vect) {
	/* TODO: is it necessary to build here? probably not, consumes time */
	/* GV_BUILD_BASE is sufficient to toggle boundary cleaning */
	Vect_build_partial(&Tmp, GV_BUILD_BASE);
    }

    if (use_tmp_vect && !flag.no_clean->answer &&
        strcmp(ogr_driver_name, "OSM") == 0 &&
	Vect_get_num_primitives(Out, GV_LINE) > 0) {
	convert_osm_lines(Out, snap);
    }

    /* make this a separate function ?
     * no, too many arguments */
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

	/* the principal purpose is to convert non-topological polygons to 
	 * topological areas */
	G_message(_("Cleaning polygons"));

	if (snap >= 0) {
	    G_message("%s", separator);
	    G_message(_("Snapping boundaries (threshold = %.3e)..."), snap);
	    Vect_snap_lines(&Tmp, GV_BOUNDARY, snap, NULL);
	}

	/* It is not to clean to snap centroids, but I have seen data with 2 duplicate polygons
	 * (as far as decimal places were printed) and centroids were not identical */
	/* Disabled, because the mechanism has changed:
	 * at this stage, there are no centroids yet, centroids are calculated 
	 * later for output areas, not fo input polygons */
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
		failed_centr++;
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
	OGR_iterator_reset(&OGR_iter);
	for (layer = 0; layer < nlayers; layer++) {
	    int do_fid_warning = 1;

	    G_message("%s", separator);
	    G_message(_("Finding centroids for OGR layer <%s>..."), layer_names[layer]);
	    layer_id = layers[layer];
	    Ogr_layer = ds_getlayerbyindex(Ogr_ds, layer_id);
	    Ogr_featuredefn = OGR_L_GetLayerDefn(Ogr_layer);

	    igeom = -1;
#if GDAL_VERSION_NUM >= 1110000
	    if (param.geom->answer)
		igeom = OGR_FD_GetGeomFieldIndex(Ogr_featuredefn, param.geom->answer);
#endif

	    cat = 0;		/* field = layer + 1 */
	    feature_count = 0;
	    while ((Ogr_feature = ogr_getnextfeature(&OGR_iter, layer_id,
						     layer_names[layer],
						     poSpatialFilter[layer],
						     attr_filter)) != NULL) {
		int area_cat;

		G_percent(feature_count++, n_features[layer], 2);

		area_cat = ++cat;

		/* Category */
		if (key_idx[layer] > -1)
		    area_cat = OGR_F_GetFieldAsInteger(Ogr_feature, key_idx[layer]);
		else {
		    /* get feature ID */
		    grass_int64 ogr_fid = OGR_F_GetFID(Ogr_feature);

		    if (ogr_fid != OGRNullFID) {
			find_fid_cat.fid = ogr_fid;
			/* the order of features might have changed from 
			 * the first pass through the features: 
			 * find cat for FID as assigned in the first pass */
			if ((new_fid_cat = pavl_find(fid_cat_tree[layer], &find_fid_cat)) != NULL) {
			    area_cat = new_fid_cat->cat;
			    if (do_fid_warning && area_cat != cat) {
				G_warning(_("The order of features in input layer <%s> has changed"),
					  layer_names[layer]);
				do_fid_warning = 0;
			    }
			}
		    }
		}

		/* Geometry */
#if GDAL_VERSION_NUM >= 1110000
		Ogr_featuredefn = OGR_iter.Ogr_featuredefn;
		for (i = 0; i < OGR_FD_GetGeomFieldCount(Ogr_featuredefn); i++) {
		    if (igeom > -1 && i != igeom)
			continue; /* use only geometry defined via param.geom */
	    
		    Ogr_geometry = OGR_F_GetGeomFieldRef(Ogr_feature, i);
#else
		    Ogr_geometry = OGR_F_GetGeometryRef(Ogr_feature);
#endif
		    if (Ogr_geometry != NULL) {
			centroid(Ogr_geometry, Centr, &si, layer + 1, area_cat,
				 min_area, type);
		    }
#if GDAL_VERSION_NUM >= 1110000
		}
#endif
		OGR_F_Destroy(Ogr_feature);
	    }
	    /* search tree is no longer needed */
	    pavl_destroy(fid_cat_tree[layer], free_fid_cat);
	    G_percent(1, 1, 1);
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

    ds_close(Ogr_ds);
    G_free(fid_cat_tree);

    if (use_tmp_vect) {
	/* Copy temporary vector to output vector */
	Vect_copy_map_lines(&Tmp, &Map);
	/* release memory occupied by topo, we may need that memory for main output */
	Vect_set_release_support(&Tmp);
	Vect_close(&Tmp); /* temporary map is deleted automatically */
    }

    Vect_build(&Map);
#if 0
    /* disabled, Vect_topo_check() is quite slow */
    if (flag.no_clean->answer)
	Vect_topo_check(&Map, NULL);
#endif

    /* fast topology check */
    err_boundaries = err_centr_out = err_centr_dupl = 0;
    if (Vect_get_num_primitives(&Map, GV_BOUNDARY) > 0) {
	int line, nlines, ltype;

	nlines = Vect_get_num_lines(&Map);
	for (line = 1; line <= nlines; line++) {
	    if (!Vect_line_alive(&Map, line))
		continue;
	    
	    ltype = Vect_get_line_type(&Map, line);
	    if (ltype == GV_BOUNDARY) {
		int left, right;

		left = right = 0;
		Vect_get_line_areas(&Map, line, &left, &right);

		if (left == 0 || right == 0) {
		    err_boundaries++;
		}
	    }
	    else if (ltype == GV_CENTROID) {
		int area;
		
		area = 0;
		
		area = Vect_get_centroid_area(&Map, line);

		if (area == 0)
		    err_centr_out++;
		else if (area < 0)
		    err_centr_dupl++;
	    }
	}
    }

    /* test for topological errors */
    /* this test is not perfect:
     * small gaps (areas without centroid) are not detected
     * small gaps may also be true gaps */
    ncentr = Vect_get_num_primitives(&Map, GV_CENTROID);
    if (failed_centr || err_boundaries || err_centr_out || err_centr_dupl
        || ncentr != n_polygons || n_overlaps) {

	double min_snap, max_snap;
	int exp;

	Vect_get_map_box(&Map, &box);
	
	if (fabs(box.E) > fabs(box.W))
	    xmax = fabs(box.E);
	else
	    xmax = fabs(box.W);
	if (fabs(box.N) > fabs(box.S))
	    ymax = fabs(box.N);
	else
	    ymax = fabs(box.S);

	if (xmax < ymax)
	    xmax = ymax;

	/* double precision ULP */
	min_snap = frexp(xmax, &exp);
	exp -= 52;
	min_snap = ldexp(min_snap, exp);
	/* human readable */
	min_snap = log10(min_snap);
	if (min_snap < 0)
	    min_snap = (int)min_snap;
	else
	    min_snap = (int)min_snap + 1;
	min_snap = pow(10, min_snap);

	/* single precision ULP */
	max_snap = frexp(xmax, &exp);
	exp -= 23;
	max_snap = ldexp(max_snap, exp);
	/* human readable */
	max_snap = log10(max_snap);
	if (max_snap < 0)
	    max_snap = (int)max_snap;
	else
	    max_snap = (int)max_snap + 1;
	max_snap = pow(10, max_snap);

	/* topological errors are
	 * - areas too small / too thin to calculate a centroid
	 * - incorrect boundaries
	 * - duplicate area centroids
	 * - centroids outside any area
	 * 
	 * overlapping polygons are topological problems only
	 * if input polygons are not supposed to overlap
	 */

	G_important_message("%s", separator);
	/* topological errors */
	if (failed_centr || err_boundaries || err_centr_out || err_centr_dupl) {
	    char error_msg[8096];

	    strcpy(error_msg, _("The output contains topological errors:"));
	    if (failed_centr) {
		strcat(error_msg, "\n");
		sprintf(error_msg + strlen(error_msg),
		       _("Unable to calculate a centroid for %d areas"),
		       failed_centr);
	    }
	    if (err_boundaries) {
		strcat(error_msg, "\n");
		sprintf(error_msg + strlen(error_msg),
		        _("Number of incorrect boundaries: %d"),
			err_boundaries);
	    }
	    if (err_centr_out) {
		strcat(error_msg, "\n");
		sprintf(error_msg + strlen(error_msg),
		        _("Number of centroids outside area: %d"),
			err_centr_out);
	    }
	    if (err_centr_dupl) {
		strcat(error_msg, "\n");
		sprintf(error_msg + strlen(error_msg),
		        _("Number of duplicate centroids: %d"),
			err_centr_dupl);
	    }
	    G_warning("%s", error_msg);
	    
	    G_important_message(_("The input could be cleaned by snapping vertices to each other."));

	    if (snap < max_snap) {
		G_important_message(_("Estimated range of snapping threshold: [%g, %g]"), min_snap, max_snap);
	    }

	    if (snap < 0) {
		double e1, e2;

		/* human readable */
		e1 = log10(min_snap);
		e2 = log10(max_snap);
		e1 = (int)((e1 + e2) / 2. - 0.5);
		snap = pow(10, e1);
		G_important_message(_("Try to import again, snapping with %g: 'snap=%g'"), snap, snap);
	    }
	    else if (snap < min_snap) {
		G_important_message(_("Try to import again, snapping with at least %g: 'snap=%g'"), min_snap, min_snap);
	    }
	    else if (snap * 10 < max_snap) {
		min_snap = snap * 10;
		G_important_message(_("Try to import again, snapping with %g: 'snap=%g'"), min_snap, min_snap);
	    }
	    else
		/* assume manual cleaning is required */
		G_important_message(_("Manual cleaning may be needed."));
	}
	/* overlapping polygons */
	else if (n_overlaps) {
	    G_important_message(_("%d areas represent multiple (overlapping) features, because polygons overlap "
		       "in input layer(s). Such areas are linked to more than 1 row in attribute table. "
		       "The number of features for those areas is stored as category in layer %d"),
		       n_overlaps, nlayers + 1);
	    G_important_message("%s", separator);
	    G_important_message(_("If overlapping is not desired, the input data can be "
				  "cleaned by snapping vertices to each other."));

	    if (snap < max_snap) {
		G_important_message(_("Estimated range of snapping threshold: [%g, %g]"), min_snap, max_snap);
	    }

	    if (snap > 0) {
		if (snap < min_snap) {
		    G_important_message(_("Try to import again, snapping with at least %g: 'snap=%g'"), min_snap, min_snap);
		}
		else if (snap * 10 <= max_snap) {
		    min_snap = snap * 10;
		    G_important_message(_("Try to import again, snapping with %g: 'snap=%g'"), min_snap, min_snap);
		}
		else
		    /* assume manual cleaning is required */
		    G_important_message(_("Manual cleaning may be needed."));
	    }
	}
	/* number of centroids does not match number of input polygons */
	else if (ncentr != n_polygons) {
	    if (ncentr < n_polygons) {
		G_important_message(_("%d input polygons got lost during import."), n_polygons - ncentr);
	    }
	    if (ncentr > n_polygons) {
		G_important_message(_("%d additional areas where created during import."), ncentr - n_polygons);
	    }
	    if (snap > 0) {
		G_important_message(_("The snapping threshold %g might be too large."), snap);
		G_important_message(_("Estimated range of snapping threshold: [%g, %g]"), min_snap, max_snap);
		/* assume manual cleaning is required */
		G_important_message(_("Try to reduce the snapping threshold or clean the output manually."));
	    }
	    else {
		G_important_message(_("The input could be cleaned by snapping vertices to each other."));
		G_important_message(_("Estimated range of snapping threshold: [%g, %g]"), min_snap, max_snap);
	    }
	}
    }

    Vect_get_map_box(&Map, &box);
    if (0 != Vect_close(&Map))
        G_fatal_error(_("Import failed"));

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

	cur_wind.north = MAX(cur_wind.north, box.N);
	cur_wind.south = MIN(cur_wind.south, box.S);
	cur_wind.west = MIN(cur_wind.west, box.W);
	cur_wind.east = MAX(cur_wind.east, box.E);

	cur_wind.rows = (int)ceil((cur_wind.north - cur_wind.south)
				  / cur_wind.ns_res);
	cur_wind.south = cur_wind.north - cur_wind.rows * cur_wind.ns_res;

	cur_wind.cols = (int)ceil((cur_wind.east - cur_wind.west)
				  / cur_wind.ew_res);
	cur_wind.east = cur_wind.west + cur_wind.cols * cur_wind.ew_res;

	if (strcmp(G_mapset(), "PERMANENT") == 0) {
	    G_put_element_window(&cur_wind, "", "DEFAULT_WIND");
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

void OGR_iterator_init(struct OGR_iterator *OGR_iter, ds_t Ogr_ds,
                       char *dsn, int nlayers,
		       int ogr_interleaved_reading)
{
    OGR_iter->Ogr_ds = Ogr_ds;
    OGR_iter->dsn = dsn;
    OGR_iter->nlayers = nlayers;
    OGR_iter->ogr_interleaved_reading = ogr_interleaved_reading;
    OGR_iter->requested_layer = -1;
    OGR_iter->curr_layer = -1;
    OGR_iter->Ogr_layer = NULL;
    OGR_iter->has_nonempty_layers = 0;
    OGR_iter->done = 0;

    if (OGR_iter->ogr_interleaved_reading) {
#if GDAL_VERSION_NUM >= 2020000
	G_verbose_message(_("Using GDAL 2.2+ style interleaved reading for GDAL version %d.%d.%d"),
			  GDAL_VERSION_MAJOR, GDAL_VERSION_MINOR, GDAL_VERSION_REV); 
#else
	G_verbose_message(_("Using GDAL 1.x style interleaved reading for GDAL version %d.%d.%d"),
			  GDAL_VERSION_MAJOR, GDAL_VERSION_MINOR, GDAL_VERSION_REV); 
#endif
    }
}

void OGR_iterator_reset(struct OGR_iterator *OGR_iter)
{
#if GDAL_VERSION_NUM >= 2020000
    GDALDatasetResetReading(OGR_iter->Ogr_ds);
#endif
    OGR_iter->requested_layer = -1;
    OGR_iter->curr_layer = -1;
    OGR_iter->Ogr_layer = NULL;
    OGR_iter->has_nonempty_layers = 0;
    OGR_iter->done = 0;
}

OGRFeatureH ogr_getnextfeature(struct OGR_iterator *OGR_iter,
                               int layer, char *layer_name,
			       OGRGeometryH poSpatialFilter,
			       const char *attr_filter)
{
    if (OGR_iter->requested_layer != layer) {

	/* reset OGR reading */
	if (!OGR_iter->ogr_interleaved_reading) {
	    OGR_iter->curr_layer = layer;
	    OGR_iter->Ogr_layer = ds_getlayerbyindex(OGR_iter->Ogr_ds, OGR_iter->curr_layer);
	    OGR_iter->Ogr_featuredefn = OGR_L_GetLayerDefn(OGR_iter->Ogr_layer);
	    OGR_L_ResetReading(OGR_iter->Ogr_layer);
	}
	else {
	    int i;

	    /* clear filters */
	    for (i = 0; i < OGR_iter->nlayers; i++) {
		OGR_iter->Ogr_layer = ds_getlayerbyindex(OGR_iter->Ogr_ds, i);
		OGR_L_SetSpatialFilter(OGR_iter->Ogr_layer, NULL);
		OGR_L_SetAttributeFilter(OGR_iter->Ogr_layer, NULL);
	    }

#if GDAL_VERSION_NUM >= 2020000
	    GDALDatasetResetReading(OGR_iter->Ogr_ds);
#else
	    /* need to re-open OGR DSN in order to start reading from the beginning
	     * NOTE: any constraints are lost */
	    OGR_DS_Destroy(OGR_iter->Ogr_ds);
	    OGR_iter->Ogr_ds = OGROpen(OGR_iter->dsn, FALSE, NULL);
	    if (OGR_iter->Ogr_ds == NULL)
		G_fatal_error(_("Unable to re-open data source <%s>"), OGR_iter->dsn);
	    OGR_iter->Ogr_layer = OGR_DS_GetLayer(OGR_iter->Ogr_ds, layer);
	    OGR_iter->curr_layer = 0;
	    OGR_iter->has_nonempty_layers = 0;
#endif
	    OGR_iter->Ogr_layer = ds_getlayerbyindex(OGR_iter->Ogr_ds, layer);
	    OGR_iter->Ogr_featuredefn = OGR_L_GetLayerDefn(OGR_iter->Ogr_layer);
	    OGR_L_SetSpatialFilter(OGR_iter->Ogr_layer, poSpatialFilter);
	    if (OGR_L_SetAttributeFilter(OGR_iter->Ogr_layer, attr_filter) != OGRERR_NONE)
		G_fatal_error(_("Error setting attribute filter '%s'"),
		              attr_filter);
#if GDAL_VERSION_NUM < 2020000
	    OGR_iter->Ogr_layer = OGR_DS_GetLayer(OGR_iter->Ogr_ds, OGR_iter->curr_layer);
#endif
	}
	OGR_iter->requested_layer = layer;
	OGR_iter->done = 0;
    }

    if (OGR_iter->done == 1)
	return NULL;

    if (!OGR_iter->ogr_interleaved_reading) {
	OGRFeatureH Ogr_feature;

	Ogr_feature = OGR_L_GetNextFeature(OGR_iter->Ogr_layer);
	if (Ogr_feature == NULL) {
	    OGR_iter->Ogr_layer = NULL;
	    OGR_iter->done = 1;
	}

	return Ogr_feature;
    }
    else {
	OGRFeatureH Ogr_feature = NULL;

	/* fetch next feature */
#if GDAL_VERSION_NUM >= 2020000
	while (1) {
	    OGR_iter->Ogr_layer = NULL;
	    Ogr_feature = GDALDatasetGetNextFeature(OGR_iter->Ogr_ds,
						    &(OGR_iter->Ogr_layer),
						    NULL, NULL, NULL);

	    if (Ogr_feature == NULL) {
		OGR_iter->Ogr_layer = NULL;
		OGR_iter->done = 1;

		return Ogr_feature;
	    }
	    if (OGR_iter->Ogr_layer != NULL) {
		const char *ln = OGR_L_GetName(OGR_iter->Ogr_layer);
		
		if (ln && *ln && strcmp(ln, layer_name) == 0) {

		    return Ogr_feature;
		}
	    }
	    OGR_F_Destroy(Ogr_feature);
	    OGR_iter->Ogr_layer = NULL;
	}
#else
	while (1) {
	    Ogr_feature = OGR_L_GetNextFeature(OGR_iter->Ogr_layer);
	    if (Ogr_feature != NULL) {
		OGR_iter->has_nonempty_layers = 1;
		if (OGR_iter->curr_layer != layer)
		    OGR_F_Destroy(Ogr_feature);
		else
		    return Ogr_feature;
	    }
	    else {
		OGR_iter->curr_layer++;
		if (OGR_iter->curr_layer == OGR_iter->nlayers) {
		    if (!OGR_iter->has_nonempty_layers) {
			OGR_iter->Ogr_layer = NULL;
			OGR_iter->done = 1;

			return NULL;
		    }
		    else {
			OGR_iter->curr_layer = 0;
			OGR_iter->has_nonempty_layers = 0;
		    }
		}
		G_debug(3, "advancing to layer %d ...", OGR_iter->curr_layer);
		OGR_iter->Ogr_layer = OGR_DS_GetLayer(OGR_iter->Ogr_ds, OGR_iter->curr_layer);
		OGR_iter->Ogr_featuredefn = OGR_L_GetLayerDefn(OGR_iter->Ogr_layer);
	    }
	}
#endif
    }

    return NULL;
}

int create_spatial_filter(ds_t Ogr_ds, OGRGeometryH *poSpatialFilter,
                          int nlayers, int *layers, char **layer_names,
                          double *xmin, double *ymin,
			  double *xmax, double *ymax,
			  int use_region, struct Option *spat)
{
    int layer;
    int have_spatial_filter;
    int *have_ogr_extent;
    double *xminl, *yminl, *xmaxl, *ymaxl;
    OGRLayerH Ogr_layer;
    OGREnvelope oExt;
    OGRGeometryH Ogr_oRing;
    struct Cell_head cur_wind;

    /* fetch extents */
    have_ogr_extent = (int *)G_malloc(nlayers * sizeof(int));
    xminl = (double *)G_malloc(nlayers * sizeof(double));
    xmaxl = (double *)G_malloc(nlayers * sizeof(double));
    yminl = (double *)G_malloc(nlayers * sizeof(double));
    ymaxl = (double *)G_malloc(nlayers * sizeof(double));

    for (layer = 0; layer < nlayers; layer++) {
	Ogr_layer = ds_getlayerbyindex(Ogr_ds, layers[layer]);
	have_ogr_extent[layer] = 0;
	if ((OGR_L_GetExtent(Ogr_layer, &oExt, 1)) == OGRERR_NONE) {
	    xminl[layer] = oExt.MinX;
	    xmaxl[layer] = oExt.MaxX;
	    yminl[layer] = oExt.MinY;
	    ymaxl[layer] = oExt.MaxY;

	    /* OGR extents are unreliable,
	     * sometimes excluding valid features
	     * reason: converting double to float or %.6g and back
	     * -> expand a bit */
	    G_debug(2, "xmin old: %.15g", xminl[layer]);
	    xminl[layer] = xminl[layer] - fabs(xminl[layer] * 2.0e-6);
	    G_debug(2, "xmin new: %.15g", xminl[layer]);

	    G_debug(2, "xmax old: %.15g", xmaxl[layer]);
	    xmaxl[layer] = xmaxl[layer] + fabs(xmaxl[layer] * 2.0e-6);
	    G_debug(2, "xmax new: %.15g", xmaxl[layer]);

	    G_debug(2, "ymin old: %.15g", yminl[layer]);
	    yminl[layer] = yminl[layer] - fabs(yminl[layer] * 2.0e-6);
	    G_debug(2, "ymin new: %.15g", yminl[layer]);

	    G_debug(2, "ymax old: %.15g", ymaxl[layer]);
	    ymaxl[layer] = ymaxl[layer] + fabs(ymaxl[layer] * 2.0e-6);
	    G_debug(2, "ymax new: %.15g", ymaxl[layer]);

	    /* use OGR extents if possible, needed to skip corrupted data
	     * in OGR dsn/layer */
	    have_ogr_extent[layer] = 1;
	}
	/* OGR_L_GetExtent(): 
	 * Note that some implementations of this method may alter 
	 * the read cursor of the layer. */
#if GDAL_VERSION_NUM >= 2020000
	GDALDatasetResetReading(Ogr_ds);
#else
	OGR_L_ResetReading(Ogr_layer);
#endif
    }

    /* set spatial filter */
    if (use_region && spat->answer)
	G_fatal_error(_("Select either the current region flag or the spatial option, not both"));
    if (use_region) {
	G_get_window(&cur_wind);
	*xmin = cur_wind.west;
	*xmax = cur_wind.east;
	*ymin = cur_wind.south;
	*ymax = cur_wind.north;
    }
    if (spat->answer) {
	int i;
	/* See as reference: gdal/ogr/ogr_capi_test.c */

	/* cut out a piece of the map */
	/* order: xmin,ymin,xmax,ymax */
	i = 0;
	while (spat->answers[i]) {
	    if (i == 0)
		*xmin = atof(spat->answers[i]);
	    if (i == 1)
		*ymin = atof(spat->answers[i]);
	    if (i == 2)
		*xmax = atof(spat->answers[i]);
	    if (i == 3)
		*ymax = atof(spat->answers[i]);
	    i++;
	}
	if (i != 4)
	    G_fatal_error(_("4 parameters required for 'spatial' parameter"));
	if (*xmin > *xmax)
	    G_fatal_error(_("xmin is larger than xmax in 'spatial' parameters"));
	if (*ymin > *ymax)
	    G_fatal_error(_("ymin is larger than ymax in 'spatial' parameters"));
    }
    if (use_region || spat->answer) {
	G_debug(2, "cut out with boundaries: xmin:%f ymin:%f xmax:%f ymax:%f",
		*xmin, *ymin, *xmax, *ymax);
    }

    /* create spatial filter for each layer */
    have_spatial_filter = 0;
    for (layer = 0; layer < nlayers; layer++) {
	int have_filter = 0;

	/* OGR extents are unreliable, 
	 * sometimes excluding valid features:
	 * disabled */
	if (0 && have_ogr_extent[layer]) {
	    if (*xmin <= *xmax && *ymin <= *ymax) {
		/* check for any overlap */
		if (xminl[layer] > *xmax || xmaxl[layer] < *xmin ||
		    yminl[layer] > *ymax || ymaxl[layer] < *ymin) {
		    G_warning(_("The spatial filter does not overlap with OGR layer <%s>. Nothing to import."),
			      layer_names[layer]);

		    xminl[layer] = *xmin;
		    xmaxl[layer] = *xmax;
		    yminl[layer] = *ymin;
		    ymaxl[layer] = *ymax;
		}
		else {
		    /* shrink with user options */
		    xminl[layer] = MAX(xminl[layer], *xmin);
		    xmaxl[layer] = MIN(xmaxl[layer], *xmax);
		    yminl[layer] = MAX(yminl[layer], *ymin);
		    ymaxl[layer] = MIN(ymaxl[layer], *ymax);
		}
	    }
	    have_filter = 1;
	}
	else if (*xmin <= *xmax && *ymin <= *ymax) {
	    xminl[layer] = *xmin;
	    xmaxl[layer] = *xmax;
	    yminl[layer] = *ymin;
	    ymaxl[layer] = *ymax;

	    have_filter = 1;
	}

	if (have_filter) {
	    /* some invalid features can be filtered out by using
	     * the layer's extents as spatial filter
	     * hopefully these filtered features are all invalid */
	    /* TODO/BUG:
	     * for OSM, a spatial filter applied on the 'points' layer 
	     * will also affect other layers */

	    /* in theory this could be an irregular polygon */
	    G_debug(2, "spatial filter for layer <%s>: xmin:%f ymin:%f xmax:%f ymax:%f",
		    layer_names[layer],
		    xminl[layer], yminl[layer],
		    xmaxl[layer], ymaxl[layer]);

	    poSpatialFilter[layer] = OGR_G_CreateGeometry(wkbPolygon);
	    Ogr_oRing = OGR_G_CreateGeometry(wkbLinearRing);
	    OGR_G_AddPoint_2D(Ogr_oRing, xminl[layer], yminl[layer]);
	    OGR_G_AddPoint_2D(Ogr_oRing, xminl[layer], ymaxl[layer]);
	    OGR_G_AddPoint_2D(Ogr_oRing, xmaxl[layer], ymaxl[layer]);
	    OGR_G_AddPoint_2D(Ogr_oRing, xmaxl[layer], yminl[layer]);
	    OGR_G_AddPoint_2D(Ogr_oRing, xminl[layer], yminl[layer]);
	    OGR_G_AddGeometryDirectly(poSpatialFilter[layer], Ogr_oRing);

	    have_spatial_filter = 1;
	}
	else
	    poSpatialFilter[layer] = NULL;
    }
    /* update xmin, xmax, ymin, ymax if possible */
    for (layer = 0; layer < nlayers; layer++) {
	if (have_ogr_extent[layer]) {
	    if (xmin > xmax) {
		*xmin = xminl[layer];
		*xmax = xmaxl[layer];
		*ymin = yminl[layer];
		*ymax = ymaxl[layer];
	    }
	    else {
		/* expand */
		*xmin = MIN(xminl[layer], *xmin);
		*xmax = MAX(xmaxl[layer], *xmax);
		*ymin = MIN(yminl[layer], *ymin);
		*ymax = MAX(ymaxl[layer], *ymax);
	    }
	}
    }
    G_free(have_ogr_extent);
    G_free(xminl);
    G_free(xmaxl);
    G_free(yminl);
    G_free(ymaxl);

    return have_spatial_filter;
}
