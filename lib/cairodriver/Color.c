/*!
  \file cairodriver/Color.c

  \brief GRASS cairo display driver - colors management

  (C) 2007-2008 by Lars Ahlzen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
  \author Glynn Clements 
*/  

#include "cairodriver.h"

/*!
  \brief Set source color (opaque)
  
  This color will then be used for any subsequent drawing operation
  until a new source pattern is set.
  
  \param color value
*/
void Cairo_color(int color)
{
    static int previous_color = 0x7FFFFFFF;

    G_debug(3, "Cairo_color: %d", color);

    if (color != previous_color) {
	int r = (color >> 16) & 0xFF;
	int g = (color >> 8) & 0xFF;
	int b = (color >> 0) & 0xFF;

	cairo_set_source_rgba(cairo, CAIROCOLOR(r), CAIROCOLOR(g),
			      CAIROCOLOR(b), 1.0);
	previous_color = color;

	G_debug(3, "Set color to: %g %g %g", CAIROCOLOR(r), CAIROCOLOR(g),
		CAIROCOLOR(b));
    }
}

/*!
  \brief Get color value
  
  \param r,g,b red,green,blue

  \return value
*/
int Cairo_lookup_color(int r, int g, int b)
{
    G_debug(3, "Cairo_lookup_color: %d %d %d", r, g, b);

    return (r << 16) + (g << 8) + (b << 0);
}
