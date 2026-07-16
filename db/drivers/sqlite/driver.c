/***********************************************************
 *
 * MODULE:       SQLite driver
 *
 * AUTHOR(S):    Radim Blazek, Markus Metz
 *
 * SPDX-FileCopyrightText: 2011 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 **************************************************************/
#include <grass/dbmi.h>
#include "globals.h"
#include "proto.h"

int db__driver_init(int argc G_UNUSED, char *argv[] G_UNUSED)
{
    init_error();

    return DB_OK;
}

int db__driver_finish(void)
{
    return DB_OK;
}

