/***********************************************************
*
* MODULE:       SQLite driver 
*   	    	
* AUTHOR(S):    Radim Blazek
*
* COPYRIGHT:    (C) 2005 by the GRASS Development Team
*
* This program is free software under the GNU General Public
* License (>=v2). Read the file COPYING that comes with GRASS
* for details.
*
**************************************************************/
#define MAIN
#include <stdlib.h>
#include <grass/dbmi.h>
#include "globals.h"
#include "dbdriver.h"

int main (int argc, char *argv[])

{
	init_dbdriver();
	exit (db_driver (argc, argv));
}
