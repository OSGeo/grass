
/*****************************************************************************
*
* MODULE:       DBF driver 
*   	    	
* AUTHOR(S):    Radim Blazek
*
* PURPOSE:      Simple driver for reading and writing dbf files     
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
#include <grass/dbmi.h>
#include "globals.h"
#include "proto.h"

int db__driver_fetch(dbCursor * cn, int position, int *more)
{
    cursor *c;
    dbToken token;
    dbTable *table;
    dbColumn *column;
    dbValue *value;
    int col, ncols;
    int htype, sqltype, ctype;
    int dbfrow, dbfcol;

    /* get cursor token */
    token = db_get_cursor_token(cn);

    /* get the cursor by its token */
    if (!(c = (cursor *) db_find_token(token))) {
	db_error("cursor not found");
	return DB_FAILED;
    }

    /* fetch on position */
    switch (position) {
    case DB_NEXT:
	c->cur++;
	break;
    case DB_CURRENT:
	break;
    case DB_PREVIOUS:
	c->cur--;
	break;
    case DB_FIRST:
	c->cur = 0;
	break;
    case DB_LAST:
	c->cur = c->nrows - 1;
	break;
    };

    if ((c->cur >= c->nrows) || (c->cur < 0)) {
	*more = 0;
	return DB_OK;
    }
    *more = 1;


    /* get the data out of the descriptor into the table */
    table = db_get_cursor_table(cn);
    ncols = db_get_table_number_of_columns(table);
    dbfrow = c->set[c->cur];
    for (col = 1; col <= ncols; col++) {
	dbfcol = c->cols[col - 1];
	column = db_get_table_column(table, col - 1);
	value = db_get_column_value(column);
	db_free_string(&value->s);

	sqltype = db_get_column_sqltype(column);
	ctype = db_sqltype_to_Ctype(sqltype);
	htype = db_get_column_host_type(column);

	if (db.tables[c->table].rows[dbfrow].values[dbfcol].is_null) {
	    db_set_value_null(value);
	}
	else {
	    db_set_value_not_null(value);
	    switch (ctype) {
	    case DB_C_TYPE_STRING:
		db_set_string(&(value->s),
			      db.tables[c->table].rows[dbfrow].values[dbfcol].
			      c);
		break;
	    case DB_C_TYPE_INT:
		value->i = db.tables[c->table].rows[dbfrow].values[dbfcol].i;
		break;
	    case DB_C_TYPE_DOUBLE:
		value->d = db.tables[c->table].rows[dbfrow].values[dbfcol].d;
		break;
	    }
	}
    }
    return DB_OK;
}

int db__driver_get_num_rows(dbCursor * cn)
{
    cursor *c;
    dbToken token;

    /* get cursor token */
    token = db_get_cursor_token(cn);

    /* get the cursor by its token */
    if (!(c = (cursor *) db_find_token(token))) {
	db_error("cursor not found");
	return DB_FAILED;
    }

    return (c->nrows);

}
