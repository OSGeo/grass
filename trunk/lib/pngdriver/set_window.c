/*!
  \file lib/pngdriver/set_window.c

  \brief GRASS png display driver - set window

  (C) 2007-2014 by Glynn Clements and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Per Henrik Johansen (original contributor)
  \author Glynn Clements
*/

#include <math.h>
#include "pngdriver.h"

/*!
  \brief Set window
  
  \param t,b,l,r top, bottom, left, right
*/
void PNG_Set_window(double t, double b, double l, double r)
{
    png.clip_top  = t > 0          ? t : 0;
    png.clip_bot  = b < png.height ? b : png.height;
    png.clip_left = l > 0          ? l : 0;
    png.clip_rite = r < png.width  ? r : png.width;
}
