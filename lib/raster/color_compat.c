/*!
 * \file lib/raster/color_compat.c
 *
 * \brief Raster Library - Predefined color tables
 *
 * (C) 2007-2009 Glynn Clements and the GRASS Development Team
 *
 * This program is free software under the GNU General Public License 
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Glynn Clements <glynn@gclements.plus.com>
 */

#include <grass/gis.h>
#include <grass/raster.h>

/*!
 * \brief Make color wave (integer)
 *
 * Generates a color table with 3 sections: red only, green only, and
 * blue only, each increasing from none to full intensity and back
 * down to none. This table is good for continuous data like
 * elevation.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param min minimum value
 * \param max maximum value
 */
void Rast_make_wave_colors(struct Colors *colors, CELL min, CELL max)
{
    Rast_make_colors(colors, "wave", min, max);
}

/*!
 * \brief Make color wave (floating-point)
 *
 * Generates a color table with 3 sections: red only, green only, and
 * blue only, each increasing from none to full intensity and back
 * down to none. This table is good for continuous data like
 * elevation.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param min minimum value
 * \param max maximum value
 */
void Rast_make_wave_fp_colors(struct Colors *colors, DCELL min, DCELL max)
{
    Rast_make_fp_colors(colors, "wave", min, max);
}

/*!
 * \brief Create RYG color table (integer)
 *
 * Generates a color table red-yellow-green.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param min minimum value
 * \param max maximum value
 */
void Rast_make_ryg_colors(struct Colors *colors, CELL min, CELL max)
{
    Rast_make_colors(colors, "ryg", min, max);
}

/*!
 * \brief Create RYG color table (floating-point)
 *
 * Generates a color table red-yellow-green.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param min minimum value
 * \param max maximum value
 */
void Rast_make_ryg_fp_colors(struct Colors *colors, DCELL min, DCELL max)
{
    Rast_make_fp_colors(colors, "ryg", min, max);
}

/*!
 * \brief Make color ramp (integer)
 *
 * Generates a color table with 3 sections: red only, green only, and
 * blue only, each increasing from none to full intensity. This table
 * is good for continuous data, such as elevation.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param min minimum value
 * \param max maximum value
 */
void Rast_make_ramp_colors(struct Colors *colors, CELL min, CELL max)
{
    Rast_make_colors(colors, "ramp", min, max);
}

/*!
 * \brief Make color ramp (floating-point)
 *
 * Generates a color table with 3 sections: red only, green only, and
 * blue only, each increasing from none to full intensity. This table
 * is good for continuous data, such as elevation.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param min minimum value
 * \param max maximum value
 */
void Rast_make_ramp_fp_colors(struct Colors *colors, DCELL min, DCELL max)
{
    Rast_make_fp_colors(colors, "ramp", min, max);
}

/*!
 * \brief Make rainbow colors (integer)
 *
 * Generates a "shifted" rainbow color table - yellow to green to cyan
 * to blue to magenta to red. The color table is based on rainbow
 * colors. (Normal rainbow colors are red, orange, yellow, green,
 * blue, indigo, and violet.) This table is good for continuous data,
 * such as elevation.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param min minimum value
 * \param max maximum value
 */
void Rast_make_rainbow_colors(struct Colors *colors, CELL min, CELL max)
{
    Rast_make_colors(colors, "rainbow", min, max);
}

/*!
 * \brief Make rainbow colors (floating-point)
 *
 * Generates a "shifted" rainbow color table - yellow to green to cyan
 * to blue to magenta to red. The color table is based on rainbow
 * colors. (Normal rainbow colors are red, orange, yellow, green,
 * blue, indigo, and violet.) This table is good for continuous data,
 * such as elevation.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param min minimum value
 * \param max maximum value
 */
void Rast_make_rainbow_fp_colors(struct Colors *colors, DCELL min, DCELL max)
{
    Rast_make_fp_colors(colors, "rainbow", min, max);
}

