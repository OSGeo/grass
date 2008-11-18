
/****************************************************************************
 *
 * MODULE:       gis library
 * AUTHOR(S):    Glynn Clements <glynn@gclements.plus.com>
 * COPYRIGHT:    (C) 2007 Glynn Clements and the GRASS Development Team
 *
 * NOTE:         Compatibility wrappers for G_make_*[_fp]_colors()
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *****************************************************************************/

#include <grass/gis.h>


/*!
 * \brief make color wave
 *
 * Generates a color table with 3 sections: red only,
 * green only, and blue only, each increasing from none to full intensity and
 * back down to none.  This table is good for continuous data like elevation.
 *
 *  \param colors
 *  \param min
 *  \param max
 *  \return int
 */

int G_make_wave_colors(struct Colors *colors, CELL min, CELL max)
{
    return G_make_colors(colors, "wave", min, max);
}

int G_make_wave_fp_colors(struct Colors *colors, DCELL min, DCELL max)
{
    return G_make_fp_colors(colors, "wave", min, max);
}


int G_make_ryg_colors(struct Colors *colors, CELL min, CELL max)
{
    return G_make_colors(colors, "ryg", min, max);
}

int G_make_ryg_fp_colors(struct Colors *colors, DCELL min, DCELL max)
{
    return G_make_fp_colors(colors, "ryg", min, max);
}

/*!
 * \brief make color ramp
 *
 * Generates a color table with 3 sections: red only,
 * green only, and blue only, each increasing from none to full intensity. This
 * table is good for continuous data, such as elevation.
 *
 *  \param colors
 *  \param min
 *  \param max
 *  \return int
 */

int G_make_ramp_colors(struct Colors *colors, CELL min, CELL max)
{
    return G_make_colors(colors, "ramp", min, max);
}

int G_make_ramp_fp_colors(struct Colors *colors, DCELL min, DCELL max)
{
    return G_make_fp_colors(colors, "ramp", min, max);
}

/*!
 * \brief make rainbow colors
 *
 * Generates a "shifted" rainbow color table - yellow
 * to green to cyan to blue to magenta to red. The color table is based on
 * rainbow colors. (Normal rainbow colors are red, orange, yellow, green, blue,
 * indigo, and violet.)  This table is good for continuous data, such as
 * elevation.
 *
 *  \param colors
 *  \param min
 *  \param max
 *  \return int
 */

int G_make_rainbow_colors(struct Colors *colors, CELL min, CELL max)
{
    return G_make_colors(colors, "rainbow", min, max);
}

int G_make_rainbow_fp_colors(struct Colors *colors, DCELL min, DCELL max)
{
    return G_make_fp_colors(colors, "rainbow", min, max);
}


int G_make_gyr_colors(struct Colors *colors, CELL min, CELL max)
{
    return G_make_colors(colors, "gyr", min, max);
}

int G_make_gyr_fp_colors(struct Colors *colors, DCELL min, DCELL max)
{
    return G_make_fp_colors(colors, "gyr", min, max);
}

/*!
 * \brief make linear grey scale
 *
 * Generates a grey scale color table. Each color
 * is a level of grey, increasing from black to white.
 *
 *  \param colors
 *  \param min
 *  \param max
 *  \return int
 */

int G_make_grey_scale_colors(struct Colors *colors, CELL min, CELL max)
{
    return G_make_colors(colors, "grey", min, max);
}

int G_make_grey_scale_fp_colors(struct Colors *colors, DCELL min, DCELL max)
{
    return G_make_fp_colors(colors, "grey", min, max);
}

int G_make_byr_colors(struct Colors *colors, CELL min, CELL max)
{
    return G_make_colors(colors, "byr", min, max);
}

int G_make_byr_fp_colors(struct Colors *colors, DCELL min, DCELL max)
{
    return G_make_fp_colors(colors, "byr", min, max);
}

int G_make_bgyr_colors(struct Colors *colors, CELL min, CELL max)
{
    return G_make_colors(colors, "bgyr", min, max);
}

int G_make_bgyr_fp_colors(struct Colors *colors, DCELL min, DCELL max)
{
    return G_make_fp_colors(colors, "bgyr", min, max);
}

int G_make_byg_colors(struct Colors *colors, CELL min, CELL max)
{
    return G_make_colors(colors, "byg", min, max);
}

int G_make_byg_fp_colors(struct Colors *colors, DCELL min, DCELL max)
{
    return G_make_fp_colors(colors, "byg", min, max);
}

/*!
 * \brief make aspect colors
 *
 * Generates a color table for aspect data.
 *
 *  \param colors
 *  \param min
 *  \param max
 *  \return int
 */

int G_make_aspect_colors(struct Colors *colors, CELL min, CELL max)
{
    return G_make_colors(colors, "aspect", min, max);
}

int G_make_aspect_fp_colors(struct Colors *colors, DCELL min, DCELL max)
{
    return G_make_fp_colors(colors, "aspect", min, max);
}
