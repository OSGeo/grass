/*!
  \file db/driver/postgres/db.c
  
  \brief DBMI - Low Level PostgreSQL database driver - select cursor
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Radim Blazek
 */

#include <stdlib.h>
#include <grass/dbmi.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "globals.h"
#include "proto.h"

int db__driver_open_select_cursor(dbString * sel, dbCursor * dbc, int mode)
{
    PGresult *res;
    cursor *c;
    dbTable *table;
    char *str;

    /* Set datetime style */
    res = PQexec(pg_conn, "SET DATESTYLE TO ISO");

    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
	db_d_append_error(_("Unable set DATESTYLE"));
	db_d_report_error();
	PQclear(res);
	return DB_FAILED;
    }

    PQclear(res);

    /* allocate cursor */
    c = alloc_cursor();
    if (c == NULL)
	return DB_FAILED;

    db_set_cursor_mode(dbc, mode);
    db_set_cursor_type_readonly(dbc);

    /* \ must be escaped, see explanation in db_driver_execute_immediate() */
    str = G_str_replace(db_get_string(sel), "\\", "\\\\");
    G_debug(3, "Escaped SQL: %s", str);

    c->res = PQexec(pg_conn, str);

    if (!c->res || PQresultStatus(c->res) != PGRES_TUPLES_OK) {
	db_d_append_error("%s\n%s\n%s",
			  _("Unable to select:"),
			  db_get_string(sel),
			  PQerrorMessage(pg_conn));
	db_d_report_error();
	PQclear(c->res);
	if (str)
	    G_free(str);
	return DB_FAILED;
    }

    if (str)
	G_free(str);

    if (describe_table(c->res, &table, c) == DB_FAILED) {
	db_d_append_error(_("Unable to describe table"));
	db_d_report_error();
	PQclear(res);
	return DB_FAILED;
    }

    c->nrows = PQntuples(c->res);
    c->row = -1;

    /* record table with dbCursor */
    db_set_cursor_table(dbc, table);

    /* set dbCursor's token for my cursor */
    db_set_cursor_token(dbc, c->token);

    return DB_OK;
}
