/*!
  \file db/driver/postgres/error.c
  
  \brief DBMI - Low Level PostgreSQL database driver - report errors
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Radim Blazek
*/

#include <stdio.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "globals.h"

/* init error message */
void init_error(void)
{
    if (!errMsg) {
	errMsg = (dbString *) G_malloc(sizeof(dbString));
	db_init_string(errMsg);
    }

    db_set_string(errMsg, _("DBMI-Postgres driver error:\n"));
}

/* append error message */
void append_error(const char *msg)
{
    db_append_string(errMsg, msg);
}

/* report error message */
void report_error(void)
{
    db_append_string(errMsg, "\n");
    db_error(db_get_string(errMsg));
}
