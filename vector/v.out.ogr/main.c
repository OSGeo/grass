/*
 ***************************************************************
 *
 * MODULE:       v.out.ogr
 *
 * AUTHOR(S):    Radim Blazek
 *               Some extensions: Markus Neteler, Benjamin Ducke
 *               Multi-features support by Martin Landa <landa.martin gmail.com>
 *
 * PURPOSE:      Converts GRASS vector to one of supported OGR vector formats.
 *
 * COPYRIGHT:    (C) 2001-2017 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/

#include <stdlib.h>
#include <string.h>


#include <grass/gis.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

#include "local_proto.h"

#include <ogr_srs_api.h>

int main(int argc, char *argv[])
{
    int i, otype, ftype, donocat;
    int num_to_export;
    int field;
    int overwrite, found;

    struct GModule *module;
    struct Options options;
    struct Flags flags;

    char buf[SQL_BUFFER_SIZE];
    char key1[SQL_BUFFER_SIZE], key2[SQL_BUFFER_SIZE];
    struct Cell_head cellhd;
    char **tokens;

    /* Vector */
    struct Map_info In;

    /* Attributes */
    int doatt = 0, ncol = 0, colsqltype, colwidth, keycol = -1;
    int *colctype = NULL;
    const char **colname = NULL;
    struct field_info *Fi = NULL;
    dbDriver *Driver = NULL;
    dbTable *Table;
    dbString dbstring;
    dbColumn *Column;

    int n_feat;           /* number of written features */
    int n_nocat, n_noatt; /* number of features without cats/atts written/skip */

    /* OGR */
    int drn;
    OGRFieldType ogr_ftype = OFTInteger;
    ds_t hDS;
    dr_t hDriver;
    OGRLayerH Ogr_layer;
    OGRFieldDefnH Ogr_field;
    OGRFeatureDefnH Ogr_featuredefn;
    OGRwkbGeometryType wkbtype = wkbUnknown;
    OGRSpatialReferenceH Ogr_projection;
    char **papszDSCO = NULL, **papszLCO = NULL;
    int num_types;
    char *dsn;
    int outer_ring_ccw;

    G_gisinit(argv[0]);

    /* Module options */
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("export"));
    G_add_keyword(_("output"));
    G_add_keyword("OGR");

    module->label =
	_("Exports a vector map layer to any of the supported OGR vector formats.");
    module->description = _("By default a vector map layer is exported to OGC GeoPackage format.");
    module->overwrite = TRUE;

    /* parse & read options */
    parse_args(argc, argv, &options, &flags);

    if (flags.list->answer) {
	list_formats();
	exit(EXIT_SUCCESS);
    }

    /* check for weird options */
    if (G_strncasecmp(options.dsn->answer, "PG:", 3) == 0 &&
        strcmp(options.format->answer, "PostgreSQL") != 0)
        G_warning(_("Data source starts with \"PG:\" prefix, expecting \"PostgreSQL\" "
                    "format (\"%s\" given)"), options.format->answer);

    /* parse dataset creation options */
    i = 0;
    while (options.dsco->answers[i]) {
	tokens = G_tokenize(options.dsco->answers[i], "=");
	if (G_number_of_tokens(tokens))
	    papszDSCO = CSLSetNameValue(papszDSCO, tokens[0], tokens[1]);
	G_free_tokens(tokens);
	i++;
    }

    /* parse layer creation options */
    i = 0;
    while (options.lco->answers[i]) {
	tokens = G_tokenize(options.lco->answers[i], "=");
	if (G_number_of_tokens(tokens))
	    papszLCO = CSLSetNameValue(papszLCO, tokens[0], tokens[1]);
	G_free_tokens(tokens);
	i++;
    }

    /*
       If no output type specified: determine one automatically.
       Centroids, Boundaries and Kernels always have to be exported
       explicitly, using the "type=" option.
     */
    field = 0;
    if (!flags.new->answer) {
	/* open input vector (topology required) */
	Vect_set_open_level(2);
	if (Vect_open_old2(&In, options.input->answer, "",
				options.field->answer) < 0)
	    G_fatal_error(_("Unable to open vector map <%s>"),
			    options.input->answer);

	if (strcmp(options.type->answer, "auto") == 0) {
	    G_debug(2, "Automatic type determination.");

	    options.type->answers = G_malloc(sizeof(char *) * 10);
	    G_zero(options.type->answers, sizeof(char *) * 10);
	    num_types = 0;

	    if (Vect_get_num_primitives(&In, GV_POINT) > 0) {
		options.type->answers[num_types++] = G_store("point");
		G_debug(3, "Adding points to export list.");
	    }

	    if (Vect_get_num_primitives(&In, GV_LINE) > 0) {
		options.type->answers[num_types++] = G_store("line");
		G_debug(3, "Adding lines to export list.");
	    }

	    if (Vect_get_num_areas(&In) > 0) {
		options.type->answers[num_types++] = G_store("area");
		G_debug(3, "Adding areas to export list.");
	    }

	    /*  Faces and volumes:
	       For now, volumes will just be exported as sets of faces.
	     */
	    if (Vect_get_num_primitives(&In, GV_FACE) > 0) {
		options.type->answers[num_types++] = G_store("face");
		G_debug(3, "Adding faces to export list.");
	    }

	    /* this check HAS TO FOLLOW RIGHT AFTER check for GV_FACE! */
	    if (Vect_get_num_volumes(&In) > 0) {
		G_warning(_("Volumes will be exported as sets of faces"));
		if (num_types == 0) {
		    /* no other types yet? */
		    options.type->answers[num_types++] = G_store("volume");
		    G_debug(3, "Adding volumes to export list.");
		}
		else {
		    if (strcmp(options.type->answers[num_types - 1], "face")
			!= 0) {
			/* only put faces on export list if that's not the case already */
			options.type->answers[num_types++] =
			    G_store("volume");
			G_debug(3, "Adding volumes to export list.");
		    }
		}
	    }

	    if (num_types == 0) {
		G_warning(_("Unable to determine input map's vector feature type(s)."));
            }
	}
	field = Vect_get_field_number(&In, options.field->answer);
    }

    /* check output feature type */
    otype = Vect_option_to_types(options.type);
    ftype = Vect_option_to_types(options.otype);

    if (!options.layer->answer) {
	char xname[GNAME_MAX], xmapset[GMAPSET_MAX];

	if (G_name_is_fully_qualified(options.input->answer, xname, xmapset))
	    options.layer->answer = G_store(xname);
	else
	    options.layer->answer = G_store(options.input->answer);
    }

    if (ftype == GV_BOUNDARY) {
        if (!flags.multi->answer)
            wkbtype = wkbPolygon;
        else
            wkbtype = wkbMultiPolygon;
    }
    else if (otype & GV_POINTS) {
        if (!flags.multi->answer)
            wkbtype = wkbPoint;
        else
            wkbtype = wkbMultiPoint;
    }
    else if (otype & GV_LINES || ftype == GV_LINE) {
        if (!flags.multi->answer)
            wkbtype = wkbLineString;
        else
            wkbtype = wkbMultiLineString;
    }
    else if (otype & GV_AREA) {
        if (!flags.multi->answer)
            wkbtype = wkbPolygon;
        else
            wkbtype = wkbMultiPolygon;
    }
    else if (otype & (GV_FACE | GV_VOLUME)) {
        if (!flags.multi->answer)
            wkbtype = wkbPolygon25D;
        else
            wkbtype = wkbMultiPolygon25D;
    }

    if (((GV_POINTS & otype) && (GV_LINES & otype)) ||
	((GV_POINTS & otype) && (GV_AREA & otype)) ||
	((GV_POINTS & otype) && (GV_FACE & otype)) ||
	((GV_POINTS & otype) && (GV_KERNEL & otype)) ||
	((GV_POINTS & otype) && (GV_VOLUME & otype)) ||
	((GV_LINES & otype) && (GV_AREA & otype)) ||
	((GV_LINES & otype) && (GV_FACE & otype)) ||
	((GV_LINES & otype) && (GV_KERNEL & otype)) ||
	((GV_LINES & otype) && (GV_VOLUME & otype)) ||
	((GV_KERNEL & otype) && (GV_POINTS & otype)) ||
	((GV_KERNEL & otype) && (GV_LINES & otype)) ||
	((GV_KERNEL & otype) && (GV_AREA & otype)) ||
	((GV_KERNEL & otype) && (GV_FACE & otype)) ||
	((GV_KERNEL & otype) && (GV_VOLUME & otype))
	) {
	G_warning(_("The combination of types is not supported"
		    " by all formats."));
	wkbtype = wkbUnknown;
    }

    /* fetch PROJ info */
    G_get_default_window(&cellhd);
    Ogr_projection = NULL;
    if (cellhd.proj != PROJECTION_XY) {
	struct Key_Value *projinfo, *projunits, *projepsg;

	projinfo = G_get_projinfo();
	projunits = G_get_projunits();
	projepsg = G_get_projepsg();

#if GDAL_VERSION_MAJOR >= 3 && PROJ_VERSION_MAJOR >= 6
	char *indef = NULL, *inwkt = NULL;

	if ((indef = G_get_projsrid())) {
	    PJ *obj = NULL;

	    if ((obj = proj_create(NULL, indef))) {
		inwkt = G_store(proj_as_wkt(NULL, obj, PJ_WKT2_LATEST, NULL));

		if (inwkt && !*inwkt) {
		    G_free(inwkt);
		    inwkt = NULL;
		}
	    }
	}
	if (!inwkt) {
	    inwkt = G_get_projwkt();
	}
	if (inwkt) {
	    Ogr_projection = OSRNewSpatialReference(inwkt);
	}
#endif
	if (!Ogr_projection)
	    Ogr_projection = GPJ_grass_to_osr2(projinfo, projunits, projepsg);

#if GDAL_VERSION_MAJOR >= 3 && PROJ_VERSION_MAJOR >= 6
	if (Ogr_projection) {
	    /* convert bound CRS */
	    PJ *obj = NULL;
	    char *papszOptions[2];

	    inwkt = NULL;

	    papszOptions[0] = G_store("FORMAT=WKT2");
	    papszOptions[1] = NULL;
	    OSRExportToWktEx(Ogr_projection, &inwkt, (const char **)papszOptions);
	    G_free(papszOptions[0]);

	    if ((obj = proj_create(NULL, inwkt))) {
		if (proj_get_type(obj) == PJ_TYPE_BOUND_CRS) {
		    PJ *source_crs;

		    G_debug(1, "found bound crs");
		    source_crs = proj_get_source_crs(NULL, obj);
		    if (source_crs) {
			inwkt = G_store(proj_as_wkt(NULL, source_crs, PJ_WKT2_LATEST, NULL));
			if (inwkt && !*inwkt) {
			    G_free(inwkt);
			    inwkt = NULL;
			}
			proj_destroy(source_crs);
			Ogr_projection = NULL;
			if (inwkt) {
			    char *inwkttmp = inwkt;
			    OSRImportFromWkt(Ogr_projection, &inwkttmp);
			}
		    }
		}
		proj_destroy(obj);
	    }
	    if (inwkt)
		CPLFree(inwkt);
	}
#endif

	if (Ogr_projection ==  NULL)
	    G_fatal_error(_("Unable to create OGR spatial reference"));

	if (flags.esristyle->answer &&
	    (strcmp(options.format->answer, "ESRI_Shapefile") == 0))
	    OSRMorphToESRI(Ogr_projection);
    }

    dsn = NULL;
    if (options.dsn->answer)
        dsn = G_store(options.dsn->answer);

    /* create new OGR layer in datasource */
    if (flags.new->answer) {
	const char *name;

	name =
	    options.layer->answer ? options.layer->answer : options.input->
	    answer;

	create_ogr_layer(dsn, options.format->answer, name,
			 wkbtype, papszDSCO, papszLCO);

	G_message(_("OGR layer <%s> created in datasource <%s> (format '%s')"),
		  name, options.dsn->answer, options.format->answer);
	exit(EXIT_SUCCESS);
    }

    if (flags.cat->answer)
	donocat = 1;
    else
	donocat = 0;

    if ((GV_AREA & otype) && Vect_get_num_islands(&In) > 0 &&
	flags.cat->answer)
	G_warning(_("The map contains islands. With the -c flag, "
		    "islands will appear as filled areas, not holes in the output map."));


    /* check what users wants to export and what's present in the map */
    if (Vect_get_num_primitives(&In, GV_POINT) > 0 && !(otype & GV_POINTS))
	G_warning(n_("%d point found, but not requested to be exported. "
		     "Verify 'type' parameter.",
                     "%d points found, but not requested to be exported. "
                     "Verify 'type' parameter.",
                     Vect_get_num_primitives(&In, GV_POINT)),
                  Vect_get_num_primitives(&In, GV_POINT));

    if (Vect_get_num_primitives(&In, GV_LINE) > 0 && !(otype & GV_LINES))
	G_warning(n_("%d line found, but not requested to be exported. "
		     "Verify 'type' parameter.",
                     "%d line(s) found, but not requested to be exported. "
                     "Verify 'type' parameter.",
                     Vect_get_num_primitives(&In, GV_LINE)),
                  Vect_get_num_primitives(&In, GV_LINE));

    if (Vect_get_num_primitives(&In, GV_BOUNDARY) > 0 &&
	!(otype & GV_BOUNDARY) && !(otype & GV_AREA))
	G_warning(n_("%d boundary found, but not requested to be exported. "
		     "Verify 'type' parameter.",
                     "%d boundaries found, but not requested to be exported. "
                     "Verify 'type' parameter.",
                     Vect_get_num_primitives(&In, GV_BOUNDARY)),
                  Vect_get_num_primitives(&In, GV_BOUNDARY));

    if (Vect_get_num_primitives(&In, GV_CENTROID) > 0 &&
	!(otype & GV_CENTROID) && !(otype & GV_AREA))
	G_warning(n_("%d centroid found, but not requested to be exported. "
		     "Verify 'type' parameter.",
                     "%d centroids found, but not requested to be exported. "
                     "Verify 'type' parameter.",
                     Vect_get_num_primitives(&In, GV_CENTROID)),
                  Vect_get_num_primitives(&In, GV_CENTROID));

    if (Vect_get_num_areas(&In) > 0 && !(otype & GV_AREA))
	G_warning(n_("%d area found, but not requested to be exported. "
		     "Verify 'type' parameter.",
                     "%d areas found, but not requested to be exported. "
                     "Verify 'type' parameter.",
                     Vect_get_num_areas(&In)),
                  Vect_get_num_areas(&In));

    if (Vect_get_num_primitives(&In, GV_FACE) > 0 && !(otype & GV_FACE))
	G_warning(n_("%d face found, but not requested to be exported. "
		     "Verify 'type' parameter.",
                     "%d faces found, but not requested to be exported. "
                     "Verify 'type' parameter.",
                     Vect_get_num_primitives(&In, GV_FACE)),
                  Vect_get_num_primitives(&In, GV_FACE));

    if (Vect_get_num_volumes(&In) > 0 && !(otype & GV_VOLUME))
	G_warning(n_("%d volume found, but not requested to be exported. "
		     "Verify 'type' parameter.",
                     "%d volumes found, but not requested to be exported. "
                     "Verify 'type' parameter.",
                     Vect_get_num_volumes(&In)),
                  Vect_get_num_volumes(&In));

    /* warn and eventually abort if there is nothing to be exported */
    num_to_export = 0;
    if (Vect_get_num_primitives(&In, GV_POINT) < 1 && (otype & GV_POINTS)) {
	G_warning(_("No points found, but requested to be exported. "
		    "Will skip this feature type."));
    }
    else {
	if (otype & GV_POINT)
	    num_to_export += Vect_get_num_primitives(&In, GV_POINT);
    }

    if (Vect_get_num_primitives(&In, GV_LINE) < 1 && (otype & GV_LINE)) {
	G_warning(_("No lines found, but requested to be exported. "
		    "Will skip this feature type."));
    }
    else {
	if (otype & GV_LINE)
	    num_to_export += Vect_get_num_primitives(&In, GV_LINE);
    }

    if (Vect_get_num_primitives(&In, GV_BOUNDARY) < 1 &&
	(otype & GV_BOUNDARY)) {
	G_warning(_("No boundaries found, but requested to be exported. "
		   "Will skip this feature type."));
    }
    else {
	if (otype & GV_BOUNDARY)
	    num_to_export += Vect_get_num_primitives(&In, GV_BOUNDARY);
    }

    if (Vect_get_num_areas(&In) < 1 && (otype & GV_AREA)) {
	G_warning(_("No areas found, but requested to be exported. "
		    "Will skip this feature type."));
    }
    else {
	if (otype & GV_AREA)
	    num_to_export += Vect_get_num_areas(&In);
    }

    if (Vect_get_num_primitives(&In, GV_CENTROID) < 1 &&
	(otype & GV_CENTROID)) {
	G_warning(_("No centroids found, but requested to be exported. "
		   "Will skip this feature type."));
    }
    else {
	if (otype & GV_CENTROID)
	    num_to_export += Vect_get_num_primitives(&In, GV_CENTROID);
    }

    if (Vect_get_num_primitives(&In, GV_FACE) < 1 && (otype & GV_FACE)) {
	G_warning(_("No faces found, but requested to be exported. "
		    "Will skip this feature type."));
    }
    else {
	if (otype & GV_FACE)
	    num_to_export += Vect_get_num_primitives(&In, GV_FACE);
    }

    if (Vect_get_num_primitives(&In, GV_KERNEL) < 1 && (otype & GV_KERNEL)) {
	G_warning(_("No kernels found, but requested to be exported. "
		    "Will skip this feature type."));
    }
    else {
	if (otype & GV_KERNEL)
	    num_to_export += Vect_get_num_primitives(&In, GV_KERNEL);
    }

    if (Vect_get_num_volumes(&In) < 1 && (otype & GV_VOLUME)) {
	G_warning(_("No volumes found, but requested to be exported. "
		    "Will skip this feature type."));
    }
    else {
	if (otype & GV_VOLUME)
	    num_to_export += Vect_get_num_volumes(&In);
    }

    G_debug(1, "Requested to export %d features", num_to_export);

    /* Open OGR DSN */
