
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

#include <grass/dbmi.h>
#include "globals.h"
#include "proto.h"

int db__driver_list_tables(dbString ** tlist, int *tcount, int system)
{
    dbString *list;
    int i;

    *tlist = NULL;
    *tcount = 0;

    list = db_alloc_string_array(db.ntables);
    if (list == NULL && db.ntables > 0)
	return DB_FAILED;

    for (i = 0; i < db.ntables; i++) {
	if (db_set_string(&list[i], (char *)db.tables[i].name) != DB_OK) {
	    return DB_FAILED;
	}
    }


    *tlist = list;
    *tcount = db.ntables;
    return DB_OK;
}
