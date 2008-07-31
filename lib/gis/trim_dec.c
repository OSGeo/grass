/**
 * \file trim_dec.c
 *
 * \brief GIS Library - Trim string decimal functions.
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


/**
 * \brief Removes trailing zeros from decimal number.
 *
 * Example: 23.45000 would come back as 23.45
 *
 * \param[in,out] buf
 * \return always returns 0
 */

int G_trim_decimal (char *buf)
{
    char *mark;

    /* find the . */
    while (*buf != '.')
	if (*buf++ == 0)
	    return 0;

    mark = buf;
    while (*++buf)
	if (*buf != '0')
	    mark = buf+1;
    *mark = 0;

    return 0;
}