#if GDAL_VERSION_NUM >= 2020000
    G_debug(2, "driver count = %d", GDALGetDriverCount());
    drn = -1;
    for (i = 0; i < GDALGetDriverCount(); i++) {
	hDriver = GDALGetDriver(i);
	G_debug(2, "driver %d : %s", i, GDALGetDriverShortName(hDriver));
	/* chg white space to underscore in OGR driver names */
	sprintf(buf, "%s", GDALGetDriverShortName(hDriver));
	G_strchg(buf, ' ', '_');
	if (strcmp(buf, options.format->answer) == 0) {
	    drn = i;
	    G_debug(2, " -> driver = %d", drn);
	}
    }
#else
    G_debug(2, "driver count = %d", OGRGetDriverCount());
    drn = -1;
    for (i = 0; i < OGRGetDriverCount(); i++) {
	hDriver = OGRGetDriver(i);
	G_debug(2, "driver %d : %s", i, OGR_Dr_GetName(hDriver));
	/* chg white space to underscore in OGR driver names */
	sprintf(buf, "%s", OGR_Dr_GetName(hDriver));
	G_strchg(buf, ' ', '_');
	if (strcmp(buf, options.format->answer) == 0) {
	    drn = i;
	    G_debug(2, " -> driver = %d", drn);
	}
    }
