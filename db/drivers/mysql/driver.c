/**********************************************************
 * MODULE:    mysql
 * AUTHOR(S): Radim Blazek (radim.blazek@gmail.com)
 * PURPOSE:   MySQL database driver
 * SPDX-FileCopyrightText: 2001 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 **********************************************************/
#include <stdlib.h>

#include <grass/gis.h>
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
