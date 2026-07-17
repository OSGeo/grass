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
 *
 *****************************************************************************/

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
