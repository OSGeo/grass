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

#include <stdlib.h>
#include <string.h>
#include <grass/dbmi.h>
#include "globals.h"
#include "proto.h"
#include "dbdriver.h"

DATABASE db;
dbString *errMsg = NULL;

int main(int argc, char *argv[])
{
    char *name;

    init_dbdriver();

    /* Do not call G_getenv() nor other functions reading GISRC here! It may be
     * that grass variables are not available here, but will be set in
     * db_driver() */

    /* Set pointer to driver name */
    name = argv[0] + strlen(argv[0]);

    while (name > argv[0]) {
        if (name[0] == '/') {
            name++;
            break;
        }
        name--;
    }

    exit(db_driver(argc, argv));
}
