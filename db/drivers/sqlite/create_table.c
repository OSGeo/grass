
/**
 * \file create_table.c
 *
 * \brief Low level SQLite table creation.
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
#include "globals.h"
#include "proto.h"


/**
 * \fn int db__driver_create_table (dbTable *table)
 *
 * \brief SQLite create table.
 *
 * \param[in] table
 * \return int DB_FAILED on error; DB_OK on success
 */

int db__driver_create_table(dbTable * table)
{
    int col, ncols;
    sqlite3_stmt *statement;
    dbString sql;
    const char *rest;
    int ret;

    G_debug(3, "db__driver_create_table()");

    init_error();

    db_init_string(&sql);

    /* db_table_to_sql ( table, &sql ); */

    db_set_string(&sql, "create table ");
    db_append_string(&sql, db_get_table_name(table));
    db_append_string(&sql, " ( ");

    ncols = db_get_table_number_of_columns(table);

    for (col = 0; col < ncols; col++) {
	dbColumn *column = db_get_table_column(table, col);
	const char *colname = db_get_column_name(column);
	int sqltype = db_get_column_sqltype(column);
	int collen = db_get_column_length(column);
	char buf[32];

	G_debug(3, "%s (%s)", colname, db_sqltype_name(sqltype));

	if (col > 0)
	    db_append_string(&sql, ", ");
	db_append_string(&sql, colname);
	db_append_string(&sql, " ");
	switch (sqltype) {
	case DB_SQL_TYPE_CHARACTER:
	    sprintf(buf, "varchar(%d)", collen);
	    db_append_string(&sql, buf);
	    break;
	case DB_SQL_TYPE_SMALLINT:
	    db_append_string(&sql, "smallint");
	    break;
	case DB_SQL_TYPE_INTEGER:
	    db_append_string(&sql, "integer");
	    break;
	case DB_SQL_TYPE_REAL:
	    db_append_string(&sql, "real");
	    break;
	case DB_SQL_TYPE_DOUBLE_PRECISION:
	    db_append_string(&sql, "double precision");
	    break;
	case DB_SQL_TYPE_DECIMAL:
	    db_append_string(&sql, "decimal");
	    break;
	case DB_SQL_TYPE_NUMERIC:
	    db_append_string(&sql, "numeric");
	    break;
	case DB_SQL_TYPE_DATE:
	    db_append_string(&sql, "date");
	    break;
	case DB_SQL_TYPE_TIME:
	    db_append_string(&sql, "time");
	    break;
	case DB_SQL_TYPE_TIMESTAMP:
	    db_append_string(&sql, "timestamp");
	    break;
	case DB_SQL_TYPE_INTERVAL:
	    db_append_string(&sql, "interval");
	    break;
	case DB_SQL_TYPE_TEXT:
	    db_append_string(&sql, "text");
	    break;
	case DB_SQL_TYPE_SERIAL:
	    db_append_string(&sql, "serial");
	    break;
	default:
	    G_warning("Unknown column type (%s)", colname);
	    return DB_FAILED;
	}
    }
    db_append_string(&sql, " )");

    G_debug(3, " SQL: %s", db_get_string(&sql));

    ret = sqlite3_prepare(sqlite, db_get_string(&sql), -1, &statement, &rest);

    if (ret != SQLITE_OK) {
	append_error("Cannot create table:\n");
	append_error(db_get_string(&sql));
	append_error("\n");
	append_error((char *)sqlite3_errmsg(sqlite));
	report_error();
	sqlite3_finalize(statement);
	db_free_string(&sql);
	return DB_FAILED;
    }

    ret = sqlite3_step(statement);

    if (ret != SQLITE_DONE) {
	append_error("Error in sqlite3_step():\n");
	append_error((char *)sqlite3_errmsg(sqlite));
	report_error();
	return DB_FAILED;
    }

    sqlite3_finalize(statement);
    db_free_string(&sql);

    return DB_OK;
}
