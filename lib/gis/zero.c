/**
 * \file zero.c
 *
 * \brief Zeroing functions.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2006
 */

#include <string.h>
#include <grass/gis.h>


/**
 * \fn int G_zero (void *buf, int i)
 *
 * \brief Zero out a buffer, <b>buf</b>, of length <b>i</b>.
 *
 * \param[in,out] buf
 * \param[in] i number of bytes to be zeroed
 * \return always returns 0
 */

int G_zero (void *buf, int i)
{
    memset(buf, 0, i);

    return 0;
}
