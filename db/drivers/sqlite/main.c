
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
#include <grass/dbmi.h>
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
    int min;
    static int last_min = -1;

    /* do something here while waiting? */
    if (n_calls > 0 && last_min > -1) {
	time(&curr_time);
	min = (curr_time - start_time) / 60;
	if (min > 1 && min > last_min) {
	    last_min = min;
	    G_debug(3, "Already waiting for %d minutes...", min);
	}
    }
    else {
	time(&start_time);
	last_min = 0;
    }

    return 1;
}
