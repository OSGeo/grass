/*!
   \file lib/cairodriver/color.c

   \brief GRASS cairo display driver - colors management

   SPDX-FileCopyrightText: 2007-2008 Lars Ahlzen
   SPDX-FileCopyrightText: Other GRASS authors
   SPDX-License-Identifier: GPL-2.0-or-later

   \author Lars Ahlzen <lars ahlzen.com> (original contributor)
   \author Glynn Clements
 */

#include "cairodriver.h"

/*!
   \brief Set source color (opaque)

   This color will then be used for any subsequent drawing operation
   until a new source pattern is set.

   \param r red color value
   \param g green color value
   \param b blue color value
 */
void Cairo_Color(int r, int g, int b)
{
    G_debug(3, "Cairo_Color: %d,%d,%d", r, g, b);

    cairo_set_source_rgba(cairo, CAIROCOLOR(r), CAIROCOLOR(g), CAIROCOLOR(b),
                          1.0);

    G_debug(3, "Set color to: %g %g %g", CAIROCOLOR(r), CAIROCOLOR(g),
            CAIROCOLOR(b));
}
