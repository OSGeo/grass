/*!
 * \file gis/color_get.c
 *
 * \brief GIS Library - Get colors from a raster map.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <grass/gis.h>

/*!
 * \brief Get a category color.
 *
 * \todo To be removed, replaced by G_get_raster_color().
 *
 * The <i>red, green</i>, and <i>blue</i> intensities for the color
 * associated with category <i>n</i> are extracted from the
 * <i>colors</i> structure. The intensities will be in the range 0 ­-
 * 255. Also works for null cells.
 *
 * \param n cell to get color from
 * \param[out] red red value
 * \param[out] grn green value
 * \param[out] blu blue value
 * \param colors pointer to Colors structure which holds color info
 *
 * \return 1 if color is set
 * \return 0 if color is not set
 */
int G_get_color(CELL n, int *red, int *grn, int *blu, struct Colors *colors)
{
    CELL cat;
    unsigned char r, g, b, set;

    cat = n;
    G_lookup_colors(&cat, &r, &g, &b, &set, 1, colors);

    *red = (int)r;
    *grn = (int)g;
    *blu = (int)b;

    return (int)set;
}

/*!
 * \brief Gets color from raster map
 *
 * Looks up the rgb colors for <i>rast</i> in the color table
 * <i>colors</i>.
 *
 * \param rast raster cell value
 * \param[out] red red value
 * \param[out] grn green value
 * \param[out] blu blue value
 * \param colors pointer to Colors structure which holds color info
 * \param map_type map type (CELL, FCELL, DCELL)
 *
 * \return 1 if color is set
 * \return 0 if color is not set
 */
int G_get_raster_color(const void *rast,
		       int *red, int *grn, int *blu,
		       struct Colors *colors, RASTER_MAP_TYPE map_type)
{
    unsigned char r, g, b, set;

    G_lookup_raster_colors(rast, &r, &g, &b, &set, 1, colors, map_type);

    *red = (int)r;
    *grn = (int)g;
    *blu = (int)b;

    return (int)set;
}

/*!
 * \brief Gets color from raster map (CELL)
 *
 * Looks up the rgb colors for <i>rast</i> in the color table
 * <i>colors</i>.
 *
 * \param rast raster cell value
 * \param[out] red red value
 * \param[out] grn green value
 * \param[out] blu blue value
 * \param colors pointer to Colors structure which holds color info
 *
 * \return 1 if color is set
 * \return 0 if color is not set
 */
int G_get_c_raster_color(const CELL * rast,
			 int *red, int *grn, int *blu, struct Colors *colors)
{
    return G_get_raster_color(rast, red, grn, blu, colors, CELL_TYPE);
}

/*!
 * \brief Gets color from raster map (FCELL)
 *
 * Looks up the rgb colors for <i>rast</i> in the color table
 * <i>colors</i>.
 *
 * \param rast raster cell value
 * \param[out] red red value
 * \param[out] grn green value
 * \param[out] blu blue value
 * \param colors pointer to Colors structure which holds color info
 *
 * \return 1 if color is set
 * \return 0 if color is not set
 */
int G_get_f_raster_color(const FCELL * rast,
			 int *red, int *grn, int *blu, struct Colors *colors)
{
    return G_get_raster_color(rast, red, grn, blu, colors, FCELL_TYPE);
}

/*!
 * \brief Gets color from raster map (DCELL)
 *
 * Looks up the rgb colors for <i>rast</i> in the color table
 * <i>colors</i>.
 *
 * \param rast raster cell value
 * \param[out] red red value
 * \param[out] grn green value
 * \param[out] blu blue value
 * \param colors pointer to Colors structure which holds color info
 *
 * \return 1 if color is set
 * \return 0 if color is not set
 */
int G_get_d_raster_color(const DCELL * rast,
			 int *red, int *grn, int *blu, struct Colors *colors)
{
    return G_get_raster_color(rast, red, grn, blu, colors, DCELL_TYPE);
}

/*!
 * \brief Gets color for null value.
 *
 * Puts the red, green, and blue components of <i>colors</i> for the
 * NULL-value into <i>red, grn, and blu</i>.
 *
 * \param[out] red red value
 * \param[out] grn green value
 * \param[out] blu blue value
 * \param colors pointer to Colors structure which holds color info
 */
void G_get_null_value_color(int *red, int *grn, int *blu,
			   const struct Colors *colors)
{
    if (colors->null_set) {
	*red = (int)colors->null_red;
	*grn = (int)colors->null_grn;
	*blu = (int)colors->null_blu;
    }
    else if (colors->undef_set) {
	*red = (int)colors->undef_red;
	*grn = (int)colors->undef_grn;
	*blu = (int)colors->undef_blu;
    }
    else
	*red = *blu = *grn = 255;	/* white */
}

/*!
 * \brief Gets default color.
 *
 * Puts the red, green, and blue components of the <tt>"default"</tt>
 * color into <i>red, grn, and blu</i>.
 *
 * \param[out] red red value
 * \param[out] grn green value
 * \param[out] blu blue value
 * \param colors pointer to Colors structure which holds color info
 */
void G_get_default_color(int *red, int *grn, int *blu,
			 const struct Colors *colors)
{
    if (colors->undef_set) {
	*red = (int)colors->undef_red;
	*grn = (int)colors->undef_grn;
	*blu = (int)colors->undef_blu;
    }
    else
	*red = *blu = *grn = 255;	/* white */
}
