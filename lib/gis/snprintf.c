
/*!
 * \file lib/gis/snprintf.c
 *
 * \brief GIS Library - snprintf() clone functions.
 *
 *
 * \todo if needed, implement alternative versions for portability.
 *  potential code source:
 *   - http://www.ijs.si/software/snprintf/
 *   - openssh's snprintf() implementation: bsd-snprintf.c
 *
 * (C) 2001-2014 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Markus Neteler
 *
 * \date 2006-2008
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <assert.h>
#include <grass/gis.h>

/**
 * \brief snprintf() clone.
 *
 * <b>Note:</b> The use of <i>snprintf()</i>/<i>G_snprintf()</i> is 
 * discouraged in favour of calculating how long the string will be and 
 * allocating enough memory!
 *
 * \param[in] str input string
 * \param[in] size length of string
 * \param[in] fmt
 * \return numer of chars written
 */

int G_snprintf(char *str, size_t size, const char *fmt, ...)
{
    va_list ap;
    int count;

    va_start(ap, fmt);
    count = vsnprintf(str, size, fmt, ap);
    va_end(ap);

    /* Windows' vsnprintf() doesn't always NUL-terminate the buffer */
    if (count == size)
	str[--count] = '\0';

    return count;
}
