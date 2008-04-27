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
#define MAIN
#include <grass/config.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/Vect.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include <gdal_version.h> /* needed for OFTDate */
#include "ogr_api.h"
#include "global.h"

#ifndef MAX
#  define MIN(a,b)      ((a<b) ? a : b)
#  define MAX(a,b)      ((a>b) ? a : b)
#endif

int geom(OGRGeometryH hGeom, struct Map_info *Map, int field, int cat, double min_area, int type, int mk_centr );
int centroid(OGRGeometryH hGeom, CENTR *Centr, SPATIAL_INDEX *Sindex, int field, int cat, double min_area, int type);

int
main (int argc, char *argv[])
{
    int    i, j, layer, arg_s_num, nogeom, ncnames;
    float  xmin=0., ymin=0., xmax=0., ymax=0.;
    int    ncols = 0, type;
    struct GModule *module;
    double min_area, snap;
    struct Option *dsn_opt, *out_opt, *layer_opt, *spat_opt, *where_opt, *min_area_opt;
    struct Option *snap_opt, *type_opt, *outloc_opt, *cnames_opt;
    struct Flag *list_flag, *no_clean_flag, *z_flag, *notab_flag, *region_flag;
    struct Flag *over_flag, *extend_flag, *formats_flag, *tolower_flag;
    char   buf[2000], namebuf[2000];
    char   *separator;
    struct Key_Value *loc_proj_info = NULL, *loc_proj_units = NULL;
    struct Key_Value *proj_info, *proj_units;
    struct Cell_head cellhd, loc_wind, cur_wind;
    char   error_msg[8192];

    /* Vector */
    struct Map_info Map;
    int    cat;

    /* Attributes */
    struct field_info *Fi;
    dbDriver *driver;
    dbString sql, strval;
    int dim, with_z;

    /* OGR */
    OGRDataSourceH Ogr_ds=NULL;
    OGRLayerH Ogr_layer;
    OGRFieldDefnH Ogr_field;
    char *Ogr_fieldname;
    OGRFieldType Ogr_ftype;
    OGRFeatureH Ogr_feature;
    OGRFeatureDefnH Ogr_featuredefn;
    OGRGeometryH Ogr_geometry, Ogr_oRing=NULL, poSpatialFilter=NULL;
    OGRSpatialReferenceH Ogr_projection;
    OGREnvelope oExt;
    int OFTIntegerListlength = 40; /* hack due to limitation in OGR */

    char **layer_names; /* names of layers to be imported */
    int *layers;  /* layer indexes */
    int nlayers; /* number of layers to import */
    char **available_layer_names; /* names of layers to be imported */
    int navailable_layers;
    int layer_id;
    int overwrite;
    FILE *output;

    G_gisinit(argv[0]);

    G_begin_polygon_area_calculations(); /* Used in geom() */

    module = G_define_module();
    module->keywords = _("vector, import");
    /* module->description = G_store (buf); */
    module->description = _("Convert OGR vector layers to GRASS vector map.");

    dsn_opt = G_define_option();
    dsn_opt->key = "dsn";
    dsn_opt->type =  TYPE_STRING;
    dsn_opt->required = NO;
    dsn_opt->gisprompt = "old_file,file,dsn";
    dsn_opt->label = _("OGR datasource name");
    dsn_opt->description = _("Examples:\n"
	"\t\tESRI Shapefile: directory containing shapefiles\n"
	"\t\tMapInfo File: directory containing mapinfo files");
    dsn_opt->guisection = _("Required");

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    out_opt->required = NO;
    out_opt->guisection = _("Required");

    layer_opt = G_define_option();
    layer_opt->key = "layer";
    layer_opt->type = TYPE_STRING;
    layer_opt->required = NO;
    layer_opt->multiple = YES;
    layer_opt->label = _("OGR layer name. If not given, all available layers are imported");
    layer_opt->description = _("Examples:\n"
	"\t\tESRI Shapefile: shapefile name\n"
	"\t\tMapInfo File: mapinfo file name");
    layer_opt->guisection = _("Query");

    spat_opt = G_define_option();
    spat_opt->key = "spatial";
    spat_opt->type = TYPE_DOUBLE;
    spat_opt->multiple = YES;
    spat_opt->required = NO;
    spat_opt->key_desc = "xmin,ymin,xmax,ymax";
    spat_opt->label = _("Import subregion only");
    spat_opt->guisection = _("Subregion");
    spat_opt->description = _("Format: xmin,ymin,xmax,ymax - usually W,S,E,N");

    where_opt = G_define_standard_option(G_OPT_WHERE);
    where_opt->guisection = _("Query");

    min_area_opt = G_define_option();
    min_area_opt->key = "min_area";
    min_area_opt->type = TYPE_DOUBLE;
    min_area_opt->required = NO;
    min_area_opt->answer = "0.0001";
    min_area_opt->label = _("Minimum size of area to be imported (square units)");
    min_area_opt->guisection = _("Min-area & snap");
    min_area_opt->description = _("Smaller areas and "
	"islands are ignored. Should be greater than snap^2");

    type_opt = G_define_standard_option(G_OPT_V_TYPE) ;
    type_opt->options = "point,line,boundary,centroid";
    type_opt->answer = "";
    type_opt->description = _("Optionally change default input type");
    type_opt->descriptions =
      _("point;import area centroids as points;"
	"line;import area boundaries as lines;"
	"boundary;import lines as area boundaries;"
	"centroid;import points as centroids");
    type_opt->guisection = _("Query");

    snap_opt = G_define_option();
    snap_opt->key = "snap";
    snap_opt->type = TYPE_DOUBLE;
    snap_opt->required = NO;
    snap_opt->answer = "-1";
    snap_opt->label = _("Snapping threshold for boundaries");
    snap_opt->guisection = _("Min-area & snap");
    snap_opt->description = _("'-1' for no snap");

    outloc_opt = G_define_option();
    outloc_opt->key = "location";
    outloc_opt->type = TYPE_STRING;
    outloc_opt->required = NO;
    outloc_opt->description = _("Name for new location to create");

    cnames_opt = G_define_option();
    cnames_opt->key = "cnames";
    cnames_opt->type = TYPE_STRING;
    cnames_opt->required = NO;
    cnames_opt->multiple = YES;
    cnames_opt->description =
	_("List of column names to be used instead of original names, "
	  "first is used for category column");
    cnames_opt->guisection = _("Attributes");

    list_flag = G_define_flag ();
    list_flag->key             = 'l';
    list_flag->description = _("List available layers in data source and exit");

    formats_flag = G_define_flag ();
    formats_flag->key  	      = 'f';
    formats_flag->description = _("List supported formats and exit");

    /* if using -c, you lose topological information ! */
    no_clean_flag = G_define_flag ();
    no_clean_flag->key             = 'c';
    no_clean_flag->description = _("Do not clean polygons (not recommended)");

    z_flag = G_define_flag ();
    z_flag->key             = 'z';
    z_flag->description = _("Create 3D output");

    notab_flag = G_define_flag ();
    notab_flag->key             = 't';
    notab_flag->description = _("Do not create attribute table");
    notab_flag->guisection = _("Attributes");

    over_flag = G_define_flag();
    over_flag->key = 'o';
    over_flag->description = _("Override dataset projection (use location's projection)");

    region_flag = G_define_flag();
    region_flag->key = 'r';
    region_flag->guisection = _("Subregion");
    region_flag->description = _("Limit import to the current region");

    extend_flag = G_define_flag();
    extend_flag->key = 'e';
    extend_flag->description = _("Extend location extents based on new dataset");

    tolower_flag = G_define_flag();
    tolower_flag->key = 'w';
    tolower_flag->description = _("Change column names to lowercase characters");
    tolower_flag->guisection = _("Attributes");

     /* The parser checks if the map already exists in current mapset, this is
     * wrong if location options is used, so we switch out the check and do it
     * in the module after the parser */
    overwrite = G_check_overwrite(argc, argv);

    if (G_parser (argc, argv))
	exit(EXIT_FAILURE);

    OGRRegisterAll(); 

    if (G_verbose() > G_verbose_min())
	output = stderr;
    else
	output = NULL;

    /* list supported formats */
    if(formats_flag->answer) {
	int iDriver;
	G_important_message (_("Available OGR Drivers:"));

	for(iDriver = 0; iDriver < OGRGetDriverCount(); iDriver++) {
	    OGRSFDriverH poDriver = OGRGetDriver(iDriver);

	    if (OGR_Dr_TestCapability(poDriver, ODrCCreateDataSource))
		fprintf (stdout, "  %s (read/write)\n",
			 OGR_Dr_GetName(poDriver));
	    else
		fprintf (stdout, "  %s (readonly)\n",
			 OGR_Dr_GetName(poDriver));
	}

	exit(EXIT_SUCCESS);
    }

    if (dsn_opt -> answer == NULL) {
	G_fatal_error (_("Required parameter <%s> not set"),
		       dsn_opt -> key);
    }

    min_area = atof (min_area_opt->answer);
    snap = atof (snap_opt->answer);
    type = Vect_option_to_types ( type_opt );

    ncnames = 0;
    if ( cnames_opt->answers ) {
	i = 0;
	while ( cnames_opt->answers[i++] ) {
	    ncnames++;
	}
    }

    /* Open OGR DSN */
    Ogr_ds = NULL;
    if (strlen (dsn_opt -> answer) > 0)
	Ogr_ds = OGROpen( dsn_opt->answer, FALSE, NULL );

    if (Ogr_ds == NULL)
	G_fatal_error (_("Unable to open data source <%s>"), dsn_opt->answer);

    /* Make a list of available layers */
    navailable_layers = OGR_DS_GetLayerCount(Ogr_ds);
    available_layer_names = (char **) G_malloc ( navailable_layers * sizeof (char *) );

    if ( list_flag->answer )
        G_important_message (_("Data source contains %d layers:"), navailable_layers );

    for ( i = 0; i < navailable_layers; i++ ) {
	Ogr_layer =  OGR_DS_GetLayer( Ogr_ds, i );
	Ogr_featuredefn = OGR_L_GetLayerDefn( Ogr_layer );
	available_layer_names[i] = G_store ( (char *)OGR_FD_GetName( Ogr_featuredefn ) );

	if ( list_flag->answer ) {
	    if ( i > 0 ) fprintf ( stdout, ", " );
	    fprintf ( stdout, "%s", available_layer_names[i] );
	}
    }
    if ( list_flag->answer ) {
	fprintf(stdout, "\n");
	fflush(stdout);
        exit(EXIT_SUCCESS);
    }

    /* check if output name was given */
    if (out_opt -> answer == NULL) {
	G_fatal_error (_("Required parameter <%s> not set"),
		       out_opt -> key);
    }

    if (G_legal_filename(out_opt->answer) < 0)
        G_fatal_error(_("<%s> is an illegal file name"), out_opt->answer);

    if ( !outloc_opt->answer ) { /* Check if the map exists */
	if ( G_find_vector2 (out_opt->answer, G_mapset()) ) {
	    if (overwrite)
	        G_warning ( _("Vector map <%s> already exists and will be overwritten"), out_opt->answer );
	    else
	        G_fatal_error ( _("Vector map <%s> already exists"), out_opt->answer );
	}
    }

    /* Make a list of layers to be imported */
    if ( layer_opt->answer ) { /* From option */
	nlayers = 0;
	while ( layer_opt->answers[nlayers] )
	    nlayers++;

	layer_names = (char **) G_malloc ( nlayers * sizeof (char *) );
	layers = (int *) G_malloc ( nlayers * sizeof (int) );

	for ( i = 0; i < nlayers; i++ ) {
	    layer_names[i] = G_store ( layer_opt->answers[i] );
	    /* Find it in the source */
	    layers[i] = -1;
	    for ( j = 0; j < navailable_layers; j++ ) {
		if ( strcmp( available_layer_names[j], layer_names[i]) == 0 ) {
		    layers[i] = j;
		    break;
		}
	    }
	    if ( layers[i] == -1 )
                G_fatal_error ( _("Layer <%s> not available"), layer_names[i]);
	}
    } else { /* use list of all layers */
	nlayers = navailable_layers;
	layer_names = available_layer_names;
	layers = (int *) G_malloc ( nlayers * sizeof (int) );
	for ( i = 0 ; i < nlayers; i++ )
	    layers[i] = i;
    }

    /* Get first imported layer to use for extents and projection check */
    Ogr_layer = OGR_DS_GetLayer( Ogr_ds, layers[0] );

    if ( region_flag->answer ) {
	if ( spat_opt->answer )
	    G_fatal_error(_("Select either the current region flag or the spatial option, not both"));

	G_get_window (&cur_wind);
	xmin=cur_wind.west;
	xmax=cur_wind.east;
	ymin=cur_wind.south;
	ymax=cur_wind.north;
    }
    if ( spat_opt->answer ) {
        /* See as reference: gdal/ogr/ogr_capi_test.c */

        /* cut out a piece of the map */
        /* order: xmin,ymin,xmax,ymax */
        arg_s_num = 0; i = 0;
        while ( spat_opt->answers[i] ) {
	   if ( i == 0 ) xmin=atof(spat_opt->answers[i]);
	   if ( i == 1 ) ymin=atof(spat_opt->answers[i]);
	   if ( i == 2 ) xmax=atof(spat_opt->answers[i]);
	   if ( i == 3 ) ymax=atof(spat_opt->answers[i]);
           arg_s_num++; i++;
        }
        if ( arg_s_num != 4 )
	    G_fatal_error ( _("4 parameters required for 'spatial' parameter"));
    }
    if ( spat_opt->answer || region_flag->answer) {
	G_debug( 2, "cut out with boundaries: xmin:%f ymin:%f xmax:%f ymax:%f",xmin,ymin,xmax,ymax);

	/* in theory this could be an irregular polygon */
	poSpatialFilter = OGR_G_CreateGeometry( wkbPolygon );
        Ogr_oRing = OGR_G_CreateGeometry( wkbLinearRing );
        OGR_G_AddPoint(Ogr_oRing, xmin, ymin, 0.0);
        OGR_G_AddPoint(Ogr_oRing, xmin, ymax, 0.0);
        OGR_G_AddPoint(Ogr_oRing, xmax, ymax, 0.0);
        OGR_G_AddPoint(Ogr_oRing, xmax, ymin, 0.0);
        OGR_G_AddPoint(Ogr_oRing, xmin, ymin, 0.0);
        OGR_G_AddGeometryDirectly(poSpatialFilter, Ogr_oRing);

        OGR_L_SetSpatialFilter(Ogr_layer, poSpatialFilter );
     }

    if ( where_opt->answer ) {

        /* select by attribute */	
        OGR_L_SetAttributeFilter (Ogr_layer, where_opt->answer );
     }

    /* fetch boundaries */
    if ( (OGR_L_GetExtent (Ogr_layer , &oExt, 1 )) == OGRERR_NONE ) {
	G_get_window ( &cellhd );
        cellhd.north  = oExt.MaxY;
        cellhd.south  = oExt.MinY;
        cellhd.west   = oExt.MinX;
        cellhd.east   = oExt.MaxX;
        cellhd.rows   = 20 ; /* TODO - calculate useful values */
        cellhd.cols   = 20 ;
        cellhd.ns_res = (cellhd.north-cellhd.south)/cellhd.rows;
        cellhd.ew_res = (cellhd.east-cellhd.west)/cellhd.cols;
    }
    else {
        cellhd.north  = 1.;
        cellhd.south  = 0.;
        cellhd.west   = 0.;
        cellhd.east   = 1.;
        cellhd.top    = 1.;
        cellhd.bottom = 1.;
        cellhd.rows   = 1 ;
        cellhd.rows3  = 1 ;
        cellhd.cols   = 1 ;
        cellhd.cols3  = 1 ;
        cellhd.depths = 1 ;
        cellhd.ns_res = 1.;
        cellhd.ns_res3= 1.;
        cellhd.ew_res = 1.;
        cellhd.ew_res3= 1.;
        cellhd.tb_res = 1.;
    }

    /* Fetch input map projection in GRASS form. */
    proj_info = NULL;
    proj_units = NULL;
    Ogr_projection = OGR_L_GetSpatialRef(Ogr_layer); /* should not be freed later */

    /* Do we need to create a new location? */
    if( outloc_opt->answer != NULL )
    {
        /* Convert projection information non-interactively as we can't
	 * assume the user has a terminal open */
        if ( GPJ_osr_to_grass( &cellhd, &proj_info,
			       &proj_units, Ogr_projection, 0) < 0 )
	    G_fatal_error (_("Unable to convert input map projection to GRASS "
			     "format; cannot create new location"));
	else
            G_make_location( outloc_opt->answer, &cellhd,
			     proj_info, proj_units, NULL );
    }
    else
    {
	int err = 0;

        /* Projection only required for checking so convert non-interactively */
        if ( GPJ_osr_to_grass( &cellhd, &proj_info,
			       &proj_units, Ogr_projection, 0) < 0 )
            G_warning (_("Unable to convert input map projection information to "
			 "GRASS format for checking"));

        /* Does the projection of the current location match the dataset? */
        /* G_get_window seems to be unreliable if the location has been changed */
        G__get_window ( &loc_wind, "", "DEFAULT_WIND", "PERMANENT");
        /* fetch LOCATION PROJ info */
        if (loc_wind.proj != PROJECTION_XY) {
            loc_proj_info = G_get_projinfo();
            loc_proj_units = G_get_projunits();
	}

        if( over_flag->answer )
        {
            cellhd.proj = loc_wind.proj;
            cellhd.zone = loc_wind.zone;
	    G_message(_("Over-riding projection check"));
        }
        else if( loc_wind.proj != cellhd.proj
              || (err = G_compare_projections( loc_proj_info, loc_proj_units,
                                              proj_info, proj_units )) != TRUE)
        {
            int     i_value;

            strcpy( error_msg,
                    _("Projection of dataset does not"
                    " appear to match current location.\n\n"));

	    /* TODO: output this info sorted by key: */
	    if(loc_wind.proj != cellhd.proj || err != -2)
	    {
                if( loc_proj_info != NULL )
                {
                    strcat( error_msg, _("GRASS LOCATION PROJ_INFO is:\n" ));
                    for( i_value = 0; i_value < loc_proj_info->nitems; i_value++ )
                        sprintf( error_msg + strlen(error_msg), "%s: %s\n",
                                 loc_proj_info->key[i_value],
                                 loc_proj_info->value[i_value] );
                    strcat( error_msg, "\n" );
                }

                if( proj_info != NULL )
                {
                    strcat( error_msg, _("Import dataset PROJ_INFO is:\n" ));
                    for( i_value = 0; i_value < proj_info->nitems; i_value++ )
                        sprintf( error_msg + strlen(error_msg), "%s: %s\n",
                                 proj_info->key[i_value],
                                 proj_info->value[i_value] );
                }
                else
                {
                    strcat( error_msg, _("Import dataset PROJ_INFO is:\n" ));
		    if( cellhd.proj == PROJECTION_XY )
                        sprintf( error_msg + strlen(error_msg),
                                 "Dataset proj = %d (unreferenced/unknown)\n",
                                 cellhd.proj );
                    else if( cellhd.proj == PROJECTION_LL )
                        sprintf( error_msg + strlen(error_msg),
                                 "Dataset proj = %d (lat/long)\n",
                                 cellhd.proj );
                    else if( cellhd.proj == PROJECTION_UTM )
                        sprintf( error_msg + strlen(error_msg),
                                 "Dataset proj = %d (UTM), zone = %d\n",
                                 cellhd.proj, cellhd.zone );
                    else if( cellhd.proj == PROJECTION_SP )
                        sprintf( error_msg + strlen(error_msg),
                                 "Dataset proj = %d (State Plane), zone = %d\n",
                                 cellhd.proj, cellhd.zone );
                    else
                        sprintf( error_msg + strlen(error_msg),
                                 "Dataset proj = %d (unknown), zone = %d\n",
                                 cellhd.proj, cellhd.zone );
                }
	    }else{
                if( loc_proj_units != NULL )
                {
                    strcat( error_msg, "GRASS LOCATION PROJ_UNITS is:\n" );
                    for( i_value = 0; i_value < loc_proj_units->nitems; i_value++ )
                        sprintf( error_msg + strlen(error_msg), "%s: %s\n",
                                 loc_proj_units->key[i_value],
                                 loc_proj_units->value[i_value] );
                    strcat( error_msg, "\n" );
                }

                if( proj_units != NULL )
                {
                    strcat( error_msg, "Import dataset PROJ_UNITS is:\n" );
                    for( i_value = 0; i_value < proj_units->nitems; i_value++ )
                        sprintf( error_msg + strlen(error_msg), "%s: %s\n",
                                 proj_units->key[i_value],
                                 proj_units->value[i_value] );
                }
	    }
            sprintf( error_msg + strlen(error_msg),
    	             _("\nYou can use the -o flag to %s to override this projection check.\n"),
    	    	     G_program_name() );
            strcat( error_msg,
             _("Consider generating a new location with 'location' parameter"
             " from input data set.\n") );
            G_fatal_error( error_msg );
        }
        else {
	    G_message( _("Projection of input dataset and current location "
			 "appear to match"));
	}
    }

    db_init_string (&sql);
    db_init_string (&strval);

    /* open output vector */
    if ( z_flag->answer )
        Vect_open_new (&Map, out_opt->answer, 1 );
    else
        Vect_open_new (&Map, out_opt->answer, 0 );

    Vect_hist_command ( &Map );

    /* Points and lines are written immediately with categories. Boundaries of polygons are
     * written to the vector then cleaned and centroids are calculated for all areas in cleaan vector.
     * Then second pass through finds all centroids in each polygon feature and adds its category
     * to the centroid. The result is that one centroids may have 0, 1 ore more categories
     * of one ore more (more input layers) fields. */
    with_z = 0;
    for ( layer = 0; layer < nlayers; layer++ ) {
	G_message (_("Layer: %s"), layer_names[layer]);
	layer_id = layers[layer];

	Ogr_layer = OGR_DS_GetLayer( Ogr_ds, layer_id );
	Ogr_featuredefn = OGR_L_GetLayerDefn( Ogr_layer );

	/* Add DB link */
	if ( !notab_flag->answer ) {
	    char *cat_col_name = "cat";

	    if ( nlayers == 1 ) { /* one layer only */
		Fi = Vect_default_field_info ( &Map, layer+1, NULL, GV_1TABLE );
	    } else {
		Fi = Vect_default_field_info ( &Map, layer+1, NULL, GV_MTABLE );
	    }

     	    if ( ncnames > 0 ) {
    		cat_col_name = cnames_opt->answers[0];
	    }
	    Vect_map_add_dblink ( &Map, layer+1, NULL, Fi->table, cat_col_name, Fi->database, Fi->driver);

	    ncols = OGR_FD_GetFieldCount( Ogr_featuredefn );
	    G_debug ( 2, "%d columns", ncols );

	    /* Create table */
	    sprintf ( buf, "create table %s (%s integer", Fi->table, cat_col_name );
	    db_set_string ( &sql, buf);
	    for ( i = 0; i < ncols; i++ ) {

		Ogr_field = OGR_FD_GetFieldDefn( Ogr_featuredefn, i );
		Ogr_ftype = OGR_Fld_GetType( Ogr_field );

		G_debug(3, "Ogr_ftype: %i", Ogr_ftype); /* look up below */

		if ( i < ncnames-1 ) {
		    Ogr_fieldname = strdup(cnames_opt->answers[i+1]);
		} else {
		    /* Change column names to [A-Za-z][A-Za-z0-9_]* */
		    Ogr_fieldname = G_strdup( OGR_Fld_GetNameRef( Ogr_field ) );
		    G_debug(3, "Ogr_fieldname: '%s'", Ogr_fieldname);

		    G_str_to_sql (Ogr_fieldname);

		    G_debug(3, "Ogr_fieldname: '%s'", Ogr_fieldname);

		}

		/* avoid that we get the 'cat' column twice */
		if ( strcmp(Ogr_fieldname, "cat") == 0 ) {
		    sprintf(namebuf, "%s_", Ogr_fieldname);
		    Ogr_fieldname = strdup ( namebuf );
		}

		/* captial column names are a pain in SQL */
		if ( tolower_flag->answer )
			G_str_to_lower(Ogr_fieldname);
		
		if ( strcmp ( OGR_Fld_GetNameRef( Ogr_field ), Ogr_fieldname) != 0 ) {
		    G_warning (_("Column name changed: '%s' -> '%s'"),
				  OGR_Fld_GetNameRef( Ogr_field ), Ogr_fieldname );
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


		if( Ogr_ftype == OFTInteger ) {
		    sprintf (buf, ", %s integer", Ogr_fieldname );
		} else if( Ogr_ftype == OFTIntegerList ) {
		    /* hack: treat as string */
		    sprintf (buf, ", %s varchar ( %d )", Ogr_fieldname, OFTIntegerListlength );
		    G_warning (_("Writing column <%s> with fixed length %d chars (may be truncated)"), Ogr_fieldname, OFTIntegerListlength);
		} else if( Ogr_ftype == OFTReal ) {
		    sprintf (buf, ", %s double precision", Ogr_fieldname );
#if GDAL_VERSION_NUM >= 1320
		} else if( Ogr_ftype == OFTDate ) {
		    sprintf (buf, ", %s date", Ogr_fieldname );
		} else if( Ogr_ftype == OFTTime ) {
		    sprintf (buf, ", %s time", Ogr_fieldname );
		} else if( Ogr_ftype == OFTDateTime ) {
		    sprintf (buf, ", %s datetime", Ogr_fieldname );
#endif
		} else if( Ogr_ftype == OFTString ) {
		    int fwidth;
		    fwidth = OGR_Fld_GetWidth(Ogr_field);
		    /* TODO: read all records first and find the longest string length */
		    if ( fwidth == 0) {
			G_warning (_("Width for column %s set to 255 (was not specified by OGR), "
				     "some strings may be truncated!"), Ogr_fieldname );
			fwidth = 255;
		    }
		    sprintf (buf, ", %s varchar ( %d )", Ogr_fieldname, fwidth );
		} else if( Ogr_ftype == OFTStringList ) {
		    /* hack: treat as string */
		    sprintf (buf, ", %s varchar ( %d )", Ogr_fieldname, OFTIntegerListlength );
		    G_warning (_("Writing column %s with fixed length %d chars (may be truncated)"), Ogr_fieldname, OFTIntegerListlength);
		} else {
		    G_warning (_("Column type not supported (%s)"), Ogr_fieldname );
		    buf[0] = 0;
		}
		db_append_string ( &sql, buf);
		G_free(Ogr_fieldname);
	    }
	    db_append_string ( &sql, ")" );
	    G_debug ( 3, db_get_string ( &sql ) );

	    driver = db_start_driver_open_database ( Fi->driver, Vect_subst_var(Fi->database,&Map) );
	    if ( driver == NULL ) {
	        G_fatal_error ( _("Cannot open database %s by driver %s"),
				Vect_subst_var(Fi->database,&Map), Fi->driver );
	    }

	    if (db_execute_immediate (driver, &sql) != DB_OK ) {
		db_close_database(driver);
		db_shutdown_driver(driver);
		G_fatal_error ( _("Cannot create table: %s"), db_get_string ( &sql )  );
	    }

	    if ( db_create_index2(driver, Fi->table, cat_col_name ) != DB_OK )
		G_warning (_("Cannot create index" ));

	    if (db_grant_on_table (driver, Fi->table, DB_PRIV_SELECT, DB_GROUP|DB_PUBLIC ) != DB_OK )
		G_fatal_error ( _("Cannot grant privileges on table %s"), Fi->table );

	    db_begin_transaction ( driver );
	}

	/* Import feature */
	cat = 1;
	nogeom = 0;
	OGR_L_ResetReading ( Ogr_layer );
	G_important_message(_("Importing map %d features..."), OGR_L_GetFeatureCount ( Ogr_layer, 1 ));
	while( (Ogr_feature = OGR_L_GetNextFeature(Ogr_layer)) != NULL ) {
	    /* Geometry */
	    Ogr_geometry = OGR_F_GetGeometryRef(Ogr_feature);
	    if ( Ogr_geometry == NULL ) {
		nogeom++;
	    } else {
	        dim = OGR_G_GetCoordinateDimension ( Ogr_geometry );
		if ( dim > 2 )
		    with_z = 1;

		geom ( Ogr_geometry, &Map, layer+1, cat, min_area, type, no_clean_flag->answer );
	    }

	    /* Attributes */
	    if ( !notab_flag->answer ) {
		sprintf ( buf, "insert into %s values ( %d", Fi->table, cat );
		db_set_string ( &sql, buf);
		for ( i = 0; i < ncols; i++ ) {
		    Ogr_field = OGR_FD_GetFieldDefn( Ogr_featuredefn, i );
		    Ogr_ftype = OGR_Fld_GetType( Ogr_field );
		    if( OGR_F_IsFieldSet( Ogr_feature, i ) ) {
			if( Ogr_ftype == OFTInteger || Ogr_ftype == OFTReal ) {
			    sprintf (buf, ", %s", OGR_F_GetFieldAsString( Ogr_feature, i) );
#if GDAL_VERSION_NUM >= 1320
			/* should we use OGR_F_GetFieldAsDateTime() here ? */
			} else if( Ogr_ftype == OFTDate || Ogr_ftype == OFTTime || Ogr_ftype == OFTDateTime ) {
			    char *newbuf;
			    db_set_string ( &strval,  (char *) OGR_F_GetFieldAsString( Ogr_feature, i) );
			    db_double_quote_string (&strval);
			    sprintf (buf, ", '%s'", db_get_string(&strval) );
			    newbuf = G_str_replace(buf, "/", "-"); /* fix 2001/10/21 to 2001-10-21 */
			    sprintf (buf, "%s", newbuf);
#endif
			} else if( Ogr_ftype == OFTString || Ogr_ftype == OFTIntegerList ) {
			    db_set_string ( &strval,  (char *) OGR_F_GetFieldAsString( Ogr_feature, i) );
			    db_double_quote_string (&strval);
			    sprintf (buf, ", '%s'", db_get_string(&strval) );
			}

		    } else {
			/* G_warning (_("Column value not set" )); */
			if( Ogr_ftype == OFTInteger || Ogr_ftype == OFTReal ) {
			    sprintf (buf, ", NULL" );
#if GDAL_VERSION_NUM >= 1320
			} else if( Ogr_ftype == OFTString || Ogr_ftype == OFTIntegerList || Ogr_ftype == OFTDate ) {
#else
			} else if( Ogr_ftype == OFTString || Ogr_ftype == OFTIntegerList ) {
#endif
			    sprintf (buf, ", ''" );
			}
		    }
		    db_append_string ( &sql, buf);
		}
		db_append_string ( &sql, " )" );
		G_debug ( 3, db_get_string ( &sql ) );

		if (db_execute_immediate (driver, &sql) != DB_OK ) {
		    db_close_database(driver);
		    db_shutdown_driver(driver);
		    G_fatal_error ( _("Cannot insert new row: %s"), db_get_string ( &sql )  );
		}
	    }

	    OGR_F_Destroy( Ogr_feature );
	    cat++;
	}

	if ( !notab_flag->answer ) {
	    db_commit_transaction ( driver );
	    db_close_database_shutdown_driver ( driver );
	}

	if ( nogeom > 0 )
	    G_warning (_("%d %s without geometry"), nogeom, nogeom == 1 ? "feature" : "features" );
    }


    separator = "-----------------------------------------------------";
    G_message ( "%s", separator );

    /* TODO: is it necessary to build here? probably not, consumes time */
    Vect_build ( &Map, output );

    if ( !no_clean_flag->answer && Vect_get_num_primitives(&Map, GV_BOUNDARY) > 0) {
	int ret, centr, ncentr, otype, n_overlaps, n_nocat;
        CENTR  *Centr;
	SPATIAL_INDEX si;
	double x, y, total_area, overlap_area, nocat_area;
	BOUND_BOX box;
	struct line_pnts *Points;
	int nmodif;

	Points = Vect_new_line_struct ();

        G_message ( "%s", separator );
	G_warning (_("Cleaning polygons, result is not guaranteed!"));

        Vect_set_release_support ( &Map );
        Vect_close ( &Map );
	Vect_open_update (&Map, out_opt->answer, G_mapset());
        Vect_build_partial ( &Map, GV_BUILD_BASE, output ); /* Downgrade topo */

	if ( snap >= 0 ) {
	    G_message ( "%s", separator );
	    G_message ( _("Snap boundaries (threshold = %.3e):"), snap );
	    Vect_snap_lines ( &Map, GV_BOUNDARY, snap, NULL, output );
	}

	/* It is not to clean to snap centroids, but I have seen data with 2 duplicate polygons
	 * (as far as decimal places were printed) and centroids were not identical */
	/* Disabled, because overlapping polygons result in many duplicate centroids anyway */
	/*
        fprintf ( stderr, separator );
	fprintf ( stderr, "Snap centroids (threshold 0.000001):\n" );
	Vect_snap_lines ( &Map, GV_CENTROID, 0.000001, NULL, stderr );
	*/

        G_message ( "%s", separator );
	G_message ( _("Break polygons:") );
	Vect_break_polygons ( &Map, GV_BOUNDARY, NULL, output );

	/* It is important to remove also duplicate centroids in case of duplicate imput polygons */
        G_message ( "%s", separator );
	G_message ( _("Remove duplicates:") );
	Vect_remove_duplicates ( &Map, GV_BOUNDARY | GV_CENTROID, NULL, output );

	/* Vect_clean_small_angles_at_nodes() can change the geometry so that new intersections
	 * are created. We must call Vect_break_lines(), Vect_remove_duplicates()
	 * and Vect_clean_small_angles_at_nodes() until no more small dangles are found */
	do {
            G_message ( "%s", separator );
	    G_message ( _("Break boundaries:") );
	    Vect_break_lines ( &Map, GV_BOUNDARY, NULL, output );

            G_message ( "%s", separator );
	    G_message ( _( "Remove duplicates:" ) );
	    Vect_remove_duplicates ( &Map, GV_BOUNDARY, NULL, output );

            G_message ( "%s", separator );
	    G_message (_( "Clean boundaries at nodes:") );
	    nmodif = Vect_clean_small_angles_at_nodes ( &Map, GV_BOUNDARY, NULL, output );
	} while ( nmodif > 0 );

        G_message ( "%s", separator );
	if ( type & GV_BOUNDARY ) { /* that means lines were converted boundaries */
	    G_message ( _("Change boundary dangles to lines:") );
	    Vect_chtype_dangles ( &Map, -1.0, NULL, output );
	} else {
	    G_message ( _("Change dangles to lines:") );
	    Vect_remove_dangles ( &Map, GV_BOUNDARY, -1.0, NULL, output );
	}

        G_message ( "%s", separator );
	if ( type & GV_BOUNDARY ) {
	    G_message(_("Change boundary bridges to lines:") );
	    Vect_chtype_bridges ( &Map, NULL, output );
	} else {
	    G_message ( _("Remove bridges:") );
	    Vect_remove_bridges ( &Map, NULL, output );
	}

	/* Boundaries are hopefully clean, build areas */
        G_message ( "%s", separator );
        Vect_build_partial ( &Map, GV_BUILD_ATTACH_ISLES, output );

	/* Calculate new centroids for all areas, centroids have the same id as area */
	ncentr = Vect_get_num_areas ( &Map );
	G_debug (3, "%d centroids/areas", ncentr);

	Centr = (CENTR *) G_calloc ( ncentr+1, sizeof(CENTR) );
	Vect_spatial_index_init ( &si );
	for ( centr = 1; centr <= ncentr; centr++ ) {
	    Centr[centr].valid = 0;
	    Centr[centr].cats = Vect_new_cats_struct ();
	    ret = Vect_get_point_in_area ( &Map, centr, &x, &y );
	    if ( ret < 0 ) {
		G_warning (_("Cannot calculate area centroid") );
		continue;
	    }

	    Centr[centr].x = x;
	    Centr[centr].y = y;
	    Centr[centr].valid = 1;
	    box.N = box.S = y;
	    box.E = box.W = x;
	    box.T = box.B = 0;
	    Vect_spatial_index_add_item (&si, centr, &box );
	}

	/* Go through all layers and find centroids for each polygon */
	for ( layer = 0; layer < nlayers; layer++ ) {
	    G_message (_("Layer: %s"), layer_names[layer]);
	    layer_id = layers[layer];
	    Ogr_layer = OGR_DS_GetLayer( Ogr_ds, layer_id );
	    OGR_L_ResetReading ( Ogr_layer );

	    cat = 0; /* field = layer + 1 */
	    while( (Ogr_feature = OGR_L_GetNextFeature(Ogr_layer)) != NULL ) {
		cat++;
		/* Geometry */
		Ogr_geometry = OGR_F_GetGeometryRef(Ogr_feature);
		if ( Ogr_geometry != NULL ) {
		    centroid ( Ogr_geometry, Centr, &si, layer+1, cat, min_area, type );
		}

		OGR_F_Destroy( Ogr_feature );
	    }
	}

	/* Write centroids */
	n_overlaps = n_nocat = 0;
	total_area = overlap_area = nocat_area = 0.0;
	for ( centr = 1; centr <= ncentr; centr++ ) {
	    double area;

	    area = Vect_get_area_area ( &Map, centr );
	    total_area += area;

	    if ( !(Centr[centr].valid) ) {
		continue;
	    }

	    if ( Centr[centr].cats->n_cats == 0 ) {
		nocat_area += area;
		n_nocat++;
		continue;
	    }

	    if ( Centr[centr].cats->n_cats > 1 ) {
	        Vect_cat_set ( Centr[centr].cats, nlayers+1, Centr[centr].cats->n_cats );
		overlap_area += area;
		n_overlaps++;
	    }

	    Vect_reset_line ( Points );
	    Vect_append_point ( Points, Centr[centr].x, Centr[centr].y, 0.0 );
	    if ( type & GV_POINT ) otype = GV_POINT; else otype = GV_CENTROID;
	    Vect_write_line ( &Map, otype, Points, Centr[centr].cats);
	}
        if (Centr)
		G_free(Centr);
        G_message ( "%s", separator );
	Vect_build_partial (&Map, GV_BUILD_NONE, NULL);

        G_message ( "%s", separator );
        Vect_build ( &Map, output );

        G_message ( "%s", separator );

	if ( n_overlaps > 0 ) {
	    G_warning (_("%d areas represent more (overlapping) features, because polygons overlap "
			 "in input layer(s). Such areas are linked to more than 1 row in attribute table. "
			 "The number of features for those areas is stored as category in layer %d"),
		       n_overlaps, nlayers+1 );
	}

	sprintf (buf, _("%d input polygons"), n_polygons);
	G_message ( buf );
	Vect_hist_write ( &Map, buf );

	sprintf (buf, _("Total area: %e (%d areas)"), total_area, ncentr);
	G_message ( buf );
	Vect_hist_write ( &Map, buf );

	sprintf (buf, _("Overlapping area: %e (%d areas)"), overlap_area, n_overlaps);
	G_message ( buf );
	Vect_hist_write ( &Map, buf );

	sprintf (buf, _("Area without category: %e (%d areas)"), nocat_area, n_nocat );
	G_message ( buf );
	Vect_hist_write ( &Map, buf );
    }

    /* needed?
    * OGR_DS_Destroy( Ogr_ds );
    */

    Vect_close ( &Map );


/* -------------------------------------------------------------------- */
/*      Extend current window based on dataset.                         */
/* -------------------------------------------------------------------- */
    if( extend_flag->answer )
    {
	G_get_default_window( &loc_wind );

	loc_wind.north = MAX(loc_wind.north,cellhd.north);
	loc_wind.south = MIN(loc_wind.south,cellhd.south);
	loc_wind.west  = MIN(loc_wind.west, cellhd.west);
	loc_wind.east  = MAX(loc_wind.east, cellhd.east);

	loc_wind.rows = (int) ceil((loc_wind.north - loc_wind.south) 
					/ loc_wind.ns_res);
	loc_wind.south = loc_wind.north - loc_wind.rows * loc_wind.ns_res;

	loc_wind.cols = (int) ceil((loc_wind.east - loc_wind.west) 
					/ loc_wind.ew_res);
	loc_wind.east = loc_wind.west + loc_wind.cols * loc_wind.ew_res;

	G__put_window(&loc_wind, "../PERMANENT", "DEFAULT_WIND");
    }

    if (with_z && !z_flag->answer )
	G_warning (_("Input data contains 3D features. Created vector is 2D only, "
		     "use -z flag to import 3D vector"));

    exit(EXIT_SUCCESS) ;
}

