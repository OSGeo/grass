/**
 * \file date.c
 *
 * \brief Date functions.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2006
 */

#include <time.h>
#include <grass/gis.h>


/**
 * \fn char *G_date (void)
 *
 * \brief Current date and time.
 *
 * Returns a pointer to a string which is the current date and time. The 
 * format is the same as that produced by the UNIX <i>date</i> command.
 *
 * \return Pointer to a string holding date/time
 */

char *
G_date(void)
{
    time_t clock;
    struct tm *local;
    char *date;
    char *d;

    time(&clock);

    local = localtime(&clock);
    date = asctime(local);
    for (d = date; *d; d++)
	if (*d == '\n')
	    *d = 0;

    return date;
}
