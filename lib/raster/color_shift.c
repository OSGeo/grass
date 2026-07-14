/*!
 * \file lib/raster/color_shift.c
 *
 * \brief Raster Library - Shift colors
 *
 * SPDX-FileCopyrightText:  2001-2009 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
 *
 * \author Original author CERL
 */

#include <grass/gis.h>
#include <grass/raster.h>

void Rast_shift_c_colors(CELL shift, struct Colors *colors)
{
    colors->shift += (DCELL)shift;
}

void Rast_shift_d_colors(DCELL shift, struct Colors *colors)
{
    colors->shift += shift;
}
