
/**
 * \file asprintf.c
 *
 * \brief GIS Library - GRASS implementation of asprintf().
 *
 * Eric G. Miller - Thu, 2 May 2002 17:51:54 -0700
 *
 * Rewritten by Glynn Clements, Sat, 6 Feb 2010
 * Assumes that vsnprintf() is available
 *
 * (C) 2001-2008 by the GRASS Development Team
 * (C) 2010 by Glynn Clements
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 */

#define _GNU_SOURCE		/* enable asprintf */
#include <grass/config.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <grass/gis.h>

#ifndef G_asprintf

/**
 * \brief Safe replacement for <i>asprintf()</i>.
 *
 * Allocate a string large enough to hold the new output, including the 
 * terminating NULL, and returns a pointer to the first parameter. The 
 * pointer should be passed to <i>G_free()</i> to release the allocated 
 * storage when it is no longer needed.
 *
 * \param[out] out
 * \param[in] fmt
 * \return number of bytes written
 */

int G_vasprintf(char **out, const char *fmt, va_list ap)
{
#ifdef HAVE_ASPRINTF
    return vasprintf(out, fmt, ap);
#else
    size_t size = strlen(fmt) + 50;
    char *buf = G_malloc(size);
    int count;

    for (;;) {
	count = vsnprintf(buf, size, fmt, ap);
	if (count >= 0 && count < size)
	    break;
	size *= 2;
	buf = G_realloc(buf, size);
    }

    buf = G_realloc(buf, count + 1);
    *out = buf;

    return count;
#endif /* HAVE_ASPRINTF */
}

int G_asprintf(char **out, const char *fmt, ...)
{
    va_list ap;
    int count;

    va_start(ap, fmt);
    count = G_vasprintf(out, fmt, ap);
    va_end(ap);

    return count;
}

#endif /* G_asprintf */
