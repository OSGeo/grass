/*!
  \file lib/cairodriver/line_width.c

  \brief GRASS cairo display driver - set line width

  (C) 2007-2008 by Lars Ahlzen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
  \author Glynn Clements  
*/

#include "cairodriver.h"

#define MIN_WIDTH 1

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

static double previous_width = -1;

/*!
  \brief Set line width

  \param width line width (double precision)
*/
void Cairo_Line_width(double width)
{
    G_debug(1, "Cairo_Line_width: %f", width);

    width = MAX(MIN_WIDTH, width);
    if (width != previous_width)
	cairo_set_line_width(cairo, width);

    return;
}
