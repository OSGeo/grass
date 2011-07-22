/*!
  \file db/drivers/error.c
  
  \brief Low level OGR SQL driver
 
  (C) 2004-2009 by the GRASS Development Team
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Radim Blazek
  \author Some updates by Martin Landa <landa.martin gmail.com>
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include <ogr_api.h>

#include "globals.h"
#include "proto.h"

/*!
  \brief Init error message
*/
void init_error(void)
{
    if (!errMsg) {
	errMsg = (dbString *) G_malloc(sizeof(dbString));
	db_init_string(errMsg);
    }

    db_set_string(errMsg, _("DBMI-OGR driver error:\n"));
}

/*!
  \brief Append error message

  \param fmt formatted string
*/
void append_error(const char *fmt, ...)
{
    FILE *fp = NULL;
    char *work = NULL;
    int count = 0;
    va_list ap;

    va_start(ap, fmt);
    if ((fp = tmpfile())) {
	count = vfprintf(fp, fmt, ap);
	if (count >= 0 && (work = G_calloc(count + 1, 1))) {
	    rewind(fp);
	    fread(work, 1, count, fp);
	    db_append_string(errMsg, work);
	    G_free(work);
	}
	fclose(fp);
    }
    va_end(ap);
}

/*!
  \brief Report errors
*/
void report_error(void)
{
    db_append_string(errMsg, "\n");
    db_error(db_get_string(errMsg));
}
