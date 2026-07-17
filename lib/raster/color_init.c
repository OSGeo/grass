/*!
   \file lib/raster/color_init.c

   \brief Raster Library - Initialize Colors structure

   SPDX-FileCopyrightText: 2001-2009 Other GRASS authors
   SPDX-License-Identifier: GPL-2.0-or-later

   \author Original author CERL
 */

#include <grass/gis.h>
#include <grass/raster.h>

/*!
   \brief Initialize color structure

   The <i>colors</i> structure is initialized for subsequent calls
   to Rast_add_c_color_rule() and Rast_set_c_color().

   \param colors pointer to Colors structure
 */
void Rast_init_colors(struct Colors *colors)
{
    G_zero(colors, sizeof(struct Colors));

    colors->cmax = -1;
    colors->fixed.max = -1;
    colors->modular.max = -1;
}
