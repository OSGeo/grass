
/**********************************************************
 * MODULE:    mysql
 * AUTHOR(S): Radim Blazek (radim.blazek@gmail.com)
 * PURPOSE:   MySQL database driver
 * COPYRIGHT: (C) 2001 by the GRASS Development Team
 *            This program is free software under the 
 *            GNU General Public License (>=v2). 
 *            Read the file COPYING that comes with GRASS
 *            for details.
 **********************************************************/
#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
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
	c->row = mysql_fetch_row(c->res);
	break;

    case DB_CURRENT:
	break;

    case DB_PREVIOUS:
    case DB_FIRST:
    case DB_LAST:
    default:
	db_d_append_error(_("Cursor position is not supported "
			    "by MySQL driver"));
	db_d_report_error();
	return DB_FAILED;
    }

    G_debug(3, "nrows = %d", c->nrows);
    if (c->row == NULL) {
	*more = 0;
	return DB_OK;
    }

    *more = 1;

    /* get the data out of the descriptor into the table */
    table = db_get_cursor_table(cn);

    for (i = 0; i < c->ncols; i++) {
	int col, sqltype, mysqltype;
	dbColumn *column;
	dbValue *value;
	char *val;

	col = c->cols[i];	/* known column */

	column = db_get_table_column(table, i);
	mysqltype = db_get_column_host_type(column);
	sqltype = db_get_column_sqltype(column);

	value = db_get_column_value(column);
	db_zero_string(&value->s);
	value->t.year = 0;
	value->t.month = 0;
	value->t.day = 0;
	value->t.hour = 0;
	value->t.minute = 0;
	value->t.seconds = 0.0;

	val = c->row[i];
	if (!val) {
	    value->isNull = 1;
	    continue;
	}
	else {
	    value->isNull = 0;
	}

	G_debug(3, "col %d, mysqltype %d, sqltype %d, val = '%s'",
		col, mysqltype, sqltype, c->row[col]);

	/* defined in /usr/include/mysql/mysql_com.h */
	switch (mysqltype) {
	    int ns;

	case MYSQL_TYPE_TINY:
	case MYSQL_TYPE_SHORT:
	case MYSQL_TYPE_LONG:
	case MYSQL_TYPE_INT24:
	case MYSQL_TYPE_LONGLONG:
	case MYSQL_TYPE_YEAR:
	    value->i = atoi(val);
	    break;

	case MYSQL_TYPE_FLOAT:
	case MYSQL_TYPE_DOUBLE:
	    value->d = atof(val);
	    break;

	    /* MySQL TIMESTAMP < 4.1: YYYYMMDDHHMMSS TIMESTAMP(14)
	     *                        YYMMDDHHMMSS   TIMESTAMP(12)
	     *                        YYMMDDHHMM     TIMESTAMP(10)
	     *                        YYYYMMDD       TIMESTAMP(8)
	     *                        YYMMDD         TIMESTAMP(6)
	     *                        YYMM           TIMESTAMP(4)
	     *                        YY             YY
	     * MySQL TIMESTAMP >= 4.1: 'YYYY-MM-DD HH:MM:SS' (19 chars) */
	case MYSQL_TYPE_TIMESTAMP:
	    {
		char valbuf[25], buf[10];

		memset(valbuf, 0, 25);
		strcpy(valbuf, val);

		switch (strlen(val)) {
		case 2:
		case 4:
		case 6:
		case 10:
		case 12:
		    strncpy(buf, val, 2);
		    buf[2] = 0;
		    value->t.year = atoi(buf);
		    strncpy(buf, val + 2, 2);
		    buf[2] = 0;
		    value->t.month = atoi(buf);
		    strncpy(buf, val + 4, 2);
		    buf[2] = 0;
		    value->t.day = atoi(buf);
		    strncpy(buf, val + 6, 2);
		    buf[2] = 0;
		    value->t.hour = atoi(buf);
		    strncpy(buf, val + 8, 2);
		    buf[2] = 0;
		    value->t.minute = atoi(buf);
		    strncpy(buf, val + 10, 2);
		    buf[2] = 0;
		    value->t.seconds = atof(buf);
		    break;

		case 8:
		case 14:
		    strncpy(buf, val, 4);
		    buf[4] = 0;
		    value->t.year = atoi(buf);
		    strncpy(buf, val + 4, 2);
		    buf[2] = 0;
		    value->t.month = atoi(buf);
		    strncpy(buf, val + 6, 2);
		    buf[2] = 0;
		    value->t.day = atoi(buf);
		    strncpy(buf, val + 8, 2);
		    buf[2] = 0;
		    value->t.hour = atoi(buf);
		    strncpy(buf, val + 10, 2);
		    buf[2] = 0;
		    value->t.minute = atoi(buf);
		    strncpy(buf, val + 12, 2);
		    buf[2] = 0;
		    value->t.seconds = atof(buf);
		    break;

		case 19:
		    ns = sscanf(val, "%4d-%2d-%2d %2d:%2d:%lf",
				&(value->t.year), &(value->t.month),
				&(value->t.day), &(value->t.hour),
				&(value->t.minute), &(value->t.seconds));

		    if (ns != 6) {
			db_d_append_error("%s %s",
					  _("Unable to scan timestamp: "),
					  val);
			db_d_report_error();
			return DB_FAILED;
		    }
		    break;

		default:
		    db_d_append_error("%s %s",
				      _("Unknown timestamp format: "),
				      val);
		    db_d_report_error();
		    return DB_FAILED;
		}
	    }
	    break;

	    /* MySQL DATE: 'YYYY-MM-DD' */
	case MYSQL_TYPE_DATE:
	    ns = sscanf(val, "%4d-%2d-%2d", &(value->t.year),
			&(value->t.month), &(value->t.day));

	    if (ns != 3) {
		db_d_append_error("%s %s",
				  _("Unable to scan date: "),
				  val);
		db_d_report_error();
		return DB_FAILED;
	    }
	    break;

	    /* MySQL DATETIME: 'HH:MM:SS' */
	case MYSQL_TYPE_TIME:
	    ns = sscanf(val, "%2d:%2d:%lf", &(value->t.hour),
			&(value->t.minute), &(value->t.seconds));

	    if (ns != 3) {
		db_d_append_error("%s %s",
				  _("Unable to scan time: "),
				  val);
		db_d_report_error();
		return DB_FAILED;
	    }
	    break;

	    /* MySQL DATETIME: 'YYYY-MM-DD HH:MM:SS' */
	case MYSQL_TYPE_DATETIME:
	    ns = sscanf(val, "%4d-%2d-%2d %2d:%2d:%lf",
			&(value->t.year), &(value->t.month),
			&(value->t.day), &(value->t.hour),
			&(value->t.minute), &(value->t.seconds));

	    if (ns != 6) {
		db_d_append_error("%s %s",
				  _("Unable to scan datetime:"),
				  val);
		db_d_report_error();
		return DB_FAILED;
	    }
	    break;

	case MYSQL_TYPE_STRING:
	case MYSQL_TYPE_VAR_STRING:
	case MYSQL_TYPE_SET:
	case MYSQL_TYPE_ENUM:
	case MYSQL_TYPE_BLOB:
	    db_set_string(&(value->s), val);
	    break;
	}
    }
    G_debug(3, "Row fetched");
    return DB_OK;
}

int db__driver_get_num_rows(cn)
     dbCursor *cn;
{
    cursor *c;
    dbToken token;

    /* get cursor token */
    token = db_get_cursor_token(cn);

    /* get the cursor by its token */
    if (!(c = (cursor *) db_find_token(token))) {
	db_d_append_error(_("Cursor not found"));
	db_d_report_error();
	return DB_FAILED;
    }

    return (c->nrows);
}
