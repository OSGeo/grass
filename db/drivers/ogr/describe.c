/*****************************************************************************
*
* MODULE:       OGR driver 
*   	    	
* AUTHOR(S):    Radim Blazek
*
* PURPOSE:      DB driver for OGR sources     
*
* COPYRIGHT:    (C) 2004 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
#include <grass/dbmi.h>
#include <grass/datetime.h>
#include <grass/gis.h>
#include "ogr_api.h"
#include "globals.h"
#include "proto.h"
#include <grass/glocale.h>

int db__driver_describe_table( dbString *table_name, dbTable **table )
{
    int i, nlayers;
    OGRLayerH hLayer = NULL;
    OGRFeatureDefnH hFeatureDefn;

    /* Find data source */
    nlayers = OGR_DS_GetLayerCount( hDs );

    for (i = 0; i < nlayers; i++) {
	hLayer =  OGR_DS_GetLayer( hDs, i );
	hFeatureDefn = OGR_L_GetLayerDefn ( hLayer );
	if ( G_strcasecmp ( (char *) OGR_FD_GetName( hFeatureDefn ), db_get_string(table_name) ) == 0) {
	    break;
	}
	hLayer = NULL;
    }

    if (  hLayer == NULL ) {
	append_error("Table '%s' does not exist\n", db_get_string(table_name) );
	report_error();
	return DB_FAILED;
    }
    
	G_debug (3, "->>");
    if ( describe_table( hLayer, table, NULL) == DB_FAILED ) {
	append_error("Cannot describe table\n");
	report_error();
	return DB_FAILED;
    }

    return DB_OK;
}

/* describe table, if c is not NULL cur->cols and cur->ncols is also set 
 * cursor may be null
 */
int describe_table( OGRLayerH hLayer, dbTable **table, cursor *c)
{
    int      i, ncols, kcols;
    dbColumn *column;
    OGRFeatureDefnH hFeatureDefn;
    OGRFieldDefnH hFieldDefn;
    const char *fieldName;
    int   ogrType;
    int   *cols;

    G_debug (3, "describe_table()");

    hFeatureDefn = OGR_L_GetLayerDefn ( hLayer );
    ncols = OGR_FD_GetFieldCount ( hFeatureDefn );
    
    G_debug (3, "ncols = %d", ncols);

    /* Identify known columns */
    cols = (int *) G_malloc ( ncols * sizeof(int) );
    kcols = 0;
    for (i = 0; i < ncols; i++) {
	hFieldDefn = OGR_FD_GetFieldDefn( hFeatureDefn, i );
	ogrType = OGR_Fld_GetType( hFieldDefn );
	OGR_Fld_GetNameRef( hFieldDefn );
	fieldName = OGR_Fld_GetNameRef( hFieldDefn );

	if ( ogrType != OFTInteger && ogrType != OFTReal && ogrType != OFTString ) {
	    G_warning ( _("OGR driver: column '%s', OGR type %d  is not supported"), fieldName, ogrType);
	    cols[i] = 0;
	} else { 
	    cols[i] = 1;
	    kcols++;
	}
    }
    
    if (!(*table = db_alloc_table(kcols))) {
	return DB_FAILED;
    }

    /* set the table name */
    /* TODO */
    db_set_table_name(*table, "" );

    /* set the table description */
    db_set_table_description(*table, "");

    /* TODO */
    /*
    db_set_table_delete_priv_granted (*table);
    db_set_table_insert_priv_granted (*table);
    db_set_table_delete_priv_not_granted (*table);
    db_set_table_insert_priv_not_granted (*table);
    */

    for (i = 0; i < ncols; i++) {
	int   sqlType;
	int   size, precision, scale;

	if ( !(cols[i]) ) continue; /* unknown type */
	
	hFieldDefn = OGR_FD_GetFieldDefn( hFeatureDefn, i );
	ogrType = OGR_Fld_GetType( hFieldDefn );
	fieldName = OGR_Fld_GetNameRef( hFieldDefn );

	G_debug (3, "field %d : ogrType = %d, name = %s", i, ogrType, fieldName);
	
	switch ( ogrType ) {
	    case OFTInteger:
		sqlType = DB_SQL_TYPE_INTEGER;
		size = OGR_Fld_GetWidth (hFieldDefn); /* OK ? */
	        precision = 0;
		break;
		
	    case OFTReal:
		sqlType = DB_SQL_TYPE_DOUBLE_PRECISION;
		size = OGR_Fld_GetWidth (hFieldDefn); /* OK ? */
	        precision = OGR_Fld_GetPrecision (hFieldDefn);
		break;
		
	    case OFTString:
		sqlType = DB_SQL_TYPE_CHARACTER;
		size = OGR_Fld_GetWidth (hFieldDefn);
		if ( size == 0 ) {
		   G_warning ( _("column '%s', type 'string': unknown width -> stored as varchar(250) "
			         "some data may be lost"), fieldName );
		   size = 250;
		}
	        precision = 0;
		break;
		
	    default:
		G_warning ( _("Unknown type") );
		break;
	}

	column = db_get_table_column (*table, i);

	db_set_column_host_type(column, ogrType);
	db_set_column_sqltype(column, sqlType);
	db_set_column_name(column, fieldName);
	db_set_column_length(column, size);
        db_set_column_precision (column, precision);

        /* TODO */
        scale = 0;  
        /*
 	db_set_column_scale (column, scale);
 	*/

	/* TODO */
	db_set_column_null_allowed(column);
	db_set_column_has_undefined_default_value(column);
	db_unset_column_use_default_value(column);

	/* TODO */
	/*
        db_set_column_select_priv_granted (column);
        db_set_column_update_priv_granted (column);
        db_set_column_update_priv_not_granted (column); 
	*/
    }

    if ( c ) {
	c->cols = cols;
        c->ncols = ncols;
    } else {
	G_free ( cols );
    }

    return DB_OK;
}
