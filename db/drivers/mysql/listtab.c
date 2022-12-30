/**********************************************************
 * MODULE:    mysql
 * AUTHOR(S): Radim Blazek (radim.blazek@gmail.com)
 * PURPOSE:   MySQL database driver
 * COPYRIGHT: (C) 2001 by the GRASS Development Team
 *            This program is free software under the
 *            GNU General Public License (>=v2).
 *            Read the file COPYING that comes with GRASS
 *            for details.
 **********************************************************/
#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "globals.h"
#include "proto.h"

<<<<<<< HEAD
<<<<<<< HEAD
int db__driver_list_tables(dbString **tlist, int *tcount, int system UNUSED)
=======
int db__driver_list_tables(dbString **tlist, int *tcount, int system)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
int db__driver_list_tables(dbString **tlist, int *tcount, int system)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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
<<<<<<< HEAD
<<<<<<< HEAD
        db_d_append_error("%s\n%s", _("Unable get list of tables:"),
=======
        db_d_append_error("%s\%s", _("Unable get list of tables:"),
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        db_d_append_error("%s\%s", _("Unable get list of tables:"),
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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
