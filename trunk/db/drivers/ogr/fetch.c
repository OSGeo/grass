/*!
  \file db/drivers/fetch.c
  
  \brief Low level OGR SQL driver
 
  (C) 2004-2009 by the GRASS Development Team
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Radim Blazek
  \author Some updates by Martin Landa <landa.martin gmail.com>
*/

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include <ogr_api.h>

#include "globals.h"
#include "proto.h"

/*!
  \brief Fetch record

  \param cn pointer to dbCursor
  \param position position indicator (DB_NEXT, DB_FIRST, DB_LAST, etc)
  \param[out] more 0 for no record fetched otherwise 1

  \return DB_OK on success
  \return DB_FAILED on error
*/
int db__driver_fetch(dbCursor * cn, int position, int *more)
{
    int i, col;
    int ogrType, sqlType;

    dbToken token;
    dbTable *table;
    dbColumn *column;
    dbValue *value;
    
    cursor *c;
    
    G_debug(3, "db_driver_fetch()");

    /* get cursor token */
    token = db_get_cursor_token(cn);

    /* get the cursor by its token */
    if (!(c = (cursor *) db_find_token(token))) {
	db_d_append_error(_("Cursor not found"));
	db_d_report_error();
	return DB_FAILED;
    }

    /* fetch on position */
    switch (position) {
    case DB_NEXT:
	G_debug(4, "DB_NEXT:");
	if (c->hFeature)
	    OGR_F_Destroy(c->hFeature);
	c->hFeature = OGR_L_GetNextFeature(c->hLayer);
	break;
    case DB_CURRENT:
	break;
    case DB_PREVIOUS:
	db_d_append_error(_("DB_PREVIOUS not supported"));
	db_d_report_error();
	return DB_FAILED;
	break;
    case DB_FIRST:
	OGR_L_ResetReading(c->hLayer);
	if (c->hFeature)
	    OGR_F_Destroy(c->hFeature);
	c->hFeature = OGR_L_GetNextFeature(c->hLayer);
	break;
    case DB_LAST:
	db_d_append_error(_("DB_LAST not supported"));
	db_d_report_error();
	return DB_FAILED;
	break;
    };

    if (c->hFeature == NULL) {
	*more = 0;
	return DB_OK;
    }

    *more = 1;

    /* get the data out of the descriptor into the table */
    table = db_get_cursor_table(cn);

    /* check fid column */
    if (strlen(OGR_L_GetFIDColumn(c->hLayer)) > 0) {
	column = db_get_table_column(table, 0);
	ogrType = db_get_column_host_type(column);
	sqlType = db_get_column_sqltype(column);

	value = db_get_column_value(column);
	value->i = OGR_F_GetFID(c->hFeature);
	G_debug(3, "fidcol '%s': ogrType %d, sqlType %d: val = %d",
		db_get_column_name(column), ogrType, sqlType, value->i);

	col = 0;
    }
    else {
	col = -1;
    }
    
    /* loop attributes */
    for (i = 0; i < c->ncols; i++) {
	if (!(c->cols[i])) {
	    continue;
	}			/* unknown type */
	col++;

	column = db_get_table_column(table, col);
	ogrType = db_get_column_host_type(column);
	sqlType = db_get_column_sqltype(column);

	value = db_get_column_value(column);
	db_zero_string(&value->s);

	/* Is null? */
	if (OGR_F_IsFieldSet(c->hFeature, i)) {
	    value->isNull = 0;
	}
	else {
	    value->isNull = 1;
	    continue;
	}

	G_debug(3, "col %d, ogrType %d, sqlType %d: val = '%s'",
		col, ogrType, sqlType, OGR_F_GetFieldAsString(c->hFeature,
							      i));

	switch (ogrType) {
	case OFTInteger:
	    value->i = OGR_F_GetFieldAsInteger(c->hFeature, i);
	    break;

#if GDAL_VERSION_NUM >= 2000000
        case OFTInteger64:
	    /* test for integer overflow ? */
	    value->i = OGR_F_GetFieldAsInteger64(c->hFeature, i);
	    break;
#endif

	case OFTReal:
	    value->d = OGR_F_GetFieldAsDouble(c->hFeature, i);
	    break;

	case OFTString:
	case OFTDate:
	case OFTTime:
	case OFTDateTime:
	    db_set_string(&(value->s),
			  (char *)OGR_F_GetFieldAsString(c->hFeature, i));
	    break;

	default:
	    G_warning(_("Unknown type"));
	    break;
	}
    }
    G_debug(4, "Row fetched");
    return DB_OK;
}

/*
  \brief Get number of rows (i.e. features) in open cursor

  \param cn pointer to dbCursor

  \return number of rows
  \return DB_FAILED on error
*/
int db__driver_get_num_rows(dbCursor * cn)
{
    cursor *c;
    dbToken token;

    G_debug(3, "db_driver_get_num_rows()");

    /* get cursor token */
    token = db_get_cursor_token(cn);

    /* get the cursor by its token */
    if (!(c = (cursor *) db_find_token(token))) {
	db_d_append_error(_("Cursor not found"));
	db_d_report_error();
	return DB_FAILED;
    }

    return (OGR_L_GetFeatureCount(c->hLayer, 1));
}
