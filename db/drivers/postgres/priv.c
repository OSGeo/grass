#include <grass/dbmi.h>
#include "globals.h"
#include "proto.h"

int db__driver_grant_on_table(dbString * tableName, int priv, int to)
{
    PGresult *res;
    dbString sql;
    dbConnection connection;

    G_debug(3, "db__driver_grant_on_table()");

    db_get_connection(&connection);
    db_init_string(&sql);
    init_error();

    db_set_string(&sql, "grant ");
    if (priv | DB_PRIV_SELECT)
	db_append_string(&sql, "select ");

    db_append_string(&sql, "on ");
    db_append_string(&sql, db_get_string(tableName));

    db_append_string(&sql, " to ");

    if (to | DB_GROUP && connection.group) {
	db_append_string(&sql, "group ");
	db_append_string(&sql, connection.group);

	if (to | DB_PUBLIC)
	    db_append_string(&sql, ", ");
    }

    if (to | DB_PUBLIC)
	db_append_string(&sql, "public");

    G_debug(3, " SQL: %s", db_get_string(&sql));

    res = PQexec(pg_conn, db_get_string(&sql));

    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
	append_error("Cannot grant on table:\n");
	append_error(db_get_string(&sql));
	append_error("\n");
	append_error(PQerrorMessage(pg_conn));
	report_error();
	PQclear(res);
	db_free_string(&sql);
	return DB_FAILED;
    }

    PQclear(res);
    db_free_string(&sql);

    return DB_OK;
}
