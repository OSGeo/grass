/*!
 * \file lib/raster/color_shift.c
 *
 * \brief Raster Library - Shift colors
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License 
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <grass/gis.h>
#include <grass/raster.h>

void Rast_shift_c_colors(CELL shift, struct Colors *colors)
{
    colors->shift += (DCELL) shift;
}

void Rast_shift_d_colors(DCELL shift, struct Colors *colors)
{
    colors->shift += shift;
}
