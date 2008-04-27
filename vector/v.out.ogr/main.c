/*
 ***************************************************************
 *
 * MODULE:	 v.out.ogr
 * 
 * AUTHOR(S):	 Radim Blazek
 *		 Some extensions: Markus Neteler
 *
 * PURPOSE:	 Category manipulations
 *		 
 * COPYRIGHT:	 (C) 2001 by the GRASS Development Team
 *
 *		 This program is free software under the 
 *		 GNU General Public License (>=v2). 
 *		 Read the file COPYING that comes with GRASS
 *		 for details.
 *
 **************************************************************/
#include <stdlib.h> 
#include <string.h> 
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/Vect.h>
#include <grass/config.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include "ogr_api.h"
#include "cpl_string.h"

int    fout, fskip; /* features written/ skip */
int    nocat, noatt, nocatskip; /* number of features without cats/atts written/skip */
	
int mk_att ( int cat, struct field_info *Fi, dbDriver *Driver,
	     int ncol, int keycol, int doatt, OGRFeatureH Ogr_feature);

char *OGR_list_write_drivers();
char OGRdrivers[2000];

int 
main (int argc, char *argv[])
{
    int    i, j, k, centroid, otype, donocat;
    char   *mapset;
    int    field;
    struct GModule *module;
    struct Option *in_opt, *dsn_opt, *layer_opt, *type_opt, *frmt_opt, *field_opt, *dsco, *lco;
    struct Flag   *cat_flag, *esristyle, *poly_flag;
    char   buf[2000];
    char   key1[200], key2[200];
    struct Key_Value *projinfo, *projunits;
    struct Cell_head cellhd;
    char   **tokens;

    /* Vector */
    struct Map_info In;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int    type, cat;

    /* Attributes */
    int doatt=0, ncol=0, colsqltype, colctype, keycol=-1;
    struct field_info *Fi=NULL;
    dbDriver *Driver=NULL;
    dbHandle handle;
    dbTable *Table;
    dbString dbstring;
    dbColumn *Column;
    
    /* OGR */
    int drn, ogr_ftype=OFTInteger;
    OGRDataSourceH Ogr_ds;
    OGRSFDriverH Ogr_driver;  
    OGRLayerH Ogr_layer;	
    OGRFieldDefnH Ogr_field; 
    OGRFeatureH Ogr_feature;  
    OGRFeatureDefnH Ogr_featuredefn;
    OGRGeometryH Ogr_geometry;
    unsigned int wkbtype=wkbUnknown;  /* ?? */
    OGRSpatialReferenceH Ogr_projection;
    char **papszDSCO = NULL, **papszLCO = NULL;

    G_gisinit(argv[0]);

    /* Module options */
    module = G_define_module();
    module->keywords = _("vector, export");
    module->description =
	_("Converts to one of the supported OGR vector formats.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);

    type_opt = G_define_standard_option(G_OPT_V_TYPE) ;
    type_opt->options = "point,kernel,centroid,line,boundary,area,face";
    type_opt->answer = "line,boundary";
    type_opt->description =
	_("Feature type. Combination of types is not supported "
          "by all formats.");
    type_opt->guisection = _("Input");

    dsn_opt = G_define_option();
    dsn_opt->key = "dsn";
    dsn_opt->type =  TYPE_STRING;
    dsn_opt->required = YES;
    dsn_opt->label = _("OGR output datasource name");
    dsn_opt->description = _("For example: ESRI Shapefile: filename or directory for storage");

    layer_opt = G_define_option();
    layer_opt->key = "olayer";
    layer_opt->type = TYPE_STRING;
    layer_opt->required = NO;
    layer_opt->label = _("OGR layer name. If not specified, input name is used.");
    layer_opt->description = _("For example: ESRI Shapefile: shapefile name");
    layer_opt->guisection = _("Creation");
    
    field_opt = G_define_standard_option(G_OPT_V_FIELD);
    field_opt->guisection = _("Input");

    frmt_opt = G_define_option();
    frmt_opt->key = "format";
    frmt_opt->type =  TYPE_STRING;
    frmt_opt->required = NO;
    frmt_opt->multiple = NO;
    frmt_opt->answer = "ESRI_Shapefile";
    frmt_opt->options = OGR_list_write_drivers();
    frmt_opt->description = _("OGR format");
    frmt_opt->guisection = _("Creation");

    dsco              = G_define_option();
    dsco->key         = "dsco";
    dsco->type        = TYPE_STRING;
    dsco->required    = NO;
    dsco->multiple    = YES;
    dsco->answer      = "";
    dsco->description =
	_("OGR dataset creation option (format specific, NAME=VALUE)");
    dsco->guisection  = _("Creation");

    lco               = G_define_option();
    lco->key          = "lco";
    lco->type         = TYPE_STRING;
    lco->required     = NO;
    lco->multiple     = YES;
    lco->answer       = "";
    lco->description  =
	_("OGR layer creation option (format specific, NAME=VALUE)");
    lco->guisection   = _("Creation");
	
    cat_flag = G_define_flag ();
    cat_flag->key            = 'c';
    cat_flag->description    = _("Export features with category (labeled) only. "
				 "Otherwise all features are exported");

    esristyle = G_define_flag();
    esristyle->key = 'e';
    esristyle->description = _("Use ESRI-style .prj file format "
			       "(applies to Shapefile output only)");

    poly_flag = G_define_flag();
    poly_flag->key = 'p';
    poly_flag->description = _("Export lines as polygons");
		
    if (G_parser (argc, argv))
	exit(EXIT_FAILURE); 

    
    /* read options */
    field = atoi( field_opt->answer );
    
    /* Check output type */
    otype = Vect_option_to_types ( type_opt ); 
    
    if (! layer_opt->answer)
       layer_opt->answer = G_store(in_opt->answer);
    
    if ( otype & GV_POINTS ) wkbtype = wkbPoint;
    else if ( otype & GV_LINES ) wkbtype = wkbLineString;
    else if ( otype & GV_AREA ) wkbtype = wkbPolygon;
    else if ( otype & GV_FACE ) wkbtype = wkbPolygon25D;

    if ( poly_flag->answer ) wkbtype = wkbPolygon;

    if ( ((GV_POINTS & otype) && (GV_LINES & otype)) ||
	 ((GV_POINTS & otype) && (GV_AREA & otype)) || 
	 ((GV_POINTS & otype) && (GV_FACE & otype)) || 
	 ((GV_LINES & otype) && (GV_AREA & otype)) ||
	 ((GV_LINES & otype) && (GV_FACE & otype))
       ) 
      {
	 G_warning (_("The combination of types is not supported"
                      " by all formats."));
          wkbtype=wkbUnknown;
      }

    
    if ( cat_flag->answer ) donocat = 0; else donocat = 1;
    
    Points = Vect_new_line_struct ();
    Cats = Vect_new_cats_struct ();
    
    /* open input vector */
    if ((mapset = G_find_vector2 (in_opt->answer, "")) == NULL) {
	 G_fatal_error (_("Vector map <%s> not found"), in_opt->answer);
    }
    
    Vect_set_open_level (2); 
    Vect_open_old (&In, in_opt->answer, mapset); 

    /* fetch PROJ info */
    G_get_default_window(&cellhd);
    if( cellhd.proj == PROJECTION_XY )
        Ogr_projection = NULL;
    else
    {
        projinfo = G_get_projinfo();
        projunits = G_get_projunits();
        Ogr_projection = GPJ_grass_to_osr(projinfo, projunits);
        if (esristyle->answer && (strcmp(frmt_opt->answer, "ESRI_Shapefile") == 0))
           OSRMorphToESRI(Ogr_projection);
    }

    /* Open OGR DSN */
    OGRRegisterAll();
    G_debug (2, "driver count = %d", OGRGetDriverCount() ); 
    drn = -1;
    for ( i = 0; i < OGRGetDriverCount(); i++ ) {
	Ogr_driver = OGRGetDriver( i );  
	G_debug (2, "driver %d : %s", i, OGR_Dr_GetName ( Ogr_driver) ); 
        /* chg white space to underscore in OGR driver names */
	sprintf (buf, "%s", OGR_Dr_GetName ( Ogr_driver) );
	G_strchg(buf,' ','_');
	if ( strcmp ( buf ,  frmt_opt->answer ) == 0 ) {
	    drn = i;
	    G_debug (2, " -> driver = %d", drn); 
	}
    }
    if ( drn == -1 ) G_fatal_error ( _("OGR driver <%s> not found"), frmt_opt->answer ); 
    Ogr_driver = OGRGetDriver( drn );

    /* parse dataset creation options */
    i = 0;
    while (dsco->answers[i]) {
	tokens = G_tokenize(dsco->answers[i], "=");
	if (G_number_of_tokens(tokens))
	    papszDSCO = CSLSetNameValue(papszDSCO, tokens[0], tokens[1]);
	G_free_tokens(tokens);
	i++;
    }

    papszDSCO = dsco->answers;
    Ogr_ds = OGR_Dr_CreateDataSource( Ogr_driver, dsn_opt->answer, papszDSCO );
    CSLDestroy( papszDSCO );
    if ( Ogr_ds == NULL ) G_fatal_error (_("Unable to open OGR data source '%s'"), dsn_opt->answer);

    /* parse layer creation options */
    i = 0;
    while (lco->answers[i]) {
	tokens = G_tokenize(lco->answers[i], "=");
	if (G_number_of_tokens(tokens))
	    papszLCO = CSLSetNameValue(papszLCO, tokens[0], tokens[1]);
	G_free_tokens(tokens);
	i++;
    }

    /* check if the map is 3d */
    if (Vect_is_3d(&In)) {
	/* specific check for shp */
	if (strcmp(frmt_opt->answer, "ESRI_Shapefile") == 0) {
	    const char *shpt;
	    shpt = CSLFetchNameValue(papszLCO, "SHPT");
	    if (!shpt || shpt[strlen(shpt)-1] != 'Z') {
		G_warning (_("Vector map <%s> is 3D. "
			     "Use format specific layer creation options (parameter 'lco') "
			     "to export in 3D rather than 2D (default)"),
			   in_opt->answer);
	    }
	}
	else {
	    G_warning (_("Vector map <%s> is 3D. "
			 "Use format specific layer creation options (parameter 'lco') "
			 "to export in 3D rather than 2D (default)"),
		       in_opt->answer);
	}
    }

    Ogr_layer = OGR_DS_CreateLayer( Ogr_ds, layer_opt->answer, Ogr_projection, wkbtype, papszLCO );
    CSLDestroy( papszLCO );
    if ( Ogr_layer == NULL ) G_fatal_error (_("Unable to create OGR layer"));
    
    db_init_string(&dbstring);

    /* Vector attributes -> OGR fields */
    if ( field > 0 ) {
	 doatt = 1; /* do attributes */
	 Fi = Vect_get_field( &In, field);
	 if ( Fi == NULL ) {
	     G_warning (_("No attribute table found -> using only category numbers as attributes"));
	     
	     Ogr_field = OGR_Fld_Create( "cat", OFTInteger ); 
	     OGR_L_CreateField( Ogr_layer, Ogr_field, 0 ); 
	     
	     doatt = 0;
	 } else {  
	     Driver = db_start_driver(Fi->driver);
	     if (Driver == NULL) G_fatal_error(_("Unable to start driver <%s>"), Fi->driver);

	     db_init_handle (&handle);
	     db_set_handle (&handle, Fi->database, NULL);
	     if (db_open_database(Driver, &handle) != DB_OK)
		 G_fatal_error(_("Unable to open database <%s> by driver <%s>"), Fi->database, Fi->driver);

	     db_set_string(&dbstring, Fi->table);
	     if(db_describe_table (Driver, &dbstring, &Table) != DB_OK) 
		 G_fatal_error(_("Unable to describe table <%s>"), Fi->table);

	     ncol = db_get_table_number_of_columns(Table); 
	     G_debug (2, "ncol = %d", ncol );
	     keycol = -1;
	     for (i = 0; i < ncol; i++) {
		 Column = db_get_table_column (Table, i);
		 colsqltype = db_get_column_sqltype(Column);
		 G_debug (2, "col %d: %s (%s)", i, db_get_column_name(Column),
			   db_sqltype_name(colsqltype) );
		 colctype = db_sqltype_to_Ctype ( colsqltype );

		 switch ( colctype ) {
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
		 G_debug (2, "ogr_ftype = %d", ogr_ftype );

		 strcpy ( key1, Fi->key ); G_tolcase ( key1 );
		 strcpy ( key2, db_get_column_name(Column) ); G_tolcase ( key2 );
		 if ( strcmp(key1, key2) == 0 ) keycol = i;
		 G_debug (2, "%s x %s -> %s x %s -> keycol = %d", Fi->key, 
			 db_get_column_name(Column), key1, key2, keycol );
		 
		 Ogr_field = OGR_Fld_Create( db_get_column_name(Column), ogr_ftype ); 
		 OGR_L_CreateField( Ogr_layer, Ogr_field, 0 ); 
	     }
	     if ( keycol == -1 ) G_fatal_error (_("Key column '%s' not found"), Fi->key );
	 }
	 
    }

    Ogr_featuredefn = OGR_L_GetLayerDefn( Ogr_layer );
    Ogr_feature = OGR_F_Create( Ogr_featuredefn );

    fout = fskip = nocat = noatt = nocatskip = 0;

    /* check what users wants to export and what's present in the map */
    if ( Vect_get_num_primitives(&In, GV_POINT) > 0 && !(otype & GV_POINTS) )
      G_warning(_("%d point(s) found, but not requested to be exported. "
		  "Verify 'type' parameter."),Vect_get_num_primitives(&In, GV_POINT));

    if ( Vect_get_num_primitives(&In, GV_LINE) > 0 && !(otype & GV_LINES) )
      G_warning(_("%d line(s) found, but not requested to be exported. "
		  "Verify 'type' parameter."),Vect_get_num_primitives(&In, GV_LINE));

    if ( Vect_get_num_primitives(&In, GV_BOUNDARY) > 0 && !(otype & GV_BOUNDARY) && !(otype & GV_AREA) )
      G_warning(_("%d boundary(ies) found, but not requested to be exported. "
		  "Verify 'type' parameter."),Vect_get_num_primitives(&In, GV_BOUNDARY));

    if ( Vect_get_num_primitives(&In, GV_CENTROID) > 0 && !(otype & GV_CENTROID) && !(otype & GV_AREA) )
      G_warning(_("%d centroid(s) found, but not requested to be exported. "
		  "Verify 'type' parameter."),Vect_get_num_primitives(&In, GV_CENTROID));

    if ( Vect_get_num_primitives(&In, GV_AREA) > 0  && !(otype & GV_AREA) )
       G_warning(_("%d areas found, but not requested to be exported. "
		   "Verify 'type' parameter."),Vect_get_num_primitives(&In, GV_AREA));

    if ( Vect_get_num_primitives(&In, GV_FACE) > 0  && !(otype & GV_FACE) )
       G_warning(_("%d faces found, but not requested to be exported. "
		   "Verify 'type' parameter."),Vect_get_num_primitives(&In, GV_FACE));

    /* add? GV_KERNEL */

    /* Lines (run always to count features of different type) */
    if ( (otype & GV_POINTS) || (otype & GV_LINES) ) {
	G_message(_("Exporting %i points/lines..."), Vect_get_num_lines(&In) );
	for ( i = 1; i <= Vect_get_num_lines(&In) ; i++ ) {

	    G_percent(i,Vect_get_num_lines(&In),1);

	    type = Vect_read_line (&In, Points, Cats, i);
	    G_debug (2, "line = %d type = %d", i, type );
	    if ( !(otype & type ) ) {
		 G_debug (2, "type %d not specified -> skip", type );
		 fskip++;
		 continue;
	    }

	    Vect_cat_get (Cats, field, &cat);
	    if ( cat < 0 && !donocat ) { /* Do not export not labeled */ 
		nocatskip++;
		continue; 
	    }


	    /* Geometry */
            if ( type == GV_LINE && poly_flag->answer ) 
            {
                OGRGeometryH    ring;

                ring = OGR_G_CreateGeometry( wkbLinearRing );
                Ogr_geometry = OGR_G_CreateGeometry( wkbPolygon );

                /* Area */
                for ( j = 0; j < Points->n_points; j++ ) {
                    OGR_G_AddPoint( ring, Points->x[j], Points->y[j], Points->z[j] );
                }
                if ( Points->x[Points->n_points-1] != Points->x[0] ||
                     Points->y[Points->n_points-1] != Points->y[0] ||
                     Points->z[Points->n_points-1] != Points->z[0] )
                {
                    OGR_G_AddPoint( ring, Points->x[0], Points->y[0], Points->z[0] );
                }  
                
                OGR_G_AddGeometryDirectly ( Ogr_geometry, ring );
            } 
            else if ( type == GV_POINT )
            {
                Ogr_geometry = OGR_G_CreateGeometry( wkbPoint );
                OGR_G_AddPoint( Ogr_geometry, Points->x[0], Points->y[0], Points->z[0] );
            }
            else  /* GV_LINE or GV_BOUNDARY */
            {
                Ogr_geometry = OGR_G_CreateGeometry( wkbLineString );
                for ( j = 0; j < Points->n_points; j++ ) {
                    OGR_G_AddPoint( Ogr_geometry, Points->x[j], Points->y[j], Points->z[j] );
                }
            }
	    OGR_F_SetGeometry( Ogr_feature, Ogr_geometry ); 

	    /* Output one feature for each category */
	    for ( j = -1; j < Cats->n_cats; j++ ) {
		if ( j == -1 ) {
		    if ( cat >= 0 ) continue; /* cat(s) exists */
		} else {
		    if ( Cats->field[j] == field )
			cat = Cats->cat[j];
		    else 
			continue;
		}
	        
		mk_att ( cat, Fi, Driver, ncol, keycol, doatt, Ogr_feature);
	        OGR_L_CreateFeature( Ogr_layer, Ogr_feature ); 
	    }
	    OGR_G_DestroyGeometry( Ogr_geometry );
	}
    }

    /* Areas (run always to count features of different type) */
    if ( otype & GV_AREA ) {
	G_message(_("Exporting %i areas (may take some time)..."), Vect_get_num_areas(&In) );
	for ( i = 1; i <= Vect_get_num_areas(&In) ; i++ ) {
	    OGRGeometryH    ring;
	    
	    G_percent(i,Vect_get_num_areas(&In),1);
	    
	    centroid = Vect_get_area_centroid ( &In, i );
	    cat = -1;
	    if ( centroid > 0 ) {
		Vect_read_line (&In, NULL, Cats, centroid );
		Vect_cat_get (Cats, field, &cat);
	    }
	    G_debug (3, "area = %d centroid = %d ncats = %d", i, centroid, Cats->n_cats );
	    if ( cat < 0 && !donocat ) { /* Do not export not labeled */ 
		nocatskip++;
		continue; 
	    }

	    Vect_get_area_points ( &In, i, Points );

	    /* Geometry */
	    Ogr_geometry = OGR_G_CreateGeometry( wkbPolygon );
	
	    ring = OGR_G_CreateGeometry( wkbLinearRing );
	    
	    /* Area */
	    for ( j = 0; j < Points->n_points; j++ ) {
		OGR_G_AddPoint( ring, Points->x[j], Points->y[j], Points->z[j] );
	    }
	    
	    OGR_G_AddGeometryDirectly ( Ogr_geometry, ring );

	    /* Isles */
	    for ( k = 0; k < Vect_get_area_num_isles (&In, i); k++ ) {
		Vect_get_isle_points ( &In, Vect_get_area_isle (&In, i, k), Points );

		ring = OGR_G_CreateGeometry( wkbLinearRing );
		for ( j = 0; j < Points->n_points; j++ ) {
		    OGR_G_AddPoint( ring, Points->x[j], Points->y[j], Points->z[j] );
		}
		OGR_G_AddGeometryDirectly ( Ogr_geometry, ring );
	    }
	    
	    OGR_F_SetGeometry( Ogr_feature, Ogr_geometry ); 

	    /* Output one feature for each category */
	    for ( j = -1; j < Cats->n_cats; j++ ) {
		if ( j == -1 ) {
		    if ( cat >= 0 ) continue; /* cat(s) exists */
		} else {
		    if ( Cats->field[j] == field )
			cat = Cats->cat[j];
		    else 
			continue;
		}
	        
		mk_att ( cat, Fi, Driver, ncol, keycol, doatt, Ogr_feature);
	        OGR_L_CreateFeature( Ogr_layer, Ogr_feature ); 
	    }
	    OGR_G_DestroyGeometry( Ogr_geometry );
	}
    }

    /* Faces (run always to count features of different type)  - Faces are similar to lines */
    if ( otype & GV_FACE ) {
	G_message(_("Exporting %i faces (may take some time) ..."), Vect_get_num_faces(&In) );
	for ( i = 1; i <= Vect_get_num_faces(&In) ; i++ ) {
	    OGRGeometryH    ring;
	    
	    G_percent(i,Vect_get_num_faces(&In),1);
	    
	    type = Vect_read_line(&In, Points, Cats, i);
	    G_debug(3, "line type = %d", type);

	    cat = -1;
	    Vect_cat_get (Cats, field, &cat);

	    G_debug (3, "face = %d ncats = %d", i, Cats->n_cats );
	    if ( cat < 0 && !donocat ) { /* Do not export not labeled */ 
		nocatskip++;
		continue; 
	    }

	    if ( type & GV_FACE ) {

		/* Geometry */
		Ogr_geometry = OGR_G_CreateGeometry( wkbPolygon25D );
		ring = OGR_G_CreateGeometry( wkbLinearRing );
	    
		/* Face */
		for ( j = 0; j < Points->n_points; j++ ) {
		    OGR_G_AddPoint( ring, Points->x[j], Points->y[j], Points->z[j] );
		}
	    
		OGR_G_AddGeometryDirectly ( Ogr_geometry, ring );
	    
		/* Output one feature for each category */
		for ( j = -1; j < Cats->n_cats; j++ ) {
		  if ( j == -1 ) {
		    if ( cat >= 0 ) continue; /* cat(s) exists */
		  } else {
		    if ( Cats->field[j] == field )
			cat = Cats->cat[j];
		    else 
			continue;
		  }
	        
		  mk_att ( cat, Fi, Driver, ncol, keycol, doatt, Ogr_feature);
	          OGR_L_CreateFeature( Ogr_layer, Ogr_feature ); 
		}

		OGR_F_SetGeometry( Ogr_feature, Ogr_geometry ); 

		OGR_G_DestroyGeometry( Ogr_geometry );
            } /* if type & GV_FACE */

	} /* for */
    }

	    
    OGR_F_Destroy( Ogr_feature );
    OGR_DS_Destroy( Ogr_ds );
    
    Vect_close (&In);

    if ( doatt ) {
	db_close_database(Driver);
	db_shutdown_driver(Driver);
    }

    /* Summary */
    G_message(_("%d features written"), fout);
    if ( nocat > 0 ) G_warning (_("%d features without category written"), nocat);
    if ( noatt > 0 ) G_warning (_("%d features without attributes written"), noatt);
    if ( nocatskip > 0 ) G_warning (_("%d features found without category skip"), nocatskip);

    /* Enable this? May be confusing that for area type are not reported all boundaries/centroids.
    *  OTOH why should be reported? */
    /*
    if ( ((otype & GV_POINTS) || (otype & GV_LINES)) && fskip > 0 ) 
	G_warning ( "%d features of different type skip", fskip);
    */
	 
    exit(EXIT_SUCCESS) ;
}

int
mk_att ( int cat, struct field_info *Fi, dbDriver *Driver, int ncol, int keycol, int doatt, 
	 OGRFeatureH Ogr_feature)
{
    int      j;
    char     buf[2000];
    int      colsqltype, colctype, more;
    dbTable  *Table;
    dbString dbstring;
    dbColumn *Column;
    dbCursor cursor;
    dbValue  *Value;

    G_debug (2, "mk_att() cat = %d, doatt = %d", cat, doatt );
    db_init_string(&dbstring);
    
    /* Attributes */
    /* Reset */
    if ( doatt ) {
	for (j = 0; j < ncol; j++) OGR_F_UnsetField(Ogr_feature, j);
    } else {
	OGR_F_UnsetField(Ogr_feature, 0);
    }
    
    /* Read & set attributes */
    if( cat >= 0 ) { /* Line with category */
	if ( doatt ) {
	    sprintf ( buf, "SELECT * FROM %s WHERE %s = %d", Fi->table,  Fi->key, cat); 
	    G_debug (2, "SQL: %s", buf);
	    db_set_string(&dbstring, buf);
	    if ( db_open_select_cursor(Driver, &dbstring, &cursor, DB_SEQUENTIAL) != DB_OK ) {
		G_fatal_error ( _("Cannot select attributes for cat = %d"), cat);
	    } else {
		if(db_fetch (&cursor, DB_NEXT, &more) != DB_OK) G_fatal_error (_("Unable to fetch data from table"));
		if (!more) {
		    /* G_warning ("No database record for cat = %d", cat); */
		    /* Set at least key column to category */
		    OGR_F_SetFieldInteger ( Ogr_feature, keycol, cat );
		    noatt++;
		} else {
		    Table = db_get_cursor_table (&cursor);
		    for (j = 0; j < ncol; j++) { 
			Column = db_get_table_column (Table, j);
			Value  = db_get_column_value(Column);
			db_convert_column_value_to_string (Column, &dbstring); /* for debug only */
			G_debug (2, "col %d : val = %s", j, db_get_string (&dbstring) );
			
			colsqltype = db_get_column_sqltype(Column);
			colctype = db_sqltype_to_Ctype ( colsqltype );
			G_debug (2, "  colctype = %d", colctype );
			switch ( colctype ) {
			     case DB_C_TYPE_INT:
				OGR_F_SetFieldInteger( Ogr_feature, j, db_get_value_int(Value) );  
				break; 
			     case DB_C_TYPE_DOUBLE:
				OGR_F_SetFieldDouble( Ogr_feature, j, db_get_value_double(Value) );  
				break; 
			     case DB_C_TYPE_STRING:
				OGR_F_SetFieldString( Ogr_feature, j, db_get_value_string(Value) );  
				break; 
			     case DB_C_TYPE_DATETIME:
				db_convert_column_value_to_string (Column, &dbstring);
				OGR_F_SetFieldString( Ogr_feature, j, db_get_string (&dbstring) );  
				break; 
			}
		    }
		}
	    }

	} else { /* Use cat only */
	    OGR_F_SetFieldInteger( Ogr_feature, 0, cat );  
	}
    } else { 
	/* G_warning ( "Line without cat of layer %d", field); */
	nocat++;
    }
    fout++;

    return 1;
}

/* to print available drivers in help text */
char *
OGR_list_write_drivers(void)
{
    int drn, i;
    OGRSFDriverH Ogr_driver;  
    char buf[2000];
    
    /* Open OGR DSN */
    OGRRegisterAll();
    G_debug (2, "driver count = %d", OGRGetDriverCount() ); 
    drn = -1;
    for ( i = 0; i < OGRGetDriverCount(); i++ ) {
        /* only fetch read/write drivers */
	if (OGR_Dr_TestCapability( OGRGetDriver(i),ODrCCreateDataSource) )
	{
	  Ogr_driver = OGRGetDriver( i ); 
	  G_debug (2, "driver %d/%d : %s", i, OGRGetDriverCount(), OGR_Dr_GetName ( Ogr_driver) ); 
          /* chg white space to underscore in OGR driver names */
	  sprintf (buf, "%s", OGR_Dr_GetName ( Ogr_driver) );
	  G_strchg(buf,' ','_');
	  strcat (OGRdrivers, buf);
	  if (i < OGRGetDriverCount() - 1 )
	     strcat (OGRdrivers, ",");
	}
    }
    G_debug (2, "all drivers: %s",OGRdrivers);
    return OGRdrivers;
}
