/*!
   \file lib/vector/Vlib/open_ogr.c

   \brief Vector library - Open OGR layer as vector map layer

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2010 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
   \author Update to GRASS 7.0 Martin Landa <landa.martin gmail.com> (2009)
 */

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

#ifdef HAVE_OGR
#include <ogr_api.h>
#include <cpl_string.h>

static int sqltype_to_ogrtype(int);
static int create_table(OGRLayerH, const struct field_info *);

/*!
   \brief Open existing OGR layer (level 1 - without feature index file)

   Map->name, Map->mapset, Map->fInfo.ogr.dsn and
   Map->fInfo.ogr.layer_name must be set before.

   \param[in,out] Map pointer to Map_info structure
   \param update TRUE for write mode, otherwise read-only
   
   \return 0 success
   \return -1 error
*/
int V1_open_old_ogr(struct Map_info *Map, int update)
{
    int i, layer, nLayers;
    OGRDataSourceH Ogr_ds;
    OGRLayerH Ogr_layer;
    OGRFeatureDefnH Ogr_featuredefn;
    OGRwkbGeometryType Ogr_geom_type;
    
    Ogr_layer = NULL;
    Ogr_geom_type = wkbUnknown;
    
    if (!Map->fInfo.ogr.dsn) {
	G_fatal_error(_("OGR datasource not defined"));
	return -1;
    }
    
    if (!Map->fInfo.ogr.layer_name) {
	G_fatal_error(_("OGR layer not defined"));
	return -1;
    }
    
    G_debug(2, "V1_open_old_ogr(): dsn = %s layer = %s", Map->fInfo.ogr.dsn,
	    Map->fInfo.ogr.layer_name);

    OGRRegisterAll();

    /* open data source handle */
    Ogr_ds = OGROpen(Map->fInfo.ogr.dsn, FALSE, NULL);
    if (Ogr_ds == NULL)
	G_fatal_error(_("Unable to open OGR data source '%s'"),
		      Map->fInfo.ogr.dsn);
    Map->fInfo.ogr.ds = Ogr_ds;

    /* get layer number */
    layer = -1;
    nLayers = OGR_DS_GetLayerCount(Ogr_ds);
    G_debug(2, "%d layers found in data source", nLayers);

    for (i = 0; i < nLayers; i++) {
	Ogr_layer = OGR_DS_GetLayer(Ogr_ds, i);
	Ogr_featuredefn = OGR_L_GetLayerDefn(Ogr_layer);
	if (strcmp(OGR_FD_GetName(Ogr_featuredefn), Map->fInfo.ogr.layer_name) == 0) {
	    Ogr_geom_type = OGR_FD_GetGeomType(Ogr_featuredefn);
	    layer = i;
	    break;
	}
    }
    if (layer == -1) {
	OGR_DS_Destroy(Ogr_ds);
	G_fatal_error(_("OGR layer <%s> not found"),
		      Map->fInfo.ogr.layer_name);
    }
    G_debug(2, "OGR layer %d opened", layer);

    Map->fInfo.ogr.layer = Ogr_layer;
    if (update && OGR_L_TestCapability(Map->fInfo.ogr.layer, OLCTransactions))
	OGR_L_StartTransaction(Map->fInfo.ogr.layer);
    
    Map->fInfo.ogr.lines = NULL;
    Map->fInfo.ogr.lines_types = NULL;
    Map->fInfo.ogr.lines_alloc = 0;
    Map->fInfo.ogr.lines_num = 0;
    Map->fInfo.ogr.lines_next = 0;
    
    switch(Ogr_geom_type) {
    case wkbPoint25D: case wkbLineString25D: case wkbPolygon25D:
    case wkbMultiPoint25D: case wkbMultiLineString25D: case wkbMultiPolygon25D:
    case wkbGeometryCollection25D:
	Map->head.with_z = WITH_Z;
	break;
    default:
	Map->head.with_z = WITHOUT_Z;
	break;
    }
    
    Map->fInfo.ogr.feature_cache = NULL;
    Map->fInfo.ogr.feature_cache_id = -1;	/* FID >= 0 */
    
    return 0;
}

