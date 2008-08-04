
/**
 * \file error.c
 *
 * \brief Low level driver error reporting function.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Radim Blazek
 *
 * \date 2000-2007
 */

#include <stdio.h>
#include <grass/dbmi.h>
#include <grass/gis.h>
#include "odbc.h"
#include "globals.h"


/**
 * \fn void report_error (char *err)
 *
 * \brief Reports database driver error.
 *
 * \param[in] err error message
 */

void report_error(char *err)
{
    char *msg = NULL;

    G_asprintf(&msg, "DBMI-ODBC driver error: %s", err);
    db_error(msg);
    G_free(msg);
}
