
/*!
 * \file lib/gis/writ_zeros.c
 *
 * \brief GIS Library - Write zero functions.
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

#include <unistd.h>
#include <grass/gis.h>


/**
 * \brief Writes <b>n</b> bytes of 9 to file descriptor <b>fd</b>
 *
 * \param[in] fd file descriptor
 * \param[in] n number of bytes to write
 * \return
 */

void G_write_zeros(int fd, size_t n)
{
    char zeros[1024];
    char *z;
    int i;

    if (n <= 0)
	return;

    /* There is a subtle gotcha to be avoided here.
     *
     * i must be an int for the write, but n (size_t) can be long or larger.
     * Must be careful not to cast long to int, hence
     * avoid i = n unless n is within range of int */

    /* fill zeros buffer with zeros */
    if (n > sizeof(zeros))
	i = sizeof(zeros);
    else
	i = n;			/* this is ok here */

    z = zeros;
    while (i--)
	*z++ = 0;

    /* write n zeros to fd */
    while (n > 0) {
	if (n > sizeof(zeros))
	    i = sizeof(zeros);
	else
	    i = n;		/* this is ok here */

	write(fd, zeros, i);
	n -= i;
    }
}
