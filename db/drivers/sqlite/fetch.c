
/**
 * \file fetch.c
 *
 * \brief Low level SQLite database functions.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Radim Blazek
 * \author Support for multiple connections by Markus Metz
 *
 * \date 2005-2011
 */

#include <stdlib.h>
#include <string.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "globals.h"
#include "proto.h"


/**
 * \fn int db__driver_fetch (dbCursor *cn, int position, int *more)
 *
 * \brief Low level SQLite database table record fetch.
 *
 * NOTE: <b>position</b> is one of:
 * DB_NEXT, DB_FIRST, DB_CURRENT, DB_PREVIOUS, DB_LAST.
 *
 * \param[in] cn open database cursor
 * \param[in] position database position. See NOTE.
 * \param[in,out] more 1 = more data; 0 = no more data
 * \return int
 */

int db__driver_fetch(dbCursor * cn, int position, int *more)
{
    cursor *c;
    dbToken token;
    dbTable *table;
    int i, ret;
    int ns;

    /* get cursor token */
    token = db_get_cursor_token(cn);

    /* get the cursor by its token */
    if (!(c = (cursor *) db_find_token(token))) {
	db_d_append_error(("Cursor not found"));
	db_d_report_error();
	return DB_FAILED;
    }

    G_debug(3, "fetch row = %d", c->row);

    /* fetch on position */
    switch (position) {
    case DB_NEXT:
    case DB_FIRST:

	if (position == DB_FIRST)
	    c->row = -1;

	ret = sqlite3_step(c->statement);
	if (ret != SQLITE_ROW) {
	    /* get real result code */
	    ret = sqlite3_reset(c->statement);
	    if (ret != SQLITE_OK) {
		db_d_append_error("%s\n%s",
				  _("Unable to fetch:"),
				  (char *)sqlite3_errmsg(sqlite));
		db_d_report_error();
		return DB_FAILED;
	    }
	    *more = 0;
	    return DB_OK;
	}
	c->row++;
	break;

    case DB_CURRENT:
	break;

    case DB_PREVIOUS:
	db_d_append_error(_("DB_PREVIOUS is not supported"));
	db_d_report_error();
	return DB_FAILED;
	break;

    case DB_LAST:
	db_d_append_error(_("DB_LAST is not supported"));
	db_d_report_error();
	return DB_FAILED;
	break;
    };

    *more = 1;

    /* get the data out of the descriptor into the table */
    table = db_get_cursor_table(cn);

    for (i = 0; i < c->nkcols; i++) {
	int col, litetype, sqltype;
	dbColumn *column;
	dbValue *value;
	const char *text;
	dbDateTime *dt;

	col = c->kcols[i];	/* known cols */

	column = db_get_table_column(table, i);
	sqltype = db_get_column_sqltype(column);
	/*      fails for dates: 
	   litetype  = db_get_column_host_type(column); 
	 */
	litetype = sqlite3_column_type(c->statement, col);
	text = (const char *)sqlite3_column_text(c->statement, col);

	value = db_get_column_value(column);
	db_zero_string(&value->s);

	/* Is null? */
	if (sqlite3_column_type(c->statement, col) == SQLITE_NULL) {
	    value->isNull = 1;
	    continue;
	}
	else {
	    value->isNull = 0;
	}

	G_debug(3, "col %d, litetype %d, sqltype %d: val = '%s'",
		col, litetype, sqltype, text);

	/* http://www.sqlite.org/capi3ref.html#sqlite3_column_type
	   SQLITE_INTEGER  1
	   SQLITE_FLOAT    2
	   SQLITE_TEXT     3
	   SQLITE_BLOB     4
	   SQLITE_NULL     5

	   lib/db/dbmi_base/sqltype.c defines:
	   DB_SQL_TYPE_*
	 */

	/* Note: we have set DATESTYLE TO ISO in db_driver_open_select_cursor() so datetime
	 *       format should be ISO */

	switch (sqltype) {
	case DB_SQL_TYPE_INTEGER:
	case DB_SQL_TYPE_SMALLINT:
	case DB_SQL_TYPE_SERIAL:
	    value->i = sqlite3_column_int(c->statement, col);
	    break;

	case DB_SQL_TYPE_REAL:
	case DB_SQL_TYPE_DOUBLE_PRECISION:
	    value->d = sqlite3_column_double(c->statement, col);
	    break;

	case DB_SQL_TYPE_DATE:
	    dt = &value->t;
	    dt->hour = 0;
	    dt->minute = 0;
	    dt->seconds = 0.0;
	    G_debug(3, "sqlite fetched date: <%s>", text);
	    ns = sscanf(text, "%4d-%2d-%2d", &dt->year, &dt->month, &dt->day);
	    if (ns != 3) {
		db_d_append_error("%s %s",
				  _("Unable to scan date:"),
				  text);
		db_d_report_error();
		return DB_FAILED;
	    }
	    break;

	case DB_SQL_TYPE_TIME:
	    dt = &value->t;
	    dt->year = 0;
	    dt->month = 0;
	    dt->day = 0;
	    G_debug(3, "sqlite fetched date: %s", text);
	    ns = sscanf(text, "%2d:%2d:%lf",
			&dt->hour, &dt->minute, &dt->seconds);
	    if (ns != 3) {
		db_d_append_error("%s %s",
				  _("Unable to scan time:"),
				  text);
		db_d_report_error();
		return DB_FAILED;
	    }
	    break;

	case DB_SQL_TYPE_TIMESTAMP:
	    dt = &value->t;
	    G_debug(3, "sqlite fetched timestamp: %s", text);
	    ns = sscanf(text, "%4d-%2d-%2d %2d:%2d:%lf",
			&dt->year, &dt->month, &dt->day,
			&dt->hour, &dt->minute, &dt->seconds);
	    if (ns != 6) {
		db_d_append_error("%s %s",
				  _("Unable to scan timestamp:"),
				  text);
		db_d_report_error();
		return DB_FAILED;
	    }
	    break;

	case DB_SQL_TYPE_INTERVAL:
	    dt = &value->t;
	    dt->year = 0;
	    dt->month = 0;
	    dt->day = 0;
	    dt->hour = 0;
	    dt->minute = 0;
	    G_debug(3, "sqlite fetched interval: %s", text);
	    G_warning(_("SQLite driver: parsing of interval values "
			"not implemented; assuming seconds"));
	    ns = sscanf(text, "%lf", &dt->seconds);
	    if (ns != 1) {
		db_d_append_error("%s %s",
				  _("Unable to scan interval:"),
				  text);
		db_d_report_error();
		return DB_FAILED;
	    }
	    break;

	case DB_SQL_TYPE_DECIMAL:
	case DB_SQL_TYPE_NUMERIC:
	case DB_SQL_TYPE_CHARACTER:
	case DB_SQL_TYPE_TEXT:
	    db_set_string(&value->s, text);
	    break;
	}
    }

    G_debug(3, "Row fetched");

    return DB_OK;
}


