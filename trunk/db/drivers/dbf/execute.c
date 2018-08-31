
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
#include <grass/glocale.h>
#include "globals.h"
#include "proto.h"

int db__driver_execute_immediate(dbString * sql)
{
    char *s;
    int ret;

    s = db_get_string(sql);

    ret = execute(s, NULL);

    if (ret == DB_FAILED) {
	db_d_append_error(_("Unable to execute statement."));
	db_d_report_error();
	return DB_FAILED;
    }

    return DB_OK;
}
