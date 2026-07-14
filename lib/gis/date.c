/*!
 * \file lib/gis/date.c
 *
 * \brief GIS Library - Date functions.
 *
 * SPDX-FileCopyrightText:  2001-2009 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
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
