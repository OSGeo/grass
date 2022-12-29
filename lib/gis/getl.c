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

#include <stdio.h>
#include <grass/gis.h>

/*!
 * \brief Gets a line of text from a file
 *
 * This routine runs fgets() to fetch a line of text from a file
 * (advancing file pointer) and removes trailing newline. fgets() does
 * not recognize '<code>\\r</code>' as an EOL and will read past * it.
 *
 * \param buf string buffer to receive read data
 * \param n maximum number of bytes to read
 * \param fd file descriptor structure
 *
 * \return 1 on success
 * \return 0 EOF
 */
int G_getl(char *buf, int n, FILE * fd)
{
    if (!fgets(buf, n, fd))
	return 0;

    for (; *buf && *buf != '\n'; buf++) ;
    *buf = 0;

    return 1;
}

/*!
 * \brief Gets a line of text from a file of any pedigree
 *
 * This routine is like G_getl() but is more portable.  It supports
 * text files created on various platforms (UNIX, MacOS9, DOS),
 * i.e. <code>\\n (\\012)</code>, <code>\\r (\\015)</code>, and
 * <code>\\r\\n (\\015\\012)</code> style newlines.
 * 
 * 
 * Reads in at most <i>n-1</i> characters from stream (the last spot
 * is reserved for the end-of-string NUL) and stores them into the
 * buffer pointed to by <i>buf</i>. Reading stops after an EOF or a
 * newline.  New line is not stored in the buffer. At least <i>n</i>
 * must be allocated for the string buffer.
 *
 * \param buf: string buffer to receive read data, at least <i>n</i> must be allocated
 * \param n: maximum number of bytes to read
 * \param fd: file descriptor structure
 *
 * \return 1 on success
 * \return 0 EOF
 */
int G_getl2(char *buf, int n, FILE * fd)
{
    int i = 0;
    int c;
    int ret = 1;

    while (i < n - 1) {
	c = fgetc(fd);

	if (c == EOF) {
	    if (i == 0) {	/* Read correctly (return 1) last line in file without '\n' */
		ret = 0;
	    }
	    break;
	}

	if (c == '\n')
	    break;		/* UNIX */

	if (c == '\r') {	/* DOS or MacOS9 */
	    if ((c = fgetc(fd)) != EOF) {
		if (c != '\n') {	/* MacOS9 - we have to return the char to stream */
		    ungetc(c, fd);
		}
	    }
	    break;
	}

	buf[i] = c;

	i++;
    }
    buf[i] = '\0';

    return ret;
}
