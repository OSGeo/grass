/*!
 * \file lib/gis/strlcpy.c
 *
 * \brief GIS Library - GRASS implementation of strlcpy().
 *
 * Loïc Bartoletti - 2024-07-25
 *
 * Copyright (c) 1998, 2015 Todd C. Miller <Todd.Miller@courtesan.com>
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

static size_t G__strlcpy(char *restrict dst, const char *restrict src,
                         size_t dsize);

/**
 * \brief Safe string copy function.
 *
 * Copy string src to buffer dst of size dsize. At most dsize-1
 * characters will be copied. Always NUL terminates (unless dsize == 0).
 * This function is a safer alternative to strncpy.
 *
 * \param[out] dst Pointer to the destination buffer.
 * \param[in] src Pointer to the source string. Must be a NUL-terminated C
 *                string.
 * \param[in] dsize The size of the destination buffer.
 *
 * \return The total length of the string src (not including the terminating
 *         NUL character). If the return value is >= dsize, truncation occurred.
 *
 * \note If truncation occurred, the return value is the length of the string
 *       that would have been created if enough space had been available.
 *
 * \warning This function does not pad the destination buffer with NUL bytes
 *          if the source string is shorter than dsize-1 bytes, unlike strncpy.
 *
 * \warning The src string must be a valid NUL-terminated C string. Passing an
 *          unterminated string may result in buffer overrun.
 */
size_t G_strlcpy(char *dst, const char *src, size_t dsize)
{
#ifdef HAVE_STRLCPY
    return strlcpy(dst, src, dsize);
#else
    return G__strlcpy(dst, src, dsize);
#endif
}

static size_t G__strlcpy(char *restrict dst, const char *restrict src,
                         size_t dsize)
{
    const char *osrc = src;
    size_t nleft = dsize;

    /* Copy as many bytes as will fit. */
    if (nleft != 0) {
        while (--nleft != 0) {
            if ((*dst++ = *src++) == '\0')
                break;
        }
    }

    /* Not enough room in dst, add NUL and traverse rest of src. */
    if (nleft == 0) {
        if (dsize != 0)
            *dst = '\0'; /* NUL-terminate dst */
        while (*src++)
            ;
    }

    return (src - osrc - 1); /* count does not include NUL */
}
