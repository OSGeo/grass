/*!
  \file db/drivers/describe.c
  
  \brief Low level OGR SQL driver
 
  (C) 2004-2009 by the GRASS Development Team
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Radim Blazek
  \author Some updates by Martin Landa <landa.martin gmail.com>
*/

#include <grass/gis.h>
#include <grass/datetime.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include <ogr_api.h>

#include "globals.h"
#include "proto.h"

/*!
  \brief Describe table using driver

  \param table_name table name (as dbString)
  \param table[out] pointer to dbTable

  \return DB_OK on success
  \return DB_FAILED on failure
*/
int db__driver_describe_table(dbString * table_name, dbTable ** table)
{
    int i, nlayers;
    OGRLayerH hLayer = NULL;
    OGRFeatureDefnH hFeatureDefn;

    /* get number of OGR layers in the datasource */
    nlayers = OGR_DS_GetLayerCount(hDs);

    /* find OGR layer */
    for (i = 0; i < nlayers; i++) {
	hLayer = OGR_DS_GetLayer(hDs, i);
	hFeatureDefn = OGR_L_GetLayerDefn(hLayer);
	if (G_strcasecmp
	    ((char *)OGR_FD_GetName(hFeatureDefn),
	     db_get_string(table_name)) == 0) {
	    break;
	}
	hLayer = NULL;
    }

    if (hLayer == NULL) {
	db_d_append_error(_("OGR layer <%s> does not exist\n"),
			  db_get_string(table_name));
	db_d_report_error();
	return DB_FAILED;
    }

    G_debug(3, "->>");
    if (describe_table(hLayer, table, NULL) == DB_FAILED) {
	db_d_append_error(_("Unable to describe table\n"));
	db_d_report_error();
	return DB_FAILED;
    }

    return DB_OK;
}

/*!
  \brief Describe table

  If c is not NULL cur->cols and cur->ncols is also set 
  cursor may be null

  \param hLayer OGR layer
  \param[put] table pointer to dbTable
  \param c pointer to cursor
 */
int describe_table(OGRLayerH hLayer, dbTable **table, cursor *c)
{
    int i, ncols, kcols, col;
    dbColumn *column;
    OGRFeatureDefnH hFeatureDefn;
    OGRFieldDefnH hFieldDefn;
    const char *fieldName, *fidcol;
    int ogrType;
    int *cols;

    G_debug(1, "describe_table()");

    hFeatureDefn = OGR_L_GetLayerDefn(hLayer);
    ncols = OGR_FD_GetFieldCount(hFeatureDefn);
    G_debug(2, "   ncols = %d (without fid column)", ncols);
    
    /* for some formats fid column is not defined, e.g. ESRI Shapefile */
    fidcol = OGR_L_GetFIDColumn(hLayer); 
    G_debug(2, "   fidcol = %s", fidcol);
    
    /* identify known columns */
    cols = (int *) G_malloc(ncols * sizeof(int));
    kcols = 0;
    for(i = 0; i < ncols; i++) {
	hFieldDefn = OGR_FD_GetFieldDefn(hFeatureDefn, i);
	ogrType = OGR_Fld_GetType(hFieldDefn);
	fieldName = OGR_Fld_GetNameRef(hFieldDefn);

	if (ogrType != OFTInteger &&
#if GDAL_VERSION_NUM >= 2000000
            ogrType != OFTInteger64 &&
#endif
            ogrType != OFTReal &&
	    ogrType != OFTString  && ogrType != OFTDate &&
	    ogrType != OFTTime    && ogrType != OFTDateTime ) {
	    G_warning(_("OGR driver: column '%s', OGR type %d is not supported"),
		      fieldName, ogrType);
	    cols[i] = 0;
	}
	else {
	    cols[i] = 1;
	    kcols++;
	}
    }
    
    if (*fidcol)
	kcols++;
    G_debug(2, "   kcols = %d (including fid column)", kcols);
    
    /* allocate dbTable */
    if (!(*table = db_alloc_table(kcols))) {
	return DB_FAILED;
    }
    
    /* set the table name */
    /* TODO */
    db_set_table_name(*table, "");
    
    /* set the table description */
    db_set_table_description(*table, "");

    /* TODO */
    /*
       db_set_table_delete_priv_granted (*table);
       db_set_table_insert_priv_granted (*table);
       db_set_table_delete_priv_not_granted (*table);
       db_set_table_insert_priv_not_granted (*table);
    */

    if (*fidcol) {
	column = db_get_table_column(*table, 0);
	db_set_column_host_type(column, OFTInteger);
	db_set_column_sqltype(column, DB_SQL_TYPE_INTEGER);
	db_set_column_name(column, fidcol);
	db_set_column_length(column, 11); /* ??? */
	db_set_column_precision(column, 0);
	
	col = 1;
    }
    else {
	col = 0;
    }

    for (i = 0; i < ncols; i++, col++) {
	int sqlType;
	int size, precision, scale;

	hFieldDefn = OGR_FD_GetFieldDefn(hFeatureDefn, i);
	ogrType = OGR_Fld_GetType(hFieldDefn);
	fieldName = OGR_Fld_GetNameRef(hFieldDefn);
	
	if (!(cols[i])) {
	    continue;		/* unknown type */
	}

	switch (ogrType) {
	case OFTInteger:
#if GDAL_VERSION_NUM >= 2000000
        case OFTInteger64:
#endif
	    sqlType = DB_SQL_TYPE_INTEGER;
	    size = OGR_Fld_GetWidth(hFieldDefn);	/* OK ? */
	    precision = 0;
#if GDAL_VERSION_NUM >= 2000000
            if (ogrType == OFTInteger64)
                G_warning(_("Column '%s' : type int8 (bigint) is stored as integer (4 bytes) "
                            "some data may be damaged"), fieldName);
#endif
	    break;

	case OFTReal:
	    sqlType = DB_SQL_TYPE_DOUBLE_PRECISION;
	    size = OGR_Fld_GetWidth(hFieldDefn);	/* OK ? */
	    precision = OGR_Fld_GetPrecision(hFieldDefn);
	    break;

	case OFTString:
	case OFTDate:
	case OFTTime:
	case OFTDateTime:
	    size = OGR_Fld_GetWidth(hFieldDefn);
	    if (size > 0) {
		sqlType = DB_SQL_TYPE_CHARACTER;
	    }
	    else {
		sqlType = DB_SQL_TYPE_TEXT;
	    }
	    precision = 0;
	    break;

	default:
	    G_warning(_("Unknown type"));
	    break;
	}
	
	G_debug(3, "   %d: field %d : ogrType = %d, name = %s, size=%d precision=%d",
		i, col, ogrType, fieldName, size, precision);
	
	column = db_get_table_column(*table, col);

	db_set_column_host_type(column, ogrType);
	db_set_column_sqltype(column, sqlType);
	db_set_column_name(column, fieldName);
	db_set_column_length(column, size);
	db_set_column_precision(column, precision);

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

    if (c) {
	c->cols = cols;
	c->ncols = ncols;
    }
    else {
	G_free(cols);
    }

    return DB_OK;
}
