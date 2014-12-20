/*!
 * \file lib/gis/trim_dec.c
 *
 * \brief GIS Library - Trim string decimal functions.
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
 * \brief Removes trailing zeros from decimal number.
 *
 * Example: 23.45000 would come back as 23.45
 *
 * \param[in,out] buf
 */
void G_trim_decimal(char *buf)
{
    char *mark;

    /* don't trim e+20 into e+2 */
    if( strchr(buf, 'e') || strchr(buf, 'E') )
	return;

    /* find the . */
    while (*buf != '.')
	if (*buf++ == 0)
	    return;

    mark = buf;
    while (*++buf)
	if (*buf != '0')
	    mark = buf + 1;
    *mark = 0;
}
