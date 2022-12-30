/*!
   \file lib/cairodriver/line_width.c

   \brief GRASS cairo display driver - set line width

   (C) 2007-2008 by Lars Ahlzen and the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

<<<<<<< HEAD
   \author Lars Ahlzen <lars ahlzen.com> (original contributor)
=======
   \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
   \author Glynn Clements
 */

#include <grass/gis.h>
#include "cairodriver.h"

#define MIN_WIDTH 1

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
