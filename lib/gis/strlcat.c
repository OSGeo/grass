/*!
 * \file lib/gis/strlcat.c
 *
 * \brief GIS Library - GRASS implementation of strlcat().
 *
 * If available, G_strlcat() calls system strlcat(), otherwise it uses
 * implementation by Todd C. Miller of OpenBSD.
 *
 * Addition to GRASS by Nicklas Larsson, 2024
 *
 * Original OpenBSD implementation notes:
 *
 * Copyright (c) 1998, 2015 Todd C. Miller <millert@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stddef.h>
#include <string.h>

static size_t G__strlcat(char *restrict dst, const char *restrict src,
                         size_t dsize);

/**
 * \brief Size-bounded string concatenation
 *
 * Appends string src to the end of dst. It will append at most
 * dstsize - strlen(dst) - 1 characters. It will then NUL-terminate, unless
 * dstsize is 0 or the original dst string was longer than dstsize (in practice
 * this should not happen as it means that either dstsize is incorrect or that
 * dst is not a proper string).
 *
 * If the src and dst strings overlap, the behavior is undefined.
 * This function is a safer alternative to strncat.
 *
 * \param[out] dst Pointer to the destination buffer. Must be a NUL-terminated
 *                 C string.
 * \param[in] src Pointer to the source string, which will be appended. Must
 *                be a NUL-terminated C string.
 * \param[in] dsize The size of the destination buffer.
 *
 * \return The total length of the string src, which was attempted to be
 *         created (the initial length of dst plus the length of src, not
 *         including the terminating NUL character). If the return value
 *         is >= dsize, truncation occurred.
 */
size_t G_strlcat(char *dst, const char *src, size_t dsize)
{
#ifdef HAVE_STRLCAT
    return strlcat(dst, src, dsize);
#else
    return G__strlcat(dst, src, dsize);
#endif
}

static size_t G__strlcat(char *restrict dst, const char *restrict src,
                         size_t dsize)
{
    const char *odst = dst;
    const char *osrc = src;
    size_t n = dsize;
    size_t dlen;

    /* Find the end of dst and adjust bytes left but don't go past end. */
    while (n-- != 0 && *dst != '\0')
        dst++;
    dlen = dst - odst;
    n = dsize - dlen;

    if (n-- == 0)
        return (dlen + strlen(src));
    while (*src != '\0') {
        if (n != 0) {
            *dst++ = *src;
            n--;
        }
        src++;
    }
    *dst = '\0';

    return (dlen + (src - osrc)); /* count does not include NUL */
}
