/*!
   \file lib/cairodriver/line_width.c

   \brief GRASS cairo display driver - set line width

   SPDX-FileCopyrightText: 2007-2008 by Lars Ahlzen and the GRASS Development
   Team

   SPDX-License-Identifier: GPL-2.0-or-later.

   \author Lars Ahlzen <lars ahlzen.com> (original contributor)
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