#endif
    if (drn == -1)
	G_fatal_error(_("OGR driver <%s> not found"), options.format->answer);
    hDriver = get_driver(drn);

    if (flags.append->answer) {
	G_debug(1, "Append to OGR layer");
#if GDAL_VERSION_NUM >= 2020000
	hDS = GDALOpenEx(dsn, GDAL_OF_VECTOR | GDAL_OF_UPDATE, NULL, NULL, NULL);

	if (hDS == NULL) {
	    G_debug(1, "Create OGR data source");
	    hDS = GDALCreate(hDriver, dsn, 0, 0, 0, GDT_Unknown, papszDSCO);
	}
#else
	hDS = OGR_Dr_Open(hDriver, dsn, TRUE);

	if (hDS == NULL) {
	    G_debug(1, "Create OGR data source");
	    hDS = OGR_Dr_CreateDataSource(hDriver, dsn, papszDSCO);
	}
#endif
    }
    else {
#if GDAL_VERSION_NUM >= 2020000
	if (flags.update->answer) {
	    G_debug(1, "Update OGR data source");
	    hDS = GDALOpenEx(dsn, GDAL_OF_VECTOR | GDAL_OF_UPDATE, NULL, NULL, NULL);
	}
	else {
	    G_debug(1, "Create OGR data source");
	    hDS = GDALCreate(hDriver, dsn, 0, 0, 0, GDT_Unknown, papszDSCO);
	}
#else
	if (flags.update->answer) {
	    G_debug(1, "Update OGR data source");
	    hDS = OGR_Dr_Open(hDriver, dsn, TRUE);
	}
	else {
	    G_debug(1, "Create OGR data source");
	    hDS = OGR_Dr_CreateDataSource(hDriver, dsn, papszDSCO);
	}
#endif
    }

    CSLDestroy(papszDSCO);
    if (hDS == NULL)
	G_fatal_error(_("Unable to open OGR data source '%s'"),
		      options.dsn->answer);

    /* check if OGR layer exists */
    overwrite = G_check_overwrite(argc, argv);
    found = FALSE;
