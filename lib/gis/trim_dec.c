/*!
 * \file lib/gis/trim_dec.c
 *
 * \brief GIS Library - Trim string decimal functions.
 *
 * SPDX-FileCopyrightText: 2001-2009 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
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
    if (strchr(buf, 'e') || strchr(buf, 'E'))
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
