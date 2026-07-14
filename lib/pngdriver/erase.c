/*!
   \file lib/pngdriver/erase.c

   \brief GRASS png display driver - erase screen

   SPDX-FileCopyrightText: 2003-2014 by Per Henrik Johansen and the GRASS
   Development Team

   SPDX-License-Identifier: GPL-2.0-or-later.

   \author Per Henrik Johansen (original contributor)
   \author Glynn Clements
 */

#include "pngdriver.h"

/*!
   \brief Erase screen
 */
void PNG_Erase(void)
{
    int n = png.width * png.height;
    int i;

    for (i = 0; i < n; i++)
        png.grid[i] = png.background;

    png.modified = 1;
}
