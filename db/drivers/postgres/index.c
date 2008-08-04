#include <grass/dbmi.h>
#include "globals.h"
#include "proto.h"

int db__driver_create_index(dbIndex * index)
{
    int i, ncols;
    PGresult *res;
    dbString sql;

    G_debug(3, "db__create_index()");

    db_init_string(&sql);
    init_error();

    ncols = db_get_index_number_of_columns(index);

    db_set_string(&sql, "create");
    if (db_test_index_type_unique(index))
	db_append_string(&sql, " unique");

    db_append_string(&sql, " index ");
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

    res = PQexec(pg_conn, db_get_string(&sql));

    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
	append_error("Cannot create index:\n");
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
