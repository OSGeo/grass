
/*!
 * \file lib/gis/done_msg.c
 *
 * \brief GIS Library - Done message functions.
 *
 * (C) 2001-2014 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2014
 */

#include <stdarg.h>

#include <grass/gis.h>
#include <grass/glocale.h>

/**
 * \brief Print a final message.
 *
 * \param[in] msg string.  Cannot be NULL.
 * \return
 */

void G_done_msg(const char *msg, ...)
{
    char buffer[2000];
    va_list ap;

    va_start(ap, msg);
    vsprintf(buffer, msg, ap);
    va_end(ap);

    G_message(_("%s complete. %s"), G_program_name(), buffer);
}
