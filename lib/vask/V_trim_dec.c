
/**
 * \file V_trim_dec.c
 *
 * \brief Display trim functions.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2006
 */

#include <grass/vask.h>


/**
 * \fn int V__trim_decimal (char *buf)
 *
 * \brief Remove trailing zeros from decimal number.
 *
 * For example: 23.45000 would return 23.45.
 *
 * \param[in,out] buf
 * \return always returns 0
 */

void V__trim_decimal(char *buf)
{
    char *mark;

    /* find the . */
    while (*buf != '.')
	if (*buf++ == '\0')
	    return;

    mark = buf;
    while (*++buf)
	if (*buf != '0')
	    mark = buf + 1;

    while (*mark)
	*mark++ = 0;
}
