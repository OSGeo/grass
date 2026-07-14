/*!
   \file lib/pngdriver/line_width.c

   \brief GRASS png display driver - set line width

   SPDX-FileCopyrightText: 2003-2014 by Per Henrik Johansen and the GRASS
   Development Team

   SPDX-License-Identifier: GPL-2.0-or-later.

   \author Per Henrik Johansen (original contributor)
   \author Glynn Clements
 */

#include <math.h>
#include "pngdriver.h"

/*!
   \brief Set line width

   \param width line width (double precision)
 */
void PNG_Line_width(double width)
{
    png.linewidth = (width < 0 ? 0 : (int)floor(width + 0.5));
}
