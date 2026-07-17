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
#include "dbdriver.h"

#include "globals.h"

MYSQL *connection;       /* Database connection */
dbString *errMsg = NULL; /* error message */

int main(int argc, char *argv[])
{
    init_dbdriver();
    exit(db_driver(argc, argv));
}
