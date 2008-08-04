
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
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include "ogr_api.h"
#include "globals.h"
#include "proto.h"
#include <grass/glocale.h>

int db__driver_fetch(dbCursor * cn, int position, int *more)
{
    cursor *c;
    dbToken token;
    dbTable *table;
    int i, col;

    G_debug(3, "db_driver_fetch()");

    /* get cursor token */
    token = db_get_cursor_token(cn);

    /* get the cursor by its token */
    if (!(c = (cursor *) db_find_token(token))) {
	append_error("Cursor not found");
	report_error();
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
	append_error("DB_PREVIOUS not supported");
	report_error();
	return DB_FAILED;
	break;
    case DB_FIRST:
	OGR_L_ResetReading(c->hLayer);
	if (c->hFeature)
	    OGR_F_Destroy(c->hFeature);
	c->hFeature = OGR_L_GetNextFeature(c->hLayer);
	break;
    case DB_LAST:
	append_error("DB_LAST not supported");
	report_error();
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

    col = -1;
    for (i = 0; i < c->ncols; i++) {
	int ogrType, sqlType;
	dbColumn *column;
	dbValue *value;

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

	case OFTReal:
	    value->d = OGR_F_GetFieldAsDouble(c->hFeature, i);
	    break;

	case OFTString:
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

int db__driver_get_num_rows(dbCursor * cn)
{
    cursor *c;
    dbToken token;

    G_debug(3, "db_driver_get_num_rows()");

    /* get cursor token */
    token = db_get_cursor_token(cn);

    /* get the cursor by its token */
    if (!(c = (cursor *) db_find_token(token))) {
	append_error("Cursor not found");
	report_error();
	return DB_FAILED;
    }

    return (OGR_L_GetFeatureCount(c->hLayer, 1));
}
