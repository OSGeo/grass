/*!
 * \file lib/gis/asprintf.c
 *
 * \brief GIS Library - GRASS implementation of asprintf().
 *
 * Eric G. Miller - Thu, 2 May 2002 17:51:54 -0700
 *
 * Rewritten by Glynn Clements, Sat, 6 Feb 2010
 * Assumes that vsnprintf() is available
 *
 * (C) 2002-2014 by the GRASS Development Team
 * (C) 2010 by Glynn Clements
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 */

#define _GNU_SOURCE /* enable asprintf */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <grass/gis.h>

#ifndef G_asprintf

/**
 * \brief Safe replacement for <i>asprintf()</i>.
 *
 * Allocate a string large enough to hold the new output, including the
 * terminating NULL, and return the number of characters printed. The
 * pointer out is set to the output string and should be passed to
 * <i>G_free()</i> to release the allocated storage when it is no longer
 * needed.
 *
 * \param[out] out
 * \param[in] fmt
 * \param ap
 * \return number of bytes written
 */
int G_vasprintf(char **out, const char *fmt, va_list ap)
{
#ifdef HAVE_ASPRINTF
    return vasprintf(out, fmt, ap);
#else
    va_list aq;
    char *buf;
    size_t size;
    int count;

    va_copy(aq, ap);
    /* determine a sufficient size */
    count = vsnprintf(NULL, 0, fmt, ap);
    /* count holds the number of printable characters; size must be +1 for '\0'
     */
    buf = G_malloc(size = ++count);
    count = vsnprintf(buf, size, fmt, aq);
    va_end(aq);
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

/**
 * \brief Reallocating version of <i>asprintf()</i>.
 *
 * Reallocate a string large enough to hold the output, including the
 * terminating NULL, and return the number of characters printed.
 * Contrary to <i>G_asprintf()</i>, any existing buffer pointed to by
 * out of size osize is used to hold the output and enlarged if
 * necessary. This is useful when <i>G_rasprintf</i> is called many
 * times in a loop.
 *
 * \param[out] out
 * \param[out] size
 * \param[in] fmt
 * \param ap
 * \return number of bytes written
 */
int G_rasprintf(char **out, size_t *size, const char *fmt, ...)
{
    va_list ap;
    char *buf = *out;
    size_t osize = *size;
    int count;

    va_start(ap, fmt);
    count = vsnprintf(buf, osize, fmt, ap);
    va_end(ap);

    if (count >= osize) {
        /* count holds the number of printable characters; osize must be +1 for
         * '\0' */
        buf = G_realloc(buf, osize = ++count);
        va_start(ap, fmt);
        count = vsnprintf(buf, osize, fmt, ap);
        va_end(ap);
    }
    *out = buf;
    *size = osize;

    return count;
}
