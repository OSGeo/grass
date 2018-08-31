
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