/*!
   \brief Open existing OGR layer (level 2 - feature index)

   \param[in,out] Map pointer to Map_info structure
   
   \return 0 success
   \return -1 error
*/
int V2_open_old_ogr(struct Map_info *Map)
{
    char elem[GPATH_MAX];
    char buf[5];		/* used for format version */
    long length;
    struct gvfile fp;
    struct Port_info port;
    int Version_Major, Version_Minor, Back_Major, Back_Minor, byte_order;

    G_debug(3, "V2_open_old_ogr()");
    
    sprintf(elem, "%s/%s", GV_DIRECTORY, Map->name);
    dig_file_init(&fp);
    fp.file = G_fopen_old(elem, GV_FIDX_ELEMENT, Map->mapset);
    if (fp.file == NULL) {
	G_warning(_("Unable to open fidx file for vector map <%s@%s>"),
		  Map->name, Map->mapset);
	return -1;
    }

    /* Header */
    if (0 >= dig__fread_port_C(buf, 5, &fp))
	return (-1);
    Version_Major = buf[0];
    Version_Minor = buf[1];
    Back_Major = buf[2];
    Back_Minor = buf[3];
    byte_order = buf[4];
    
    /* check version numbers */
    if (Version_Major > 5 || Version_Minor > 0) {
	if (Back_Major > 5 || Back_Minor > 0) {
	    G_fatal_error(_("Feature index format version %d.%d is not supported by this release."
			   " Try to rebuild topology or upgrade GRASS."),
			  Version_Major, Version_Minor);
	    return (-1);
	}
	G_warning(_("Your GRASS version does not fully support feature index format %d.%d of the vector."
		   " Consider to rebuild topology or upgrade GRASS."),
		  Version_Major, Version_Minor);
    }

    dig_init_portable(&port, byte_order);
    dig_set_cur_port(&port);

    /* Body */
    /* bytes 6 - 9 : header size */
    if (0 >= dig__fread_port_L(&length, 1, &fp))
	return (-1);
    G_debug(3, "  header size %ld", length);

    G_fseek(fp.file, length, SEEK_SET);

    /* number of records  */
    if (0 >= dig__fread_port_I(&(Map->fInfo.ogr.offset_num), 1, &fp))
	return (-1);

    /* alloc space */
    Map->fInfo.ogr.offset =
	(int *)G_malloc(Map->fInfo.ogr.offset_num * sizeof(int));
    Map->fInfo.ogr.offset_alloc = Map->fInfo.ogr.offset_num;

    /* offsets */
    if (0 >= dig__fread_port_I(Map->fInfo.ogr.offset,
			       Map->fInfo.ogr.offset_num, &fp))
	return (-1);

    fclose(fp.file);

    G_debug(3, "%d records read from fidx", Map->fInfo.ogr.offset_num);

    Map->fInfo.ogr.next_line = 1;

    return 0;
}

