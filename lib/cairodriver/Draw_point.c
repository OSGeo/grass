/*!
  \file cairodriver/Draw_point.c

  \brief GRASS cairo display driver - draw point

  (C) 2007-2008 by Lars Ahlzen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
  \author Glynn Clements  
*/

#include "cairodriver.h"

#define POINTSIZE 1.0
#define HALFPOINTSIZE (0.5*POINTSIZE)

/*!
  \brief Draw point

  \param x,y point coordinates
*/
void Cairo_draw_point(double x, double y)
{
    G_debug(3, "Cairo_draw_point: %f %f", x, y);

    cairo_rectangle(cairo,
		    x - HALFPOINTSIZE, y - HALFPOINTSIZE,
		    POINTSIZE, POINTSIZE);
    cairo_fill(cairo);
    ca.modified = 1;

    return;
}
