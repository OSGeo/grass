/*****************************************************************************
 *
 * MODULE:       DBF driver
 *
 * AUTHOR(S):    Radim Blazek
 *
 * PURPOSE:      Simple driver for reading and writing dbf files
 *
 * SPDX-FileCopyrightText: 2000 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *****************************************************************************/
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "globals.h"
#include "proto.h"

int db__driver_execute_immediate(dbString *sql)
{
    char *s;
    int ret;

    s = db_get_string(sql);

    ret = execute(s, NULL);

    if (ret == DB_FAILED) {
        db_d_append_error(_("Unable to execute statement."));
        db_d_report_error();
        return DB_FAILED;
    }

    return DB_OK;
}

