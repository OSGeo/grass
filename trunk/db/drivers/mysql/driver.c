
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

#include "globals.h"
#include "proto.h"

int db__driver_init(int argc, char *argv[])
{
    init_error();
    return DB_OK;
}

int db__driver_finish()
{
    return DB_OK;
}
