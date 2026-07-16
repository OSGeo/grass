/**********************************************************
 * MODULE:    mysql
 * AUTHOR(S): Radim Blazek (radim.blazek@gmail.com)
 * PURPOSE:   MySQL database driver
 * SPDX-FileCopyrightText: 2001 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 **********************************************************/
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "globals.h"
#include "proto.h"

int db__driver_create_index(dbIndex *index)
{
    int i, ncols;
    dbString sql;

    G_debug(3, "db__create_index()");

    db_init_string(&sql);

    ncols = db_get_index_number_of_columns(index);

    db_set_string(&sql, "CREATE");
    if (db_test_index_type_unique(index))
        db_append_string(&sql, " UNIQUE");

    db_append_string(&sql, " INDEX ");
    db_append_string(&sql, db_get_index_name(index));
    db_append_string(&sql, " ON ");

    db_append_string(&sql, db_get_index_table_name(index));

    db_append_string(&sql, " ( ");

    for (i = 0; i < ncols; i++) {
        if (i > 0)
            db_append_string(&sql, ", ");

        db_append_string(&sql, db_get_index_column_name(index, i));
    }

    db_append_string(&sql, " )");

    G_debug(3, " SQL: %s", db_get_string(&sql));

    if (mysql_query(connection, db_get_string(&sql)) != 0) {
        db_d_append_error("%s\n%s\n%s", _("Unable to create index:"),
                          db_get_string(&sql), mysql_error(connection));
        db_d_report_error();
        db_free_string(&sql);
        return DB_FAILED;
    }

    db_free_string(&sql);

    return DB_OK;
}

