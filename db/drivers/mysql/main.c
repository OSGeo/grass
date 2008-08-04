
/**********************************************************
 * MODULE:    mysql
 * AUTHOR(S): Radim Blazek (radim.blazek@gmail.com)
 * PURPOSE:   MySQL database driver
 * COPYRIGHT: (C) 2001 by the GRASS Development Team
 *            This program is free software under the 
 *            GNU General Public License (>=v2). 
 *            Read the file COPYING that comes with GRASS
 *            for details.
 **********************************************************/
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/dbmi.h>
#include "dbdriver.h"

#define MAIN
#include "globals.h"

int main(int argc, char *argv[])
{
    init_dbdriver();
    exit(db_driver(argc, argv));
}
