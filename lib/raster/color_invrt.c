/*!
   \file lib/raster/color_invrt.c

   \brief Raster library - Invert colors

   (C) 2003-2009 by the GRASS Development Team

    SPDX-License-Identifier: GPL-2.0-or-later
\author Original author CERL
 */

#include <grass/gis.h>
#include <grass/raster.h>

void Rast_invert_colors(struct Colors *colors)
{
    colors->invert = !colors->invert;
}
