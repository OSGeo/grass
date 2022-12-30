/*!
   \file lib/cairodriver/box.c

   \brief GRASS cairo display driver - draw box

   (C) 2007-2008 by Lars Ahlzen and the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

<<<<<<< HEAD
<<<<<<< HEAD
   \author Lars Ahlzen <lars ahlzen.com> (original contributor)
=======
   \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
   \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
   \author Glynn Clements
 */

#include "cairodriver.h"

/*!
   \brief Draw a (filled) rectangle

   \param x1,y1,x2,y2 rectangle coordinates
 */
void Cairo_Box(double x1, double y1, double x2, double y2)
{
    G_debug(3, "Cairo_Box %f %f %f %f\n", x1, y1, x2, y2);

    cairo_rectangle(cairo, x1, y1, x2 - x1, y2 - y1);
    cairo_fill(cairo);
    ca.modified = 1;

    return;
}
