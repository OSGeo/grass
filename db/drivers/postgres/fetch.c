/*!
  \file db/driver/postgres/fetch.c
  
  \brief DBMI - Low Level PostgreSQL database driver - fetch data

  \todo implement time zone handling
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Radim Blazek
 */

#include <stdlib.h>
#include <string.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "globals.h"
#include "proto.h"

int db__driver_fetch(dbCursor * cn, int position, int *more)
{
    cursor *c;
    dbToken token;
    dbTable *table;
    int i;

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
	c->row++;
	break;
    case DB_CURRENT:
	break;
    case DB_PREVIOUS:
	c->row--;
	break;
    case DB_FIRST:
	c->row = 0;
	break;
    case DB_LAST:
	c->row = c->nrows - 1;
	break;
    };

    G_debug(3, "row = %d nrows = %d", c->row, c->nrows);
    if (c->row < 0 || c->row >= c->nrows) {
	*more = 0;
	return DB_OK;
    }

    *more = 1;

    /* get the data out of the descriptor into the table */
    table = db_get_cursor_table(cn);

    for (i = 0; i < c->ncols; i++) {
	int col, gpgtype, sqltype;
	dbColumn *column;
	dbValue *value;

	col = c->cols[i];	/* known cols */

	column = db_get_table_column(table, i);
	gpgtype = db_get_column_host_type(column);
	sqltype = db_get_column_sqltype(column);

	value = db_get_column_value(column);
	db_zero_string(&value->s);

	/* Is null? */
	if (PQgetisnull(c->res, c->row, col)) {
	    value->isNull = 1;
	    continue;
	}
	else {
	    value->isNull = 0;
	}

	G_debug(3, "row %d, col %d, gpgtype %d, sqltype %d: val = '%s'",
		c->row, col, gpgtype, sqltype, PQgetvalue(c->res, c->row,
							  col));

	switch (gpgtype) {
	    int ns, tz;

	case PG_TYPE_CHAR:
	case PG_TYPE_BPCHAR:
	case PG_TYPE_VARCHAR:
	case PG_TYPE_TEXT:
	    db_set_string(&(value->s), PQgetvalue(c->res, c->row, col));
	    break;

	case PG_TYPE_BIT:
	case PG_TYPE_INT2:
	case PG_TYPE_INT4:
	case PG_TYPE_INT8:
	case PG_TYPE_SERIAL:
	case PG_TYPE_OID:
	    value->i = atoi(PQgetvalue(c->res, c->row, col));
	    break;

	case PG_TYPE_FLOAT4:
	case PG_TYPE_FLOAT8:
	case PG_TYPE_NUMERIC:
	    value->d = atof(PQgetvalue(c->res, c->row, col));
	    break;

	    /* Note: we have set DATESTYLE TO ISO in db_driver_open_select_cursor() so datetime
	     *       format should be ISO */

	case PG_TYPE_DATE:
	    /* Example: '1999-01-25' */
	    ns = sscanf(PQgetvalue(c->res, c->row, col), "%4d-%2d-%2d",
			&(value->t.year), &(value->t.month), &(value->t.day));

	    if (ns != 3) {
		db_d_append_error("%s %s",
				  _("Unable to scan date:"),
				  PQgetvalue(c->res, c->row, col));
		db_d_report_error();
		return DB_FAILED;
	    }
	    value->t.hour = 0;
	    value->t.minute = 0;
	    value->t.seconds = 0.0;
	    break;

	case PG_TYPE_TIME:
	    /* Example: '04:05:06.25', '04:05:06' */
	    ns = sscanf(PQgetvalue(c->res, c->row, col), "%2d:%2d:%lf",
			&(value->t.hour), &(value->t.minute),
			&(value->t.seconds));

	    if (ns != 3) {
		db_d_append_error("%s %s",
				  _("Unable to scan time:"),
				  PQgetvalue(c->res, c->row, col));
		db_d_report_error();
		return DB_FAILED;
	    }
	    value->t.year = 0;
	    value->t.month = 0;
	    value->t.day = 0;
	    break;

	case PG_TYPE_TIMESTAMP:
	    /* Example: '1999-01-25 04:05:06.25+01', '1999-01-25 04:05:06' */
	    ns = sscanf(PQgetvalue(c->res, c->row, col),
			"%4d-%2d-%2d %2d:%2d:%lf%3d", &(value->t.year),
			&(value->t.month), &(value->t.day), &(value->t.hour),
			&(value->t.minute), &(value->t.seconds), &tz);

	    if (ns == 7) {
		db_d_append_error("%s %s",
				  _("Unable to scan timestamp "
				    "(no idea how to process time zone):"),
				  PQgetvalue(c->res, c->row, col));
		db_d_report_error();
		return DB_FAILED;
	    }
	    else if (ns < 6) {
		db_d_append_error("%s %s",
				  _("Unable to scan timestamp "
				    "(not enough arguments):"),
				  PQgetvalue(c->res, c->row, col));
		db_d_report_error();
		return DB_FAILED;
	    }
	    break;

	case PG_TYPE_BOOL:
	    if (strcmp(PQgetvalue(c->res, c->row, col), "t") == 0)
		db_set_string(&(value->s), "1");
	    else if (strcmp(PQgetvalue(c->res, c->row, col), "f") == 0)
		db_set_string(&(value->s), "0");
	    else
		G_warning(_("Unable to recognize boolean value"));
	    break;
	}
    }
    G_debug(3, "Row fetched");
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
	db_d_append_error(_("Taken not found"));
	db_d_report_error();
	return DB_FAILED;
    }

    return (c->nrows);
}
