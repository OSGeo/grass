/**********************************************************
 * MODULE:    mysql
 * AUTHOR(S): Radim Blazek (radim.blazek@gmail.com)
 * PURPOSE:   MySQL database driver
 * SPDX-FileCopyrightText: 2001 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 **********************************************************/
#include <stdio.h>

#include <grass/gis.h>
#include <grass/dbmi.h>

#include "globals.h"
#include "proto.h"

/* init error message */
void init_error(void)
{
    db_d_init_error("MySQL");
}
