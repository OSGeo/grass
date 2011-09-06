
/**
 * \file listtab.c
 *
 * \brief Low level SQLite table functions.
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
#include "globals.h"
#include "proto.h"


/**
 * \fn int db__driver_list_tables (dbString **tlist, int *tcount, int system)
 *
 * \brief List SQLite database tables.
 *
 * \param[in] tlist list of tables
 * \param[in] tcount number of tables
 * \param[in] system
 * \return int DB_FAILED on error; DB_OK on success
 */

int db__driver_list_tables(dbString ** tlist, int *tcount, int system)
{
    int i, nrows;
    dbString *list;
    sqlite3_stmt *statement;
    const char *rest;
    int ret;

    init_error();

    ret = sqlite3_prepare(sqlite,
			  "select name from sqlite_master where type = 'table' or type = 'view'",
			  -1, &statement, &rest);

    while (ret == SQLITE_BUSY || ret == SQLITE_IOERR_BLOCKED) {
	ret = sqlite3_busy_handler(sqlite, sqlite_busy_callback, NULL);
    }

    if (ret != SQLITE_OK) {
	append_error("Cannot list tables\n");
	append_error((char *)sqlite3_errmsg(sqlite));
	report_error();
	sqlite3_finalize(statement);
	return DB_FAILED;
    }

    nrows = 0;
    while (sqlite3_step(statement) == SQLITE_ROW) {
	nrows++;
    }
    sqlite3_reset(statement);

    G_debug(3, "nrows = %d", nrows);

    list = db_alloc_string_array(nrows);

    if (list == NULL) {
	append_error("Cannot db_alloc_string_array()");
	report_error();
	return DB_FAILED;
    }

    i = 0;
    while (sqlite3_step(statement) == SQLITE_ROW) {
	G_debug(3, "table: %s", sqlite3_column_text(statement, 0));
	db_set_string(&list[i], (char *)sqlite3_column_text(statement, 0));
	i++;
    }

    sqlite3_finalize(statement);

    *tlist = list;
    *tcount = nrows;

    return DB_OK;
}
