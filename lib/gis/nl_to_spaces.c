// SPDX-License-Identifier: GPL-2.0-or-later
#include <grass/gis.h>

void G_newlines_to_spaces(char *s)
{
    while (*s) {
        if (*s == '\n')
            *s = ' ';
        s++;
    }
}
