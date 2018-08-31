/*!
  \file lib/pngdriver/color.c

  \brief GRASS png display driver - PNG_color_rgb

  (C) 2003-2014 by Per Henrik Johansen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Per Henrik Johansen (original contributor)
  \author Glynn Clements  
*/

#include <grass/gis.h>
#include "pngdriver.h"

/*!
  \brief  Identify a color 

  Identify a color that has been set in the reset_color() (found in Reset_clr.c
  file in this directory).  Subsequent graphics calls will use this color.
 
  Called by:
       Color() in ../lib/Color.c

  \param r red color value
  \param g green color value
  \param b blue color value
*/

void PNG_color_rgb(int r, int g, int b)
{
    png.current_color = png_get_color(r, g, b, 0);
}
