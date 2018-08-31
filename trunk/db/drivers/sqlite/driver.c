
/***********************************************************
*
* MODULE:       SQLite driver 
*   	    	
* AUTHOR(S):    Radim Blazek, Markus Metz
*
* COPYRIGHT:    (C) 2011 by the GRASS Development Team
*
* This program is free software under the GNU General Public
* License (>=v2). Read the file COPYING that comes with GRASS
* for details.
*
**************************************************************/
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
