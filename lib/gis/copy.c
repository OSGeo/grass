
/**
 * \file copy.c
 *
 * \brief GIS Library - Memory copy functions.
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

#include <grass/gis.h>
#include <string.h>

/**
 * \brief Copies <b>n</b> bytes starting at address <b>b</b> into 
 * address <b>a</b>.
 *
 * \param[out] a destination (to)
 * \param[in]  b source (from)
 * \param[in]  n number of bytes to copy
 *
 * \return always returns 0
 */
int G_copy(void *a, const void *b, int n)
{
    memcpy(a, b, n);
    return 0;
}
