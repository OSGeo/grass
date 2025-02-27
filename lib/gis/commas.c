/*!
 * \file lib/gis/commas.c
 *
 * \brief GIS Library - Comma string functions.
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

#include <string.h>
#include <grass/gis.h>

/**
 * \brief Inserts commas into a number string.
 *
 * Examples:
 *
 *  - 1234567    becomes 1,234,567
 *  - 1234567.89 becomes 1,234,567.89
 *  - 12345      becomes 12,345
 *  - 1234       stays   1234
 *
 * <b>Note:</b> Does not work with negative numbers.
 *
 * \param[in,out] buf string
 * \return 1 if no commas inserted
 * \return 0 if commas inserted
 */

int G_insert_commas(char *buf)
{
    char number[100];
    int i, len;
    int comma;

    while (*buf == ' ')
        buf++;
    G_strlcpy(number, buf, sizeof(number));
    for (len = 0; number[len]; len++)
        if (number[len] == '.')
            break;
    if (len < 5)
        return 1;

    i = 0;
    if ((comma = len % 3)) {
        while (i < comma)
            *buf++ = number[i++];
        *buf++ = ',';
    }

    for (comma = 0; number[i]; comma++) {
        if (number[i] == '.')
            break;
        if (comma && (comma % 3 == 0))
            *buf++ = ',';
        *buf++ = number[i++];
    }
    while (number[i])
        *buf++ = number[i++];
    *buf = 0;

    return 0;
}

/**
 * \brief Removes commas from number string.
 *
 * Examples:
 *  - 1,234,567    becomes 1234567<br>
 *  - 1,234,567.89 becomes 1234567.89<br>
 *  - 12,345      becomes 12345<br>
 *  - 1234       stays   1234
 *
 * \param[in,out] buf string
 * \return
 */

void G_remove_commas(char *buf)
{
    char *b;

    for (b = buf; *b; b++)
        if (*b != ',')
            *buf++ = *b;

    *buf = 0;
}
