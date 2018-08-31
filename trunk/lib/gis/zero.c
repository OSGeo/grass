/*!
 * \file lib/gis/zero.c
 *
 * \brief GIS Library - Zeroing functions.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <string.h>
#include <grass/gis.h>

/*!
 * \brief Zero out a buffer, <b>buf</b>, of length <b>i</b>.
 *
 * \param[out] buf
 * \param i number of bytes to be zeroed
 */
void G_zero(void *buf, int i)
{
    memset(buf, 0, i);
}
