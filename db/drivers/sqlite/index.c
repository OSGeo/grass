
/**
 * \file index.c
 *
 * \brief Low level SQLite database index functions.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Radim Blazek
 * \author Support for multiple connections by Markus Metz
 *
 * \date 2005-2011
 */

#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "globals.h"
#include "proto.h"


/**
 * \fn int db__driver_create_index (dbIndex *index)
 *
 * \brief Create SQLite database index.
 *
 * \param[in] index index to be created
 * \return int DB_FAILED on error; DB_OK on success
 */

int db__driver_create_index(dbIndex * index)
{
    int i, ncols;
    sqlite3_stmt *statement;
    dbString sql;
    const char *rest;
    int ret;

    G_debug(3, "db__create_index()");

    db_init_string(&sql);

    ncols = db_get_index_number_of_columns(index);

    db_set_string(&sql, "create");
    if (db_test_index_type_unique(index))
	db_append_string(&sql, " unique");

    db_append_string(&sql, " index ");
    db_append_string(&sql, " if not exists ");
    db_append_string(&sql, db_get_index_name(index));
    db_append_string(&sql, " on ");

    db_append_string(&sql, db_get_index_table_name(index));

    db_append_string(&sql, " ( ");

    for (i = 0; i < ncols; i++) {
	if (i > 0)
	    db_append_string(&sql, ", ");

	db_append_string(&sql, db_get_index_column_name(index, i));
    }

    db_append_string(&sql, " )");

    G_debug(3, " SQL: %s", db_get_string(&sql));

    /* SQLITE bug?
     * If the database schema has changed, sqlite can prepare a statement,
     * but sqlite can not step, the statement needs to be prepared anew again */
    while (1) {
	ret = sqlite3_prepare(sqlite, db_get_string(&sql), -1, &statement, &rest);

	if (ret != SQLITE_OK) {
	    db_d_append_error("%s\n%s\n%s",
			      _("Unable to create index:"),
			      db_get_string(&sql),
			      (char *)sqlite3_errmsg(sqlite));
	    db_d_report_error();
	    sqlite3_finalize(statement);
	    db_free_string(&sql);
	    return DB_FAILED;
	}

	ret = sqlite3_step(statement);
	/* get real result code */
	ret = sqlite3_reset(statement);

	if (ret == SQLITE_SCHEMA) {
	    sqlite3_finalize(statement);
	    /* try again */
	}
	else if (ret != SQLITE_OK) {
	    db_d_append_error("%s\n%s",
			      _("Error in sqlite3_step():"),
			      (char *)sqlite3_errmsg(sqlite));
	    db_d_report_error();
	    sqlite3_finalize(statement);
	    db_free_string(&sql);
	    return DB_FAILED;
	}
	else
	    break;
    }

    sqlite3_finalize(statement);
    db_free_string(&sql);

    return DB_OK;
}
