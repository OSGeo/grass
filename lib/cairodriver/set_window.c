/*!
   \file lib/cairodriver/set_window.c

   \brief GRASS cairo display driver - set window

   (C) 2007-2008 by Lars Ahlzen and the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

<<<<<<< HEAD
   \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
=======
<<<<<<< HEAD
<<<<<<< HEAD
   \author Lars Ahlzen <lars ahlzen.com> (original contributor)
=======
   \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
   \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main
   \author Glynn Clements
 */

#include "cairodriver.h"

/*!
   \brief Set window

   \param t,b,l,r top, bottom, left, right
 */
void Cairo_Set_window(double t, double b, double l, double r)
{
    G_debug(1, "Cairo_Set_window: %f %f %f %f", t, b, l, r);

    cairo_reset_clip(cairo);
    cairo_rectangle(cairo, l, t, r - l, b - t);
    cairo_clip(cairo);

    return;
}
