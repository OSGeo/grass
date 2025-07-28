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

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

/**
 * \brief Writes <b>n</b> bytes of zero to file descriptor <b>fd</b>
 *
 * \param[in] fd file descriptor
 * \param[in] n number of bytes to write
 * \return
 */
void G_write_zeros(int fd, size_t n)
{
    char zeros[1024];
    char *z;
    size_t i;

    if (n <= 0)
        return;

    /* fill zeros buffer with zeros */
    if (n > sizeof(zeros))
        i = sizeof(zeros);
    else
        i = n;

    z = zeros;
    while (i--)
        *z++ = 0;

    /* write n zeros to fd */
    while (n > 0) {
        if (n > sizeof(zeros))
            i = sizeof(zeros);
        else
            i = n;

        if (write(fd, zeros, i) < 0)
            G_fatal_error(_("File writing error in %s() %d:%s"), __func__,
                          errno, strerror(errno));
        n -= i;
    }
}
