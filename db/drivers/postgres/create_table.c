/*!
  \file db/driver/postgres/create_table.c
  
  \brief DBMI - Low Level PostgreSQL database driver - create table
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Radim Blazek
 */

#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "globals.h"
#include "proto.h"

int db__driver_create_table(dbTable * table)
{
    int col, ncols;
    dbColumn *column;
    const char *colname;
    int sqltype;
    char buf[500];
    PGresult *res;
    dbString sql;
    dbConnection connection;

    G_debug(3, "db__driver_create_table()");

    db_init_string(&sql);

    /* db_table_to_sql ( table, &sql ); */

    db_set_string(&sql, "create table ");
    db_append_string(&sql, db_get_table_name(table));
    db_append_string(&sql, " ( ");

    ncols = db_get_table_number_of_columns(table);

    for (col = 0; col < ncols; col++) {
	column = db_get_table_column(table, col);
	colname = db_get_column_name(column);
	sqltype = db_get_column_sqltype(column);

	G_debug(3, "%s (%s)", colname, db_sqltype_name(sqltype));

	if (col > 0)
	    db_append_string(&sql, ", ");
	db_append_string(&sql, colname);
	db_append_string(&sql, " ");
	switch (sqltype) {
	case DB_SQL_TYPE_CHARACTER:
	    sprintf(buf, "varchar(%d)", db_get_column_length(column));
	    db_append_string(&sql, buf);
	    break;
	case DB_SQL_TYPE_TEXT:
	    db_append_string(&sql, "text");
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

	    /* TODO: better numeric types */
	case DB_SQL_TYPE_DOUBLE_PRECISION:
	case DB_SQL_TYPE_DECIMAL:
	case DB_SQL_TYPE_NUMERIC:
	case DB_SQL_TYPE_INTERVAL:
	    db_append_string(&sql, "double precision");
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

	default:
	    G_warning(_("Unknown column type (%s)"), colname);
	    return DB_FAILED;
	}
    }
    db_append_string(&sql, " )");


    G_debug(3, " SQL: %s", db_get_string(&sql));

    res = PQexec(pg_conn, db_get_string(&sql));

    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
	db_d_append_error("%s\n%s\n%s",
			  _("Unable to create table:"),
			  db_get_string(&sql),
			  PQerrorMessage(pg_conn));
	db_d_report_error();
	PQclear(res);
	db_free_string(&sql);
	return DB_FAILED;
    }

    PQclear(res);

    /* Grant privileges */
    db_get_connection(&connection);

    db_set_string(&sql, "grant select on ");
    db_append_string(&sql, db_get_table_name(table));
    db_append_string(&sql, " to public");

    if (connection.group) {
	db_append_string(&sql, ", group ");
	db_append_string(&sql, connection.group);
    }

    G_debug(3, " SQL: %s", db_get_string(&sql));

    res = PQexec(pg_conn, db_get_string(&sql));

    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
	db_d_append_error("%s\n%s\%s",
			  _("Unable to grant select on table:"),
			  db_get_string(&sql),
			  PQerrorMessage(pg_conn));
	db_d_report_error();
	PQclear(res);
	db_free_string(&sql);
	return DB_FAILED;
    }

    PQclear(res);
    db_free_string(&sql);

    return DB_OK;
}
