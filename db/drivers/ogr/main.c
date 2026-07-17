/*****************************************************************************
 *
 * MODULE:       OGR driver
 *
 * AUTHOR(S):    Radim Blazek
 *               Some updates by Martin Landa <landa.martin gmail.com>
 *
 * PURPOSE:      DB driver for OGR sources
 *
 * SPDX-FileCopyrightText: 2004-2009 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 *****************************************************************************/

#include <stdlib.h>

#include <grass/dbmi.h>

#include <ogr_api.h>

#include "globals.h"
#include "dbdriver.h"

OGRDataSourceH hDs;
dbString *errMsg = NULL; /* error message */

int main(int argc, char *argv[])
{
    init_dbdriver();
    exit(db_driver(argc, argv));
}
