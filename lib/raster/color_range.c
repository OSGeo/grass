/*!
 * \file lib/raster/color_range.c
 *
 * \brief Raster Library - Color range functions.
 *
 * SPDX-FileCopyrightText: 2001-2009 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * \author Original author CERL
 */

#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>

/*!
   \brief Set color range (CELL version)

   \param min,max minimum and maximum value
   \param colors pointer to Colors structure which holds color info
 */
void Rast_set_c_color_range(CELL min, CELL max, struct Colors *colors)
{
    if (min < max) {
        colors->cmin = (DCELL)min;
        colors->cmax = (DCELL)max;
    }
    else {
        colors->cmin = (DCELL)max;
        colors->cmax = (DCELL)min;
    }
}

/*!
   \brief Set color range (DCELL version)

   \param min,max minimum and maximum value
   \param colors pointer to Colors structure which holds color info
 */
void Rast_set_d_color_range(DCELL min, DCELL max, struct Colors *colors)
{
    if (min < max) {
        colors->cmin = min;
        colors->cmax = max;
    }
    else {
        colors->cmin = max;
        colors->cmax = min;
    }
}

/*!
   \brief Get color range values (CELL)

   Returns min and max category in the range or huge numbers if the
   color table is defined on floating cell values and not on
   categories.

   \param[out] min,max minimum and maximum value
   \param colors pointer to Colors structure which holds color info
 */
void Rast_get_c_color_range(CELL *min, CELL *max, const struct Colors *colors)
{
    if (!colors->is_float) {
        *min = (CELL)floor(colors->cmin);
        *max = (CELL)ceil(colors->cmax);
    }
    else {
        *min = -255 * 255 * 255;
        *max = 255 * 255 * 255;
    }
}

/*!
   \brief Get color range values (DCELL)

   Returns min and max category in the range or huge numbers if the
   color table is defined on floating cell values and not on
   categories.

   \param[out] min,max minimum and maximum value
   \param colors pointer to Colors structure which holds color info
 */
void Rast_get_d_color_range(DCELL *min, DCELL *max, const struct Colors *colors)
{
    *min = colors->cmin;
    *max = colors->cmax;
}
