
/*****************************************************************************
*
* MODULE:       DBF driver 
*   	    	
* AUTHOR(S):    Radim Blazek
*
* PURPOSE:      Simple driver for reading and writing dbf files     
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
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

    /* Do not call G_getenv() nor other functions reading GISRC here! It may be that grass variables are
     * not available here, but will be set in db_driver() */

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