#if GDAL_VERSION_NUM >= 2020000
    for (i = 0; i < GDALDatasetGetLayerCount(hDS); i++) {
	Ogr_layer = GDALDatasetGetLayer(hDS, i);
#else
    for (i = 0; i < OGR_DS_GetLayerCount(hDS); i++) {
	Ogr_layer = OGR_DS_GetLayer(hDS, i);

#endif
	Ogr_field = OGR_L_GetLayerDefn(Ogr_layer);
	if (G_strcasecmp(OGR_FD_GetName(Ogr_field), options.layer->answer))
	    continue;

	found = TRUE;
	if (!overwrite && !flags.append->answer) {
	    G_fatal_error(_("Layer <%s> already exists in OGR data source '%s'"),
			  options.layer->answer, options.dsn->answer);
	}
	else if (overwrite) {
	    G_warning(_("OGR layer <%s> already exists and will be overwritten"),
		      options.layer->answer);
#if GDAL_VERSION_NUM >= 2020000
	    GDALDatasetDeleteLayer(hDS, i);
#else
	    OGR_DS_DeleteLayer(hDS, i);
#endif
	    break;
	}
    }

    if (flags.append->answer && !found) {
	G_warning(_("OGR layer <%s> doesn't exists, "
		    "creating new OGR layer instead"),
		  options.layer->answer);
	flags.append->answer = FALSE;
    }

    /* Automatically append driver options for 3D output to layer
       creation options if '2' is not given.*/
    if (!flags.force2d->answer &&
        Vect_is_3d(&In) &&
        strcmp(options.format->answer, "ESRI_Shapefile") == 0) {
	/* find right option */
	char shape_geom[20];
	if ((otype & GV_POINTS) || (otype & GV_KERNEL))
	    sprintf(shape_geom, "POINTZ");
	if ((otype & GV_LINES))
	    sprintf(shape_geom, "ARCZ");
	if ((otype & GV_AREA) || (otype & GV_FACE))
	    sprintf(shape_geom, "POLYGONZ");
	/* check if the right LCO is already present */
	const char *shpt;
	shpt = CSLFetchNameValue(papszLCO, "SHPT");
	if ((!shpt)) {
	    /* Not set at all? Good! */
	    papszLCO = CSLSetNameValue(papszLCO, "SHPT", shape_geom);
	} else {
	    if (strcmp(shpt, shape_geom) != 0) {
		/* Set but to a different value? Override! */
		G_warning(_("Overriding existing user-defined 'SHPT=' LCO."));
	    }
	    /* Set correct LCO for this geometry type */
	    papszLCO = CSLSetNameValue(papszLCO, "SHPT", shape_geom);
	}
    }

    /* check if the map is 3d */
    if (Vect_is_3d(&In)) {
	/* specific check for ESRI ShapeFile */
	if (strcmp(options.format->answer, "ESRI_Shapefile") == 0) {
	    const char *shpt;

	    shpt = CSLFetchNameValue(papszLCO, "SHPT");
	    if (!shpt || shpt[strlen(shpt) - 1] != 'Z') {
		G_warning(_("Vector map <%s> is 3D. "
			    "Use format specific layer creation options SHPT (parameter 'lco') "
			    "or '-z' flag to export in 3D rather than 2D (default)"),
			  options.input->answer);
	    }
	}
	/* specific check for PostgreSQL */
	else if (strcmp(options.format->answer, "PostgreSQL") == 0) {
	    const char *dim;

	    dim = CSLFetchNameValue(papszLCO, "DIM");
	    if (!dim || strcmp(dim, "3") != 0) {
		G_warning(_("Vector map <%s> is 3D. "
			    "Use format specific layer creation options DIM (parameter 'lco') "
			    "to export in 3D rather than 2D (default)."),
			  options.input->answer);
	    }
	}
	else {
	    G_warning(_("Vector map <%s> is 3D. "
			"Use format specific layer creation options (parameter 'lco') "
			"to export <in 3D rather than 2D (default)."),
		      options.input->answer);
	}
    }

    G_debug(1, "Create OGR layer");
#if GDAL_VERSION_NUM >= 2020000
    if (flags.append->answer)
	Ogr_layer = GDALDatasetGetLayerByName(hDS, options.layer->answer);
    else
	Ogr_layer = GDALDatasetCreateLayer(hDS, options.layer->answer,
	                                   Ogr_projection, wkbtype,
					   papszLCO);
#else
    if (flags.append->answer)
	Ogr_layer = OGR_DS_GetLayerByName(hDS, options.layer->answer);
    else
	Ogr_layer = OGR_DS_CreateLayer(hDS, options.layer->answer,
	                               Ogr_projection, wkbtype,
				       papszLCO);
#endif

    CSLDestroy(papszLCO);
    if (Ogr_layer == NULL) {
	if (flags.append->answer)
	    G_fatal_error(_("OGR layer <%s> not found"), options.layer->answer);
	else
	    G_fatal_error(_("Unable to create OGR layer"));
    }

    db_init_string(&dbstring);

    /* Vector attributes -> OGR fields */
    if (field > 0) {
	G_debug(1, "Create attribute table");
	doatt = 1;		/* do attributes */
	Fi = Vect_get_field(&In, field);
	if (Fi == NULL) {
	    char create_field = TRUE;
	    G_warning(_("No attribute table found -> using only category numbers as attributes"));
	    /* if we have no more than a 'cat' column, then that has to
	       be exported in any case */
	    if (flags.nocat->answer) {
		G_warning(_("Exporting 'cat' anyway, as it is the only attribute table field"));
		flags.nocat->answer = FALSE;
	    }

	    if (flags.append->answer) {
		Ogr_field = OGR_L_GetLayerDefn(Ogr_layer);
		if (OGR_FD_GetFieldIndex(Ogr_field, GV_KEY_COLUMN) > -1)
		    create_field = FALSE;
		else
		    G_warning(_("New attribute column <%s> added to the table"),
			      GV_KEY_COLUMN);
	    }

	    if (create_field) {
		Ogr_field = OGR_Fld_Create(GV_KEY_COLUMN, OFTInteger);
		if (OGR_L_CreateField(Ogr_layer, Ogr_field, 0) != OGRERR_NONE)
		    G_fatal_error(_("Unable to create column <%s>"),
		                  GV_KEY_COLUMN);
		OGR_Fld_Destroy(Ogr_field);
	    }

	    doatt = 0;
	}
	else {
	    Driver = db_start_driver_open_database(Fi->driver, Fi->database);
	    if (!Driver)
		G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			       Fi->database, Fi->driver);

	    db_set_string(&dbstring, Fi->table);
	    if (db_describe_table(Driver, &dbstring, &Table) != DB_OK)
		G_fatal_error(_("Unable to describe table <%s>"), Fi->table);

	    ncol = db_get_table_number_of_columns(Table);
	    G_debug(2, "ncol = %d", ncol);
	    colctype = G_malloc(ncol * sizeof(int));
	    colname = G_malloc(ncol * sizeof(char *));
	    keycol = -1;
	    for (i = 0; i < ncol; i++) {
		Column = db_get_table_column(Table, i);
		colname[i] =  G_store(db_get_column_name(Column));
		colsqltype = db_get_column_sqltype(Column);
		colctype[i] = db_sqltype_to_Ctype(colsqltype);
		colwidth = db_get_column_length(Column);
		G_debug(3, "col %d: %s sqltype=%d ctype=%d width=%d",
			i, colname[i], colsqltype, colctype[i], colwidth);

		switch (colctype[i]) {
		case DB_C_TYPE_INT:
		    ogr_ftype = OFTInteger;
		    break;
		case DB_C_TYPE_DOUBLE:
		    ogr_ftype = OFTReal;
		    break;
		case DB_C_TYPE_STRING:
		    ogr_ftype = OFTString;
		    break;
		case DB_C_TYPE_DATETIME:
#if GDAL_VERSION_NUM >= 1320
		    ogr_ftype = OFTDateTime;
#else
		    ogr_ftype = OFTString;
#endif
		    break;
		}
		G_debug(2, "ogr_ftype = %d", ogr_ftype);

		strcpy(key1, Fi->key);
		G_tolcase(key1);
		strcpy(key2, colname[i]);
		G_tolcase(key2);
		if (strcmp(key1, key2) == 0)
		    keycol = i;
		G_debug(2, "%s x %s -> %s x %s -> keycol = %d", Fi->key,
			colname[i], key1, key2, keycol);

		if (flags.nocat->answer &&
		    strcmp(Fi->key, colname[i]) == 0)
		    /* skip export of 'cat' field */
		    continue;

		if (flags.append->answer) {
		    Ogr_field = OGR_L_GetLayerDefn(Ogr_layer);
		    if (OGR_FD_GetFieldIndex(Ogr_field, colname[i]) > -1)
			/* skip existing fields */
			continue;
		    else
			G_warning(_("New attribute column <%s> added to the table"),
				   colname[i]);
		}

		Ogr_field = OGR_Fld_Create(colname[i], ogr_ftype);
		if (ogr_ftype == OFTString && colwidth > 0)
		    OGR_Fld_SetWidth(Ogr_field, colwidth);
		if (OGR_L_CreateField(Ogr_layer, Ogr_field, 0) != OGRERR_NONE)
		    G_fatal_error(_("Unable to create column <%s>"),
		                  colname[i]);

		OGR_Fld_Destroy(Ogr_field);
	    }
	    if (keycol == -1)
		G_fatal_error(_("Key column <%s> not found"), Fi->key);
	}
    }

    Ogr_featuredefn = OGR_L_GetLayerDefn(Ogr_layer);

    n_feat = n_nocat = n_noatt = 0;

    if (OGR_L_TestCapability(Ogr_layer, OLCTransactions)) {
	if (OGR_L_StartTransaction(Ogr_layer) != OGRERR_NONE) {
	    G_warning(_("Unable to start OGR transaction"));
	}
    }

    /* export polygons oriented according to OGC simple features standard 1.2.1
     * outer rings are oriented counter-clockwise (CCW)
     * inner rings are oriented clockwise (CW) */
    outer_ring_ccw = 1;
    /* some formats expect outer rings to be CW and inner rings to be CCW:
     * ESRI Shapefile, PGeo, FileGDB, OpenFileGDB (all ESRI) */
    if (strcmp(options.format->answer, "ESRI_Shapefile") == 0 ||
        strcmp(options.format->answer, "PGeo") == 0 ||
        strcmp(options.format->answer, "FileGDB") == 0 ||
        strcmp(options.format->answer, "OpenFileGDB") == 0) {
	outer_ring_ccw = 0;
    }
    G_debug(1, "Format \"%s\", outer ring %s",
            options.format->answer, (outer_ring_ccw ? "CCW" : "CW"));

    /* Lines (run always to count features of different type) */
    if (otype & (GV_POINTS | GV_LINES | GV_KERNEL | GV_FACE)) {
        G_message(n_("Exporting %d feature...",
                     "Exporting %d features...",
                     Vect_get_num_primitives(&In, otype)),
                 Vect_get_num_primitives(&In, otype));

        n_feat += export_lines(&In, field, otype, flags.multi->answer ? TRUE : FALSE,
                               donocat, ftype == GV_BOUNDARY ? TRUE : FALSE,
                               Ogr_featuredefn, Ogr_layer,
                               Fi, Driver, ncol, colctype,
                               colname, doatt, flags.nocat->answer ? TRUE : FALSE,
                               &n_noatt, &n_nocat);
    }

    /* Areas (run always to count features of different type) */
    if (Vect_get_num_areas(&In) > 0 && (otype & GV_AREA)) {
	G_message(n_("Exporting %d area (may take some time)...",
                     "Exporting %d areas (may take some time)...",
                     Vect_get_num_areas(&In)),
		  Vect_get_num_areas(&In));

        n_feat += export_areas(&In, field, flags.multi->answer ? TRUE : FALSE, donocat,
                               Ogr_featuredefn, Ogr_layer,
                               Fi, Driver, ncol, colctype,
                               colname, doatt, flags.nocat->answer ? TRUE : FALSE,
                               &n_noatt, &n_nocat, outer_ring_ccw);
    }

    /*
       TODO: Volumes. Do not export kernels here, that's already done.
       We do need to worry about holes, though. NOTE: We can probably
       just merge this with faces export function. Except for GRASS,
       which output format would know the difference?
     */
    if (Vect_get_num_volumes(&In) > 0 && (otype & GV_VOLUME)) {
	G_message(n_("Exporting %d volume...",
                     "Exporting %d volumes...",
                     Vect_get_num_volumes(&In)),
                  Vect_get_num_volumes(&In));
	G_warning(_("Export of volumes not implemented yet. Skipping."));
    }

    if (OGR_L_TestCapability(Ogr_layer, OLCTransactions)) {
	if (OGR_L_CommitTransaction(Ogr_layer) != OGRERR_NONE) {
	    G_warning(_("Unable to commit OGR transaction"));
	}
    }

    ds_close(hDS);

    Vect_close(&In);

    if (doatt) {
	db_close_database(Driver);
	db_shutdown_driver(Driver);
    }

    /* Summary */
    if (n_noatt > 0)
	G_important_message(n_("%d feature without attributes was written",
                               "%d features without attributes were written",
                               n_noatt), n_noatt);

    if (n_nocat > 0) {
	if (donocat)
	    G_important_message(n_("%d feature without category was written",
				   "%d features without category were written",
				   n_nocat), n_nocat);
	else
	    G_warning(n_("%d feature without category was skipped. "
                         "Features without category are written only when -%c flag is given.",
                         "%d features without category were skipped. "
                         "Features without category are written only when -%c flag is given.",
                         n_nocat), n_nocat, flags.cat->key);
    }

    /* Enable this? May be confusing that for area type are not
     * reported all boundaries/centroids. OTOH why should be
     * reported? */
    /*
       if (((otype & GV_POINTS) || (otype & GV_LINES)) && fskip > 0)
       G_warning ("%d features of different type skip", fskip);
     */

    if (n_feat < 1)
        G_warning(_("Output layer is empty, no features written"));
    G_done_msg(n_("%d feature (%s type) written to <%s> (%s format).",
                  "%d features (%s type) written to <%s> (%s format).",
                  n_feat), n_feat,
               OGRGeometryTypeToName(wkbtype),
	       options.layer->answer, options.format->answer);

    exit(EXIT_SUCCESS);
}
