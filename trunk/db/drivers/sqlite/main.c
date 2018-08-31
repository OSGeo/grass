
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

#include <stdlib.h>
#include <time.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "globals.h"
#include "dbdriver.h"

sqlite3 *sqlite;

int main(int argc, char *argv[])
{
    init_dbdriver();
    exit(db_driver(argc, argv));
}

int sqlite_busy_callback(void *arg, int n_calls)
{
    static time_t start_time = 0;
    time_t curr_time;
    int sec;
    static int last_sec = -1;

    G_debug(4, "sqlite_busy_callback()");

    /* do something here while waiting? */
    if (n_calls > 0 && last_sec > -1) {
	time(&curr_time);
	sec = (curr_time - start_time);
	if (sec > 1 && sec > last_sec && sec % 10 == 0) {
	    last_sec = sec;
	    G_warning(_("Busy SQLITE db, already waiting for %d seconds..."), sec);
	}
    }
    else {
	time(&start_time);
	last_sec = 0;
    }

    return 1;
}
