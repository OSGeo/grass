/*!
 * \file lib/gis/date.c
 *
 * \brief GIS Library - Date functions.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <time.h>
#include <grass/gis.h>

/*!
 * \brief Current date and time.
 *
 * Returns a pointer to a string which is the current date and
 * time. The format is the same as that produced by the UNIX
 * <tt>date</tt> command.
 *
 * \return pointer to a string holding date/time
 */
const char *G_date(void)
{
    static int initialized;
    static char *date;
    time_t clock;
    struct tm *local;
    char *tdate;
    char *d;

    if (G_is_initialized(&initialized))
	return date;

    time(&clock);

    local = localtime(&clock);
    tdate = asctime(local);
    for (d = tdate; *d; d++)
	if (*d == '\n')
	    *d = 0;

    date = G_store(tdate);

    G_initialize_done(&initialized);

    return date;
}
