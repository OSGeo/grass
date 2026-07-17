/**********************************************************
 * MODULE:    mysql
 * AUTHOR(S): Radim Blazek (radim.blazek@gmail.com)
 * PURPOSE:   MySQL database driver
 * SPDX-FileCopyrightText: 2001 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 **********************************************************/
#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "globals.h"
#include "proto.h"

int db__driver_list_tables(dbString **tlist, int *tcount, int system G_UNUSED)
{
    int i;
    dbString *list;
    int nrows;
    MYSQL_RES *res;
    MYSQL_ROW row;

    *tlist = NULL;
    *tcount = 0;

    res = mysql_list_tables(connection, NULL);

    if (res == NULL) {
        db_d_append_error("%s\n%s", _("Unable get list of tables:"),
                          mysql_error(connection));
        db_d_report_error();
        return DB_FAILED;
    }
    mysql_store_result(connection);

    nrows = (int)mysql_num_rows(res);
    list = db_alloc_string_array(nrows);

    i = 0;
    while ((row = mysql_fetch_row(res)) != NULL) {
        db_set_string(&list[i], row[0]);
        i++;
    }

    mysql_free_result(res);

    *tlist = list;
    *tcount = nrows;
    return DB_OK;
}
