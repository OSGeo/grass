/*!
   \file db/driver/postgres/priv.c

   \brief DBMI - Low Level PostgreSQL database driver - privileges

   SPDX-License-Identifier: GPL-2.0-or-later.

   \author Radim Blazek
 */
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "globals.h"
#include "proto.h"

int db__driver_grant_on_table(dbString *tableName, int priv, int to)
{
    PGresult *res;
    dbString sql;
    dbConnection connection;

    G_debug(3, "db__driver_grant_on_table()");

    db_get_connection(&connection);
    db_init_string(&sql);

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
        db_d_append_error("%s\n%s\n%s", _("Unable grant on table:"),
                          db_get_string(&sql), PQerrorMessage(pg_conn));
        db_d_report_error();
        PQclear(res);
        db_free_string(&sql);
        return DB_FAILED;
    }

    PQclear(res);
    db_free_string(&sql);

    return DB_OK;
}
