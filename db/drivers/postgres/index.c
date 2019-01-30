/*!
  \file db/driver/postgres/index.c
  
  \brief DBMI - Low Level PostgreSQL database driver - index management

  \todo implement time zone handling
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Radim Blazek
 */

#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "globals.h"
#include "proto.h"

int db__driver_create_index(dbIndex * index)
{
    int i, ncols;
    PGresult *res;
    dbString sql;

    G_debug(3, "db__create_index()");

    db_init_string(&sql);

    ncols = db_get_index_number_of_columns(index);

    db_set_string(&sql, "create");
    if (db_test_index_type_unique(index))
	db_append_string(&sql, " unique");

    db_append_string(&sql, " index ");
    if (PQserverVersion(pg_conn) >= 90500)
	db_append_string(&sql, "if not exists ");
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
	db_d_append_error("%s: %s\n%s",
			  _("Unable to create index"),
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
