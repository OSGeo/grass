/*!
  \file lib/pngdriver/line_width.c

  \brief GRASS png display driver - set line width

  (C) 2003-2014 by Per Henrik Johansen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Per Henrik Johansen (original contributor)
  \author Glynn Clements  
*/

#include <math.h>
#include "pngdriver.h"

/*!
  \brief Set line width

  \param width line width (double precision)
*/
void PNG_Line_width(double width)
{
    png.linewidth = (width < 0 ? 0 : (int) floor(width + 0.5));
}