/*!
 * \brief Create GYR color table (integer)
 *
 * Generates a color table green-yellow-red.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param min minimum value
 * \param max maximum value
 */
void Rast_make_gyr_colors(struct Colors *colors, CELL min, CELL max)
{
    Rast_make_colors(colors, "gyr", min, max);
}

/*!
 * \brief Create GYR color table (floating-point)
 *
 * Generates a color table green-yellow-red.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param min minimum value
 * \param max maximum value
 */
void Rast_make_gyr_fp_colors(struct Colors *colors, DCELL min, DCELL max)
{
    Rast_make_fp_colors(colors, "gyr", min, max);
}

/*!
 * \brief Make linear grey scale (integer)
 *
 * Generates a grey scale color table. Each color is a level of grey,
 * increasing from black to white.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param min minimum value
 * \param max maximum value
 */
void Rast_make_grey_scale_colors(struct Colors *colors, CELL min, CELL max)
{
    Rast_make_colors(colors, "grey", min, max);
}

/*!
 * \brief Make linear grey scale (floating-point)
 *
 * Generates a grey scale color table. Each color is a level of grey,
 * increasing from black to white.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param min minimum value
 * \param max maximum value
 */
void Rast_make_grey_scale_fp_colors(struct Colors *colors, DCELL min,
				    DCELL max)
{
    Rast_make_fp_colors(colors, "grey", min, max);
}

/*!
 * \brief Create BYR color table (integer)
 *
 * Generates a color table blue-yellow-red.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param min minimum value
 * \param max maximum value
 */
void Rast_make_byr_colors(struct Colors *colors, CELL min, CELL max)
{
    Rast_make_colors(colors, "byr", min, max);
}

/*!
 * \brief Create BYR color table (floating-point)
 *
 * Generates a color table blue-yellow-red.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param min minimum value
 * \param max maximum value
 */
void Rast_make_byr_fp_colors(struct Colors *colors, DCELL min, DCELL max)
{
    Rast_make_fp_colors(colors, "byr", min, max);
}

/*!
 * \brief Create BGYR color table (integer)
 *
 * Generates a color table blue-green-yellow-red.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param min minimum value
 * \param max maximum value
 */
void Rast_make_bgyr_colors(struct Colors *colors, CELL min, CELL max)
{
    Rast_make_colors(colors, "bgyr", min, max);
}

/*!
 * \brief Create BGYR color table (floating-point)
 *
 * Generates a color table blue-green-yellow-red.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param min minimum value
 * \param max maximum value
 */
void Rast_make_bgyr_fp_colors(struct Colors *colors, DCELL min, DCELL max)
{
    Rast_make_fp_colors(colors, "bgyr", min, max);
}

/*!
 * \brief Create BYG color table (integer)
 *
 * Generates a color table blue-yellow-green.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param min minimum value
 * \param max maximum value
 */
void Rast_make_byg_colors(struct Colors *colors, CELL min, CELL max)
{
    Rast_make_colors(colors, "byg", min, max);
}

/*!
 * \brief Create BYG color table (floating-point)
 *
 * Generates a color table blue-yellow-green.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param min minimum value
 * \param max maximum value
 */
void Rast_make_byg_fp_colors(struct Colors *colors, DCELL min, DCELL max)
{
    Rast_make_fp_colors(colors, "byg", min, max);
}

/*!
 * \brief Make aspect colors (integer)
 *
 * Generates a color table for aspect data.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param min minimum value
 * \param max maximum value
 */
void Rast_make_aspect_colors(struct Colors *colors, CELL min, CELL max)
{
    Rast_make_colors(colors, "aspect", min, max);
}

/*!
 * \brief Make aspect colors (floating-point)
 *
 * Generates a color table for aspect data.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param min minimum value
 * \param max maximum value
 */
void Rast_make_aspect_fp_colors(struct Colors *colors, DCELL min, DCELL max)
{
    Rast_make_fp_colors(colors, "aspect", min, max);
}
