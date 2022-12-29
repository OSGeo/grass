
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
