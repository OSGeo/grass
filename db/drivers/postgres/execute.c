/*!
  \file db/driver/postgres/execute.c
  
  \brief DBMI - Low Level PostgreSQL database driver - execute statemets
  
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

int db__driver_execute_immediate(dbString * sql)
{
    PGresult *res;
    char *str;

    /* Postgres supports in addition to standard escape character '
     * (apostrophe) also \ (basckslash) as this is not SQL standard,
     * GRASS modules cannot work escape all \ in the text because
     * other drivers do not support this feature. For example, if a
     * text contains string \' GRASS modules escape ' by another ' and
     * string passed to driver is \'' postgres takes \' as ' but
     * second ' remains not escaped, result is error.  Because of
     * this, all occurrences of \ in sql are escaped by \ */
    str = G_str_replace(db_get_string(sql), "\\", "\\\\");

    G_debug(3, "db__driver_execute_immediate(): Escaped SQL: '%s'", str);

    res = PQexec(pg_conn, str);

    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
	db_d_append_error("%s\n%s\n%s",
			  _("Unable to execute:"),
			  str,
			  PQerrorMessage(pg_conn));
	db_d_report_error();
	PQclear(res);
	if (str)
	    G_free(str);
	return DB_FAILED;
    }

    if (str)
	G_free(str);
    PQclear(res);

    return DB_OK;
}

int db__driver_begin_transaction(void)
{
    PGresult *res;

    G_debug(2, "pg : BEGIN");

    res = PQexec(pg_conn, "BEGIN");

    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
	db_d_append_error(_("Unable to 'BEGIN' transaction"));
	db_d_report_error();
	PQclear(res);
	return DB_FAILED;
    }

    PQclear(res);

    return DB_OK;
}

int db__driver_commit_transaction(void)
{
    PGresult *res;

    G_debug(2, "pg : COMMIT");

    res = PQexec(pg_conn, "COMMIT");

    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
	db_d_append_error(_("Unable to 'COMMIT' transaction"));
	db_d_report_error();
	PQclear(res);
	return DB_FAILED;
    }

    PQclear(res);

    return DB_OK;
}
