/*****************************************************************************
 *
 * MODULE:       DBF driver
 *
 * AUTHOR(S):    Radim Blazek
 *
 * PURPOSE:      Simple driver for reading and writing dbf files
 *
 * SPDX-FileCopyrightText: 2000 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
 *
 *****************************************************************************/

#include <grass/dbmi.h>
#include "globals.h"
#include "proto.h"

int db__driver_list_tables(dbString **tlist, int *tcount, int system G_UNUSED)
{
    dbString *list;
    int i;

    *tlist = NULL;
    *tcount = 0;

    list = db_alloc_string_array(db.ntables);
    if (list == NULL && db.ntables > 0)
        return DB_FAILED;

    for (i = 0; i < db.ntables; i++) {
        if (db_set_string(&list[i], (char *)db.tables[i].name) != DB_OK) {
            return DB_FAILED;
        }
    }

    *tlist = list;
    *tcount = db.ntables;
    return DB_OK;
}
