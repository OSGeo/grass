
/*****************************************************************************
*
* MODULE:       OGR driver 
*   	    	
* AUTHOR(S):    Radim Blazek
*               Some updates by Martin Landa <landa.martin gmail.com>
*
* PURPOSE:      DB driver for OGR sources     
*
* COPYRIGHT:    (C) 2004-2009 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/

#include <stdlib.h>

#include <grass/dbmi.h>

#include <ogr_api.h>

#include "globals.h"
#include "dbdriver.h"

OGRDataSourceH hDs;
dbString *errMsg = NULL;	/* error message */

int main(int argc, char *argv[])
{
    init_dbdriver();
    exit(db_driver(argc, argv));
}
