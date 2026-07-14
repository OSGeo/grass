/*!
   \file lib/cairodriver/box.c

   \brief GRASS cairo display driver - draw box

   SPDX-FileCopyrightText: 2007-2008 by Lars Ahlzen and the GRASS Development
   Team

   SPDX-License-Identifier: GPL-2.0-or-later.

   \author Lars Ahlzen <lars ahlzen.com> (original contributor)
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
