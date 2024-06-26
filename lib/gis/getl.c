/*!
 * \file lib/gis/getl.c
 *
 * \brief GIS Library - Get line of text from file
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <string.h>
#include <stdio.h>
#include <grass/gis.h>

/*!
 * \brief Gets a line of text from a file
 *
 * This routine runs fgets() to fetch a line of text from a file
 * (advancing file pointer) and removes trailing newline.
 *
 * \param buf string buffer to receive read data
 * \param n maximum number of bytes to read
 * \param fd file descriptor structure
 *
 * \return 1 on success
 * \return 0 EOF
 *
 * \see G_getl2()
 */
int G_getl(char *buf, int n, FILE *fd)
{
    return G_getl2(buf, n, fd);
}

/*!
 * \brief Gets a line of text from a file of any pedigree
 *
 * This routine supports
 * text files created on various platforms (UNIX, MacOS9, DOS),
 * i.e. <code>\\n (\\012)</code>, <code>\\r (\\015)</code>, and
 * <code>\\r\\n (\\015\\012)</code> style newlines.
 *
 * Reads in at most <i>n-1</i> characters from stream (the last spot
 * is reserved for the end-of-string NUL) and stores them into the
 * buffer pointed to by <i>buf</i>. Reading stops after an EOF or a
 * newline. New line is not stored in the buffer. At least <i>n</i>
 * bytes must be allocated for the string buffer.
 *
 * \param buf: string buffer to receive read data, at least <i>n</i>
 *             bytes must be allocated
 * \param n: maximum number of bytes to read
 * \param fd: file descriptor structure
 *
 * \return 1 on success
 * \return 0 EOF
 */
int G_getl2(char *buf, int n, FILE *fd)
{
    if (buf == NULL || fd == NULL || n <= 1) {
        return 0;
    }

    if (fgets(buf, n, fd) == NULL) {
        return 0; /* EOF or error */
    }

    /* Remove newline characters (\n, \r\n, or \r) */
    int len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[--len] = '\0';
    }
    if (len > 0 && buf[len - 1] == '\r') {
        buf[--len] = '\0';
    }

    return 1;
}
