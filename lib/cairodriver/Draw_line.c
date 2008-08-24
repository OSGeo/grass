/*!
  \file cairodriver/Draw_line.c

  \brief GRASS cairo display driver - colors management

  (C) 2007-2008 by Lars Ahlzen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
  \author Glynn Clements  
*/

#include "cairodriver.h"

/*!
  \brief Draw line

  \param x1,y1,x2,y2 line nodes
*/
void Cairo_draw_line(double x1, double y1, double x2, double y2)
{
    G_debug(3, "Cairo_draw_line: %f %f %f %f", x1, y1, x2, y2);

    if (x1 == x2 && y1 == y2) {
	/* don't draw degenerate lines */
	G_debug(3, "Skipping zero-length line");
	return;
    }

    cairo_move_to(cairo, x1, y1);
    cairo_line_to(cairo, x2, y2);
    cairo_stroke(cairo);
    ca.modified = 1;

    return;
}
