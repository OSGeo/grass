
/**
 * \file execute.c
 *
 * \brief Low level SQLite sql execute.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Radim Blazek
 * \author Antonio Galea
 * \author Support for multiple connections by Markus Metz
 *
 * \date 2005-2011
 */

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "globals.h"
#include "proto.h"


/**
 * \fn int db__driver_execute_immediate (dbString *sql)
 *
 * \brief Low level SQLite execute sql text.
 *
 * \param[in] sql SQL statement
 * \return int DB_FAILED on error; DB_OK on success
 */

int db__driver_execute_immediate(dbString * sql)
{
    char *s;
    int ret;
    sqlite3_stmt *stmt;
    const char *rest;

    s = db_get_string(sql);

    G_debug(3, "execute: %s", s);

    /* SQLITE bug?
     * If the database schema has changed, sqlite can prepare a statement,
     * but sqlite can not step, the statement needs to be prepared anew again */
    while (1) {
	ret = sqlite3_prepare(sqlite, s, -1, &stmt, &rest);

	if (ret != SQLITE_OK) {
	    db_d_append_error("%s\n%s",
			      _("Error in sqlite3_prepare():"),
			      (char *)sqlite3_errmsg(sqlite));
	    db_d_report_error();
	    return DB_FAILED;
	}

	ret = sqlite3_step(stmt);
	/* get real result code */
	ret = sqlite3_reset(stmt);

	if (ret == SQLITE_SCHEMA) {
	    sqlite3_finalize(stmt);
	    /* try again */
	}
	else if (ret != SQLITE_OK) {
	    db_d_append_error("%s\n%s",
			      _("Error in sqlite3_step():"),
			      (char *)sqlite3_errmsg(sqlite));
	    db_d_report_error();
	    sqlite3_finalize(stmt);
	    return DB_FAILED;
	}
	else
	    break;
    }

    ret = sqlite3_finalize(stmt);

    if (ret != SQLITE_OK) {
	db_d_append_error("%s\n%s",
			  _("Error in sqlite3_finalize():"),
			  (char *)sqlite3_errmsg(sqlite));
	db_d_report_error();
	return DB_FAILED;
    }

    return DB_OK;
}


/**
 * \fn int db__driver_begin_transaction (void)
 *
 * \brief Low level SQLite begin SQL transaction.
 *
 * \return int DB_FAILED on error; DB_OK on success
 */

int db__driver_begin_transaction(void)
{
    int ret;

    G_debug(3, "execute: BEGIN");

    ret = sqlite3_exec(sqlite, "BEGIN", NULL, NULL, NULL);

    if (ret != SQLITE_OK) {
	db_d_append_error("%s\n%s",
			  _("'BEGIN' transaction failed:"),
			  (char *)sqlite3_errmsg(sqlite));
	db_d_report_error();
	return DB_FAILED;
    }

    return DB_OK;
}


/**
 * \fn int db__driver_commit_transaction (void)
 *
 * \brief Low level SQLite commit transaction.
 *
 * \return int DB_FAILED on error; DB_OK on success
 */

int db__driver_commit_transaction(void)
{
    int ret;

    G_debug(3, "execute: COMMIT");

    ret = sqlite3_exec(sqlite, "COMMIT", NULL, NULL, NULL);

    if (ret != SQLITE_OK) {
	db_d_append_error("%s\n%s",
			  _("'COMMIT' transaction failed:"),
			  (char *)sqlite3_errmsg(sqlite));
	db_d_report_error();
	return DB_FAILED;
    }

    return DB_OK;
}
