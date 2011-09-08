
/****************************************************************************
 *
 * MODULE:       execute
 * AUTHOR(S):    Alex Shevlakov <sixote yahoo.com> (original contributor)
 *               Huidae Cho <grass4u gmail.com>, Glynn Clements <glynn gclements.plus.com>, Markus Neteler <neteler itc.it>, Radim Blazek <radim.blazek gmail.com>
 * PURPOSE:      PostgreSQL driver
 * COPYRIGHT:    (C) 2002-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
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

    init_error();

    /* Postgres supports in addition to standard escape character ' (apostrophe) also \ (basckslash)
     * as this is not SQL standard, GRASS modules cannot work escape all \ in the text
     * because other drivers do not support this feature. For example, if a text contains 
     * string \' GRASS modules escape ' by another ' and string passed to driver is \''
     * postgres takes \' as ' but second ' remains not escaped, result is error.
     * Because of this, all occurencies of \ in sql are escaped by \ */
    str = G_str_replace(db_get_string(sql), "\\", "\\\\");

    G_debug(3, "Escaped SQL: %s", str);

    res = PQexec(pg_conn, str);

    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
	append_error(_("Unable to execute:\n"));
	append_error(str);
	append_error("\n");
	append_error(PQerrorMessage(pg_conn));
	report_error();
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

    init_error();
    res = PQexec(pg_conn, "BEGIN");

    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
	append_error(_("Unable to 'BEGIN' transaction"));
	report_error();
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

    init_error();
    res = PQexec(pg_conn, "COMMIT");

    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
	append_error(_("Unable to 'COMMIT' transaction"));
	report_error();
	PQclear(res);
	return DB_FAILED;
    }

    PQclear(res);

    return DB_OK;
}
