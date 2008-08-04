
/**
 * \file zero.c
 *
 * \brief GIS Library - Zeroing functions.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2008
 */

#include <string.h>
#include <grass/gis.h>


/**
 * \brief Zero out a buffer, <b>buf</b>, of length <b>i</b>.
 *
 * \param[in,out] buf
 * \param[in] i number of bytes to be zeroed
 * \return always returns 0
 */

int G_zero(void *buf, int i)
{
    memset(buf, 0, i);

    return 0;
}
