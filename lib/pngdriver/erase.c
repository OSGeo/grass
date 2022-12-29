/*!
  \file lib/pngdriver/erase.c

  \brief GRASS png display driver - erase screen

  (C) 2003-2014 by Per Henrik Johansen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
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
