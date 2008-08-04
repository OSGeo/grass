
/**
 * \file unctrl.c
 *
 * \brief GIS Library - Handles control characters.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2006
 */

#include <stdio.h>
#include <grass/gis.h>


/**
 * \brief Printable version of control character.
 *
 * This routine returns a pointer to a string which contains an
 * English-like representation for the character <b>c</b>. This is useful for
 * nonprinting characters, such as control characters. Control characters are
 * represented by ctrl-C, e.g., control A is represented by ctrl-A. 0177 is
 * represented by DEL/RUB. Normal characters remain unchanged.
 *
 * \param[in] int c
 * \return char * pointer to string containing English-like representation for character <b>c</b>
 */

char *G_unctrl(int c)
{
    static char buf[20];

    if (c < ' ')
	sprintf(buf, "ctrl-%c", c | 0100);
    else if (c < 0177)
	sprintf(buf, "%c", c);
    else if (c == 0177)
	sprintf(buf, "DEL/RUB");
    else
	sprintf(buf, "Mctrl-%c", (c & 77) | 0100);

    return buf;
}