/*!
   \brief Prepare OGR datasource for creating new OGR layer (level 1)

   New OGR layer can be created by V2_open_new_ogr().
   
   \param[out] Map pointer to Map_info structure
   \param name name of OGR layer to create
   \param with_z WITH_Z for 3D vector data otherwise WITHOUT_Z

   \return 0 success
   \return -1 error 
*/
int V1_open_new_ogr(struct Map_info *Map, const char *name, int with_z)
{
    OGRSFDriverH    Ogr_driver;
    OGRDataSourceH  Ogr_ds;
    OGRLayerH       Ogr_layer;
    OGRFeatureDefnH Ogr_featuredefn;
    
    int            i, nlayers;
    char         **Ogr_layer_options;
     
    Ogr_layer_options = NULL;
    
    OGRRegisterAll();
	    
    Ogr_driver = OGRGetDriverByName(Map->fInfo.ogr.driver_name);
    if (!Ogr_driver) {
	G_warning(_("Unable to get OGR driver <%s>"), Map->fInfo.ogr.driver_name);
	return -1;
    }
    Map->fInfo.ogr.driver = Ogr_driver;
    
    /* TODO: creation options */
    Ogr_ds = OGR_Dr_CreateDataSource(Ogr_driver, Map->fInfo.ogr.dsn, NULL);
    if (!Ogr_ds) {
	G_warning(_("Unable to create OGR data source '%s'"),
		  Map->fInfo.ogr.dsn);
	return -1;
    }
    Map->fInfo.ogr.ds = Ogr_ds;

    nlayers = OGR_DS_GetLayerCount(Ogr_ds);
    for (i = 0; i < nlayers; i++) {
      	Ogr_layer = OGR_DS_GetLayer(Ogr_ds, i);
	Ogr_featuredefn = OGR_L_GetLayerDefn(Ogr_layer);
	if (strcmp(OGR_FD_GetName(Ogr_featuredefn), name) == 0) {	
	    if (G_get_overwrite()) {
		G_warning(_("OGR layer <%s> already exists and will be overwritten"),
			  Map->fInfo.ogr.layer_name);
		
		if (OGR_DS_DeleteLayer(Ogr_ds, i) != OGRERR_NONE) {
		    G_warning(_("Unable to delete OGR layer <%s>"),
			      Map->fInfo.ogr.layer_name);
		    return -1;
		}
	    }
	    else {
		G_fatal_error(_("OGR layer <%s> already exists in datasource '%s'"),
			      Map->fInfo.ogr.layer_name, Map->fInfo.ogr.dsn);
	    }
	    Map->fInfo.ogr.layer = NULL;
	    break;
	}
    }
    
    return 0;
}

/*!
   \brief Create new OGR layer in given OGR datasource (level 2)

   V1_open_new_ogr() is required to be called before this function.

   List of currently supported types:
    - GV_POINT     (wkbPoint)
    - GV_LINE      (wkbLineString)
    - GV_BOUNDARY  (wkb_Polygon)
   \param[in,out] Map pointer to Map_info structure
   \param type feature type (GV_POINT, GV_LINE, ...)

   \return 0 success
   \return -1 error 
*/
int V2_open_new_ogr(struct Map_info *Map, int type)
{
    int ndblinks;
    OGRLayerH            Ogr_layer;
    OGRSpatialReferenceH Ogr_spatial_ref;
    
    struct field_info *Fi;
    struct Key_Value *projinfo, *projunits;
    
    OGRwkbGeometryType Ogr_geom_type;
    char             **Ogr_layer_options;
    
    if (!Map->fInfo.ogr.driver_name ||
	!Map->fInfo.ogr.layer_name ||
	!Map->fInfo.ogr.ds)
	return -1;
    
    /* get spatial reference */
    projinfo  = G_get_projinfo();
    projunits = G_get_projunits();
    Ogr_spatial_ref = GPJ_grass_to_osr(projinfo, projunits);

    switch(type) {
    case GV_POINT:
	Ogr_geom_type = wkbPoint;
	break;
    case GV_LINE:
	Ogr_geom_type = wkbLineString;
	break;
    case GV_BOUNDARY:
	Ogr_geom_type = wkbPolygon;
	break;
    default:
	G_warning(_("Unsupported geometry type (%d)"), type);
	return -1;
    }
    
    Ogr_layer_options = Map->fInfo.ogr.layer_options;
    if (Vect_is_3d(Map)) {
	if (strcmp(Map->fInfo.ogr.driver_name, "PostgreSQL") == 0) {
	    Ogr_layer_options = CSLSetNameValue(Ogr_layer_options, "DIM", "3");
	}
    }
    else {
	if (strcmp(Map->fInfo.ogr.driver_name, "PostgreSQL") == 0) {
	    Ogr_layer_options = CSLSetNameValue(Ogr_layer_options, "DIM", "2");
	}
    }
    Ogr_layer = OGR_DS_CreateLayer(Map->fInfo.ogr.ds, Map->fInfo.ogr.layer_name,
				   Ogr_spatial_ref, Ogr_geom_type, Ogr_layer_options);
    CSLDestroy(Ogr_layer_options);
    if (!Ogr_layer) {
	G_warning(_("Unable to create OGR layer <%s> in '%s'"),
		  Map->fInfo.ogr.layer_name, Map->fInfo.ogr.dsn);
	return -1;
    }
    Map->fInfo.ogr.layer = Ogr_layer;

    ndblinks = Vect_get_num_dblinks(Map);
    if (ndblinks > 0) {
	/* write also attributes */
	Fi = Vect_get_dblink(Map, 0);
	if (Fi) {
	    if (ndblinks > 1)
		G_warning(_("More layers defined, using driver <%s> and "
			    "database <%s>"), Fi->driver, Fi->database);
	    create_table(Map->fInfo.ogr.layer, Fi);
	    G_free(Fi);
	}
	else
	G_warning(_("Database connection not defined. "
		    "Unable to write attributes."));
    }
    
    if (OGR_L_TestCapability(Map->fInfo.ogr.layer, OLCTransactions))
	OGR_L_StartTransaction(Map->fInfo.ogr.layer);

    return 0;
}

