/*
 ***************************************************************
 *
 * MODULE:       v.out.ogr
 *
 * AUTHOR(S):    Radim Blazek
 *               Some extensions: Markus Neteler, Benjamin Ducke
 *               Update for GRASS 7 by Martin Landa <landa.martin gmail.com> (create new OGR layer)
 *
 * PURPOSE:      Converts GRASS vector to one of supported OGR vector formats.
 *
 * COPYRIGHT:    (C) 2001-2009, 2011 by the GRASS Development Team
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

int main(int argc, char *argv[])
{
    int i, j, k, centroid, otype, donocat;
    int num_to_export;
    int field;
    int overwrite, found;
    struct GModule *module;
    struct Options options;
    struct Flags flags;

    char buf[SQL_BUFFER_SIZE];
    char key1[SQL_BUFFER_SIZE], key2[SQL_BUFFER_SIZE];
    struct Key_Value *projinfo, *projunits;
    struct Cell_head cellhd;
    char **tokens;

    /* Vector */
    struct Map_info In;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int type, cat;

    /* Attributes */
    int doatt = 0, ncol = 0, colsqltype, colwidth, keycol = -1;
    int *colctype = NULL;
    const char **colname = NULL;
    struct field_info *Fi = NULL;
    dbDriver *Driver = NULL;
    dbTable *Table;
    dbString dbstring;
    dbColumn *Column;

    int fout, fskip;		/* features written/ skip */
    int nocat, noatt, nocatskip;	/* number of features without cats/atts written/skip */

    /* OGR */
    int drn;
    OGRFieldType ogr_ftype = OFTInteger;
    OGRDataSourceH Ogr_ds;
    OGRSFDriverH Ogr_driver;
    OGRLayerH Ogr_layer;
    OGRFieldDefnH Ogr_field;
    OGRFeatureH Ogr_feature;
    OGRFeatureDefnH Ogr_featuredefn;
    OGRGeometryH Ogr_geometry;
    OGRwkbGeometryType wkbtype = wkbUnknown;	/* ?? */
    OGRSpatialReferenceH Ogr_projection;
    char **papszDSCO = NULL, **papszLCO = NULL;
    int num_types;

    G_gisinit(argv[0]);

    /* Module options */
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("export"));
    G_add_keyword(_("ogr"));

    module->description =
	_("Converts a vector map to any of the supported OGR vector formats.");
    module->overwrite = TRUE;
    
    /* parse & read options */
    parse_args(argc, argv, &options, &flags);
    
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
       explicitely, using the "type=" option.
     */
    if (!flags.new->answer) {
	/* open input vector (topology required) */
	Vect_set_open_level(2);
	Vect_open_old2(&In, options.input->answer, "", options.field->answer);

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

	    if (num_types == 0)
		G_fatal_error(_("Unable to determine input map's vector feature type(s)."));
	}
    }

    field = Vect_get_field_number(&In, options.field->answer);

    /* check output feature type */
    otype = Vect_option_to_types(options.type);

    if (!options.layer->answer) {
	char xname[GNAME_MAX], xmapset[GMAPSET_MAX];

	if (G_name_is_fully_qualified(options.input->answer, xname, xmapset))
	    options.layer->answer = G_store(xname);
	else
	    options.layer->answer = G_store(options.input->answer);
    }

    if (otype & GV_POINTS)
	wkbtype = wkbPoint;
    else if (otype & GV_LINES)
	wkbtype = wkbLineString;
    else if (otype & GV_AREA)
	wkbtype = wkbPolygon;
    else if (otype & GV_FACE)
	wkbtype = wkbPolygon25D;
    else if (otype & GV_VOLUME)
	wkbtype = wkbPolygon25D;

    if (flags.poly->answer)
	wkbtype = wkbPolygon;

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
    if (cellhd.proj == PROJECTION_XY)
	Ogr_projection = NULL;
    else {
	projinfo = G_get_projinfo();
	projunits = G_get_projunits();
	Ogr_projection = GPJ_grass_to_osr(projinfo, projunits);
	if (flags.esristyle->answer &&
	    (strcmp(options.format->answer, "ESRI_Shapefile") == 0))
	    OSRMorphToESRI(Ogr_projection);
    }

    /* create new OGR layer in datasource */
    if (flags.new->answer) {
	const char *name;

	name =
	    options.layer->answer ? options.layer->answer : options.input->
	    answer;

	create_ogr_layer(options.dsn->answer, options.format->answer, name,
			 wkbtype, papszDSCO, papszLCO);

	G_message(_("OGR layer <%s> created in datasource <%s> (format '%s')"),
		  name, options.dsn->answer, options.format->answer);
	exit(EXIT_SUCCESS);
    }

    if (flags.cat->answer)
	donocat = 1;
    else
	donocat = 0;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    if ((GV_AREA & otype) && Vect_get_num_islands(&In) > 0 &&
	flags.cat->answer)
	G_warning(_("The map contains islands. With the -c flag, "
		    "islands will appear as filled areas, not holes in the output map."));


    /* check what users wants to export and what's present in the map */
    if (Vect_get_num_primitives(&In, GV_POINT) > 0 && !(otype & GV_POINTS))
	G_warning(_("%d point(s) found, but not requested to be exported. "
		    "Verify 'type' parameter."), Vect_get_num_primitives(&In,
									 GV_POINT));

    if (Vect_get_num_primitives(&In, GV_LINE) > 0 && !(otype & GV_LINES))
	G_warning(_("%d line(s) found, but not requested to be exported. "
		    "Verify 'type' parameter."), Vect_get_num_primitives(&In,
									 GV_LINE));

    if (Vect_get_num_primitives(&In, GV_BOUNDARY) > 0 &&
	!(otype & GV_BOUNDARY) && !(otype & GV_AREA))
	G_warning(_("%d boundary(ies) found, but not requested to be exported. "
		   "Verify 'type' parameter."), Vect_get_num_primitives(&In,
									GV_BOUNDARY));

    if (Vect_get_num_primitives(&In, GV_CENTROID) > 0 &&
	!(otype & GV_CENTROID) && !(otype & GV_AREA))
	G_warning(_("%d centroid(s) found, but not requested to be exported. "
		    "Verify 'type' parameter."), Vect_get_num_primitives(&In,
									 GV_CENTROID));

    if (Vect_get_num_areas(&In) > 0 && !(otype & GV_AREA))
	G_warning(_("%d area(s) found, but not requested to be exported. "
		    "Verify 'type' parameter."), Vect_get_num_areas(&In));

    if (Vect_get_num_primitives(&In, GV_FACE) > 0 && !(otype & GV_FACE))
	G_warning(_("%d face(s) found, but not requested to be exported. "
		    "Verify 'type' parameter."), Vect_get_num_primitives(&In,
									 GV_FACE));

    if (Vect_get_num_volumes(&In) > 0 && !(otype & GV_VOLUME))
	G_warning(_("%d volume(s) found, but not requested to be exported. "
		    "Verify 'type' parameter."), Vect_get_num_volumes(&In));

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

    if (num_to_export < 1) {
	G_warning(_("Nothing to export"));
	exit(EXIT_SUCCESS);
    }


    /* Open OGR DSN */
    G_debug(2, "driver count = %d", OGRGetDriverCount());
    drn = -1;
    for (i = 0; i < OGRGetDriverCount(); i++) {
	Ogr_driver = OGRGetDriver(i);
	G_debug(2, "driver %d : %s", i, OGR_Dr_GetName(Ogr_driver));
	/* chg white space to underscore in OGR driver names */
	sprintf(buf, "%s", OGR_Dr_GetName(Ogr_driver));
	G_strchg(buf, ' ', '_');
	if (strcmp(buf, options.format->answer) == 0) {
	    drn = i;
	    G_debug(2, " -> driver = %d", drn);
	}
    }
    if (drn == -1)
	G_fatal_error(_("OGR driver <%s> not found"), options.format->answer);
    Ogr_driver = OGRGetDriver(drn);
    
    if (flags.append->answer) {
	G_verbose_message(_("Append existing layer requires "
			    "opening data source in update mode, forcing '-%c' flag"),
			  flags.update->key);
	flags.update->answer = TRUE;
    }
    
    if (flags.update->answer) {
	G_debug(1, "Update OGR data source");
	Ogr_ds = OGR_Dr_Open(Ogr_driver, options.dsn->answer, TRUE);
    }
    else {
	G_debug(1, "Create OGR data source");
	Ogr_ds = OGR_Dr_CreateDataSource(Ogr_driver, options.dsn->answer,
					 papszDSCO);
    }
    
    CSLDestroy(papszDSCO);
    if (Ogr_ds == NULL)
	G_fatal_error(_("Unable to open OGR data source '%s'"),
		      options.dsn->answer);
    
    /* check if OGR layer exists */
    overwrite = G_check_overwrite(argc, argv);
    found = FALSE;
    for (i = 0; i < OGR_DS_GetLayerCount(Ogr_ds); i++) {
	Ogr_layer = OGR_DS_GetLayer(Ogr_ds, i);
	Ogr_field = OGR_L_GetLayerDefn(Ogr_layer);
	if (strcmp(OGR_FD_GetName(Ogr_field), options.layer->answer))
	    continue;
	
	found = TRUE;
	if (!overwrite && !flags.append->answer) {
	    G_fatal_error(_("Layer <%s> already exists in OGR data source '%s'"),
			  options.layer->answer, options.dsn->answer);
	}
	else if (overwrite) {
	    G_warning(_("OGR layer <%s> already exists and will be overwritten"),
		      options.layer->answer);
	    OGR_DS_DeleteLayer(Ogr_ds, i);
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
			"to export in 3D rather than 2D (default)."),
		      options.input->answer);
	}
    }

    G_debug(1, "Create OGR layer");
    if (flags.append->answer)
	Ogr_layer = OGR_DS_GetLayerByName(Ogr_ds, options.layer->answer);
    else 
	Ogr_layer = OGR_DS_CreateLayer(Ogr_ds, options.layer->answer, Ogr_projection, wkbtype,
				       papszLCO);
    
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
		OGR_L_CreateField(Ogr_layer, Ogr_field, 0);
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
		     ogr_ftype = OFTString;
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
		 OGR_L_CreateField(Ogr_layer, Ogr_field, 0);
		 
		 OGR_Fld_Destroy(Ogr_field);
	     }
	     if (keycol == -1)
		 G_fatal_error(_("Key column <%s> not found"), Fi->key);
	 }
    }
    
    Ogr_featuredefn = OGR_L_GetLayerDefn(Ogr_layer);

    fout = fskip = nocat = noatt = nocatskip = 0;

    if (OGR_L_TestCapability(Ogr_layer, OLCTransactions))
	OGR_L_StartTransaction(Ogr_layer);
    
    /* Lines (run always to count features of different type) */
    if ((otype & GV_POINTS) || (otype & GV_LINES)) {
        G_message(_("Exporting %d features..."), Vect_get_num_primitives(&In, otype));
	for (i = 1; i <= Vect_get_num_lines(&In); i++) {

	    G_percent(i, Vect_get_num_lines(&In), 1);

	    type = Vect_read_line(&In, Points, Cats, i);
	    G_debug(2, "line = %d type = %d", i, type);
	    if (!(otype & type)) {
		G_debug(2, "type %d not specified -> skipping", type);
		fskip++;
		continue;
	    }

	    Vect_cat_get(Cats, field, &cat);
	    if (cat < 0 && !donocat) {	/* Do not export not labeled */
		nocatskip++;
		continue;
	    }

	    /* Geometry */
	    if (type == GV_LINE && flags.poly->answer) {
		OGRGeometryH ring;

		ring = OGR_G_CreateGeometry(wkbLinearRing);
		Ogr_geometry = OGR_G_CreateGeometry(wkbPolygon);

		/* Area */
		for (j = 0; j < Points->n_points; j++) {
		    OGR_G_AddPoint(ring, Points->x[j], Points->y[j],
				   Points->z[j]);
		}
		if (Points->x[Points->n_points - 1] != Points->x[0] ||
		    Points->y[Points->n_points - 1] != Points->y[0] ||
		    Points->z[Points->n_points - 1] != Points->z[0]) {
		    OGR_G_AddPoint(ring, Points->x[0], Points->y[0],
				   Points->z[0]);
		}

		OGR_G_AddGeometryDirectly(Ogr_geometry, ring);
	    }
	    else if ((type == GV_POINT) ||
		     ((type == GV_CENTROID) && (otype & GV_CENTROID))) {
		Ogr_geometry = OGR_G_CreateGeometry(wkbPoint);
		OGR_G_AddPoint(Ogr_geometry, Points->x[0], Points->y[0],
			       Points->z[0]);
	    }
	    else {		/* GV_LINE or GV_BOUNDARY */
		Ogr_geometry = OGR_G_CreateGeometry(wkbLineString);
		for (j = 0; j < Points->n_points; j++) {
		    OGR_G_AddPoint(Ogr_geometry, Points->x[j], Points->y[j],
				   Points->z[j]);
		}
	    }
	    Ogr_feature = OGR_F_Create(Ogr_featuredefn);

	    OGR_F_SetGeometry(Ogr_feature, Ogr_geometry);

	    /* Output one feature for each category */
	    for (j = -1; j < Cats->n_cats; j++) {
		if (j == -1) {
		    if (cat >= 0)
			continue;	/* cat(s) exists */
		}
		else {
		    if (Cats->field[j] == field)
			cat = Cats->cat[j];
		    else
			continue;
		}

		mk_att(cat, Fi, Driver, ncol, colctype, colname, doatt, flags.nocat->answer,
		       Ogr_feature, &noatt, &fout);
		OGR_L_CreateFeature(Ogr_layer, Ogr_feature);
	    }
	    OGR_G_DestroyGeometry(Ogr_geometry);
	    OGR_F_Destroy(Ogr_feature);
	}
    }

    /* Areas (run always to count features of different type) */
    if (Vect_get_num_areas(&In) > 0 && otype & GV_AREA) {
	G_message(_("Exporting %i areas (may take some time)..."),
		  Vect_get_num_areas(&In));
	for (i = 1; i <= Vect_get_num_areas(&In); i++) {
	    OGRGeometryH ring;

	    G_percent(i, Vect_get_num_areas(&In), 1);

	    centroid = Vect_get_area_centroid(&In, i);
	    cat = -1;
	    if (centroid > 0) {
		Vect_read_line(&In, NULL, Cats, centroid);
		Vect_cat_get(Cats, field, &cat);
	    }
	    G_debug(3, "area = %d centroid = %d ncats = %d", i, centroid,
		    Cats->n_cats);
	    if (cat < 0 && !donocat) {	/* Do not export not labeled */
		nocatskip++;
		continue;
	    }

	    Vect_get_area_points(&In, i, Points);

	    /* Geometry */
	    Ogr_geometry = OGR_G_CreateGeometry(wkbPolygon);

	    ring = OGR_G_CreateGeometry(wkbLinearRing);

	    /* Area */
	    for (j = 0; j < Points->n_points; j++) {
		OGR_G_AddPoint(ring, Points->x[j], Points->y[j],
			       Points->z[j]);
	    }

	    OGR_G_AddGeometryDirectly(Ogr_geometry, ring);

	    /* Isles */
	    for (k = 0; k < Vect_get_area_num_isles(&In, i); k++) {
		Vect_get_isle_points(&In, Vect_get_area_isle(&In, i, k),
				     Points);

		ring = OGR_G_CreateGeometry(wkbLinearRing);
		for (j = 0; j < Points->n_points; j++) {
		    OGR_G_AddPoint(ring, Points->x[j], Points->y[j],
				   Points->z[j]);
		}
		OGR_G_AddGeometryDirectly(Ogr_geometry, ring);
	    }

	    Ogr_feature = OGR_F_Create(Ogr_featuredefn);
	    OGR_F_SetGeometry(Ogr_feature, Ogr_geometry);

	    /* Output one feature for each category */
	    for (j = -1; j < Cats->n_cats; j++) {
		if (j == -1) {
		    if (cat >= 0)
			continue;	/* cat(s) exists */
		}
		else {
		    if (Cats->field[j] == field)
			cat = Cats->cat[j];
		    else
			continue;
		}

		mk_att(cat, Fi, Driver, ncol, colctype, colname, doatt, flags.nocat->answer,
		       Ogr_feature, &noatt, &fout);
		OGR_L_CreateFeature(Ogr_layer, Ogr_feature);
	    }
	    OGR_G_DestroyGeometry(Ogr_geometry);
	    OGR_F_Destroy(Ogr_feature);
	}
    }

    /* Faces (run always to count features of different type)  - Faces are similar to lines */
    if (Vect_get_num_primitives(&In, GV_FACE) > 0 && otype & GV_FACE) {
	G_message(_("Exporting %i faces..."), Vect_get_num_faces(&In));
	for (i = 1; i <= Vect_get_num_faces(&In); i++) {
	    OGRGeometryH ring;

	    G_percent(i, Vect_get_num_faces(&In), 1);

	    type = Vect_read_line(&In, Points, Cats, i);
	    G_debug(3, "line type = %d", type);

	    cat = -1;
	    Vect_cat_get(Cats, field, &cat);

	    G_debug(3, "face = %d ncats = %d", i, Cats->n_cats);
	    if (cat < 0 && !donocat) {	/* Do not export not labeled */
		nocatskip++;
		continue;
	    }

	    if (type & GV_FACE) {

		Ogr_feature = OGR_F_Create(Ogr_featuredefn);

		/* Geometry */
		Ogr_geometry = OGR_G_CreateGeometry(wkbPolygon25D);
		ring = OGR_G_CreateGeometry(wkbLinearRing);

		/* Face */
		for (j = 0; j < Points->n_points; j++) {
		    OGR_G_AddPoint(ring, Points->x[j], Points->y[j],
				   Points->z[j]);
		}

		OGR_G_AddGeometryDirectly(Ogr_geometry, ring);

		OGR_F_SetGeometry(Ogr_feature, Ogr_geometry);

		/* Output one feature for each category */
		for (j = -1; j < Cats->n_cats; j++) {
		    if (j == -1) {
			if (cat >= 0)
			    continue;	/* cat(s) exists */
		    }
		    else {
			if (Cats->field[j] == field)
			    cat = Cats->cat[j];
			else
			    continue;
		    }

		    mk_att(cat, Fi, Driver, ncol, colctype, colname, doatt, flags.nocat->answer,
			   Ogr_feature, &noatt, &fout);
		    OGR_L_CreateFeature(Ogr_layer, Ogr_feature);
		}

		OGR_G_DestroyGeometry(Ogr_geometry);
		OGR_F_Destroy(Ogr_feature);
	    }			/* if type & GV_FACE */
	}			/* for */
    }

    /* Kernels */
    if (Vect_get_num_primitives(&In, GV_KERNEL) > 0 && otype & GV_KERNEL) {
	G_message(_("Exporting %i kernels..."), Vect_get_num_kernels(&In));
	for (i = 1; i <= Vect_get_num_lines(&In); i++) {

	    G_percent(i, Vect_get_num_lines(&In), 1);

	    type = Vect_read_line(&In, Points, Cats, i);
	    G_debug(2, "line = %d type = %d", i, type);
	    if (!(otype & type)) {
		G_debug(2, "type %d not specified -> skipping", type);
		fskip++;
		continue;
	    }

	    Vect_cat_get(Cats, field, &cat);
	    if (cat < 0 && !donocat) {	/* Do not export not labeled */
		nocatskip++;
		continue;
	    }

	    /* Geometry */
	    if (type == GV_KERNEL) {
		Ogr_geometry = OGR_G_CreateGeometry(wkbPoint);
		OGR_G_AddPoint(Ogr_geometry, Points->x[0], Points->y[0],
			       Points->z[0]);

		Ogr_feature = OGR_F_Create(Ogr_featuredefn);

		OGR_F_SetGeometry(Ogr_feature, Ogr_geometry);

		/* Output one feature for each category */
		for (j = -1; j < Cats->n_cats; j++) {
		    if (j == -1) {
			if (cat >= 0)
			    continue;	/* cat(s) exists */
		    }
		    else {
			if (Cats->field[j] == field)
			    cat = Cats->cat[j];
			else
			    continue;
		    }

		    mk_att(cat, Fi, Driver, ncol, colctype, colname, doatt, flags.nocat->answer,
			   Ogr_feature, &noatt, &fout);
		    OGR_L_CreateFeature(Ogr_layer, Ogr_feature);
		}
		OGR_G_DestroyGeometry(Ogr_geometry);
		OGR_F_Destroy(Ogr_feature);
	    }
	}
    }

    /*
       TODO:   Volumes. Do not export kernels here, that's already done.
       We do need to worry about holes, though.
       NOTE: We can probably just merge this with faces export function.
       Except for GRASS, which output format would know the difference?
     */
    if ((otype & GV_VOLUME)) {
	G_message(_("Exporting %i volumes..."), Vect_get_num_volumes(&In));
	G_warning(_("Export of volumes not implemented yet. Skipping."));
    }

    if (OGR_L_TestCapability(Ogr_layer, OLCTransactions))
	OGR_L_CommitTransaction(Ogr_layer);

    OGR_DS_Destroy(Ogr_ds);

    Vect_close(&In);

    if (doatt) {
	db_close_database(Driver);
	db_shutdown_driver(Driver);
    }

    /* Summary */
    if (nocat > 0)
	G_warning(_("%d features without category were written"), nocat);
    if (noatt > 0)
	G_warning(_("%d features without attributes were written"), noatt);
    if (nocatskip > 0)
	G_message(_("%d features found without category were skipped"),
		  nocatskip);

    /* Enable this? May be confusing that for area type are not reported
     *    all boundaries/centroids.
     *  OTOH why should be reported? */
    /*
       if (((otype & GV_POINTS) || (otype & GV_LINES)) && fskip > 0)
       G_warning ("%d features of different type skip", fskip);
     */

    G_done_msg(_("%d features (%s) written to <%s> (%s format)."), fout,
               OGRGeometryTypeToName(wkbtype),
	       options.layer->answer, options.format->answer);

    exit(EXIT_SUCCESS);
}