/**
 * \fn int db__driver_get_num_rows (dbCursor *cn)
 *
 * \brief Gets number of rows in SQLite database table.
 *
 * \param[in] cn open database cursor
 * \return int number of rows in table
 */

int db__driver_get_num_rows(dbCursor * cn)
{
    cursor *c;
    dbToken token;
    int row, ret;

    /* get cursor token */
    token = db_get_cursor_token(cn);

    /* get the cursor by its token */
    if (!(c = (cursor *) db_find_token(token))) {
	db_d_append_error(_("Cursor not found"));
	db_d_report_error();
	return DB_FAILED;
    }

    if (c->nrows > -1) {
	return (c->nrows);
    }

    sqlite3_reset(c->statement);

    c->nrows = 0;
    while ((ret = sqlite3_step(c->statement)) == SQLITE_ROW) {
	c->nrows++;
    }

    /* get real result code */
    ret = sqlite3_reset(c->statement);

    if (ret != SQLITE_OK) {
	db_d_append_error("%s\n%s",
			  _("Unable to get number of rows:"),
			  (char *)sqlite3_errmsg(sqlite));
	db_d_report_error();
	return DB_FAILED;
    }

    /* Reset cursor position */
    row = -1;
    if (c->row > -1) {
	while (sqlite3_step(c->statement) == SQLITE_ROW) {
	    if (row == c->row)
		break;

	    row++;
	}
    }

    return (c->nrows);
}
