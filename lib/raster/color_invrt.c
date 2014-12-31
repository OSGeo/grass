/*!
   \file lib/raster/color_invrt.c

   \brief Raster library - Invert colors

   (C) 2003-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL
 */

#include <grass/gis.h>
#include <grass/raster.h>

void Rast_invert_colors(struct Colors *colors)
{
    colors->invert = !colors->invert;
}
