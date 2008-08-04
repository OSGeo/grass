#include <stdio.h>
#include <grass/gis.h>

/*!
 * \brief gets a line of text from a file
 *
 * This routine runs fgets() to fetch a line of text from a file (advancing
 * file pointer) and removes trailing newline. fgets() does not recognize
 * '<code>\\r</code>' as an EOL and will read past it.<br>
 *
 * \param buf: string buffer to receive read data
 * \param n: maximum number of bytes to read
 * \param fd: file descriptor structure
 * \return 1 if ok, 0 if EOF
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
 * \brief gets a line of text from a file of any pedigree
 *
 * This routine is like G_getl() but is more portable. 
 * It supports text files created on various platforms (UNIX, MacOS9, DOS),
 * i.e. <code>\\n (\\012)</code>, <code>\\r (\\015)</code>, and 
 * <code>\\r\\n (\\015\\012)</code> style newlines.
 * <br>
 * <br>
 * Reads in at most <b>n-1</b> characters from stream (the last spot is
 * reserved for the end-of-string NUL) and stores them into the buffer
 * pointed to by <b>buf</b>. Reading stops after an EOF or a newline.
 * New line is not stored in the buffer. At least <b>n</b> must be allocated
 * for the string buffer.<br>
 *
 * \param buf: string buffer to receive read data, at least <b>n</b> must
 *   be allocated
 * \param n: maximum number of bytes to read
 * \param fd: file descriptor structure
 * \return 1 if ok, 0 if EOF
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
