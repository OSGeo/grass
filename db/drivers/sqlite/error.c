/***********************************************************
 *
 * MODULE:       SQLite driver
 *
 * AUTHOR(S):    Radim Blazek, Markus Metz
 *
 * SPDX-FileCopyrightText: 2011 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
 *
 **************************************************************/
#include <stdio.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include "proto.h"
#include "globals.h"

/* init error message */
void init_error(void)
{
    db_d_init_error("SQLite");
}