int create_table(OGRLayerH hLayer, const struct field_info *Fi)
{
    int col, ncols;
    int sqltype, ogrtype, length;
    
    const char *colname;
    
    dbDriver *driver;
    dbHandle handle;
    dbCursor cursor;
    dbTable *table;
    dbColumn *column;
    dbString sql;
    
    OGRFieldDefnH hFieldDefn;
    OGRFeatureDefnH hFeatureDefn;

    db_init_string(&sql);
    db_init_handle(&handle);
    
    driver = db_start_driver(Fi->driver);
    if (!driver) {
	G_warning(_("Unable to start driver <%s>"), Fi->driver);
	return -1;
    }
    db_set_handle(&handle, Fi->database, NULL);
    if (db_open_database(driver, &handle) != DB_OK) {
	G_warning(_("Unable to open database <%s> by driver <%s>"),
		  Fi->database, Fi->driver);
	db_close_database_shutdown_driver(driver);
	return -1;
    }
 
    /* to get no data */
    db_set_string(&sql, "select * from ");
    db_append_string(&sql, Fi->table);
    db_append_string(&sql, " where 0 = 1");	
    
    if (db_open_select_cursor(driver, &sql, &cursor, DB_SEQUENTIAL) !=
	DB_OK) {
	G_warning(_("Unable to open select cursor: '%s'"),
		  db_get_string(&sql));
	db_close_database_shutdown_driver(driver);
	return -1;
    }

    table = db_get_cursor_table(&cursor);
    ncols = db_get_table_number_of_columns(table);

    hFeatureDefn = OGR_L_GetLayerDefn(hLayer);
    
    for (col = 0; col < ncols; col++) {
	column = db_get_table_column(table, col);
	colname = db_get_column_name(column);	
	sqltype = db_get_column_sqltype(column);
	ogrtype = sqltype_to_ogrtype(sqltype);
	length = db_get_column_length(column);
	
	if (strcmp(OGR_L_GetFIDColumn(hLayer), colname) == 0 ||
	    OGR_FD_GetFieldIndex(hFeatureDefn, colname) > -1) {
	    /* field already exists */
	    continue;
	}

	hFieldDefn = OGR_Fld_Create(colname, ogrtype);
	/* GDAL 1.9.0 (r22968) uses VARCHAR instead of CHAR */
	if (ogrtype == OFTString && length > 0)
	    OGR_Fld_SetWidth(hFieldDefn, length);
	if (OGR_L_CreateField(hLayer, hFieldDefn, TRUE) != OGRERR_NONE) {
	    G_warning(_("Creating field <%s> failed"), colname);
	    db_close_database_shutdown_driver(driver);
	    return -1;
	}
	
	OGR_Fld_Destroy(hFieldDefn);
    }

    db_close_database_shutdown_driver(driver);

    return 0;
}

int sqltype_to_ogrtype(int sqltype)
{
    int ctype, ogrtype;

    ctype = db_sqltype_to_Ctype(sqltype);
    
    switch(ctype) {
    case DB_C_TYPE_INT:
	ogrtype = OFTInteger;
	break;
    case DB_C_TYPE_DOUBLE:
	ogrtype = OFTReal;
	break;
    case DB_C_TYPE_STRING:
	ogrtype = OFTString;
	break;
    case DB_C_TYPE_DATETIME:
	ogrtype = OFTString;
	break;
    default:
	ogrtype = OFTString;
	break;
    }
    
    return ogrtype;
}

#endif
