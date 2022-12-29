/*!
 * \file lib/raster/color_set.c
 *
 * \brief Raster Library - Set colors for raster maps.
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

/*!
 * \brief Set a category color (CELL)
 *
 * The <i>red, green</i>, and <i>blue</i> intensities for the color
 * associated with category <i>cat</i> are set in the <i>colors</i>
 * structure.  The intensities must be in the range 0 - 255. Values
 * below zero are set as zero, values above 255 are set as 255.
 *
 * <b>Warning: Use of this routine is discouraged because it defeats the new
 * color logic.</b>
 *
 * It is provided only for backward compatibility. Overuse can create
 * large color tables. Rast_add_c_color_rule() should be used whenever
 * possible.
 *
 * <b>Note:</b> The <i>colors</i> structure must have been
 * initialized by G_init_color().
 *
 * \param cat raster cell value
 * \param r red value
 * \param g green value
 * \param b blue value
 * \param colors pointer to Colors structure which holds color info
 */
void Rast_set_c_color(CELL cat, int r, int g, int b, struct Colors *colors)
{
    if (Rast_is_c_null_value(&cat))
	Rast_set_null_value_color(r, g, b, colors);
    else
	Rast_add_c_color_rule(&cat, r, g, b, &cat, r, g, b, colors);
}

/*!
 * \brief Set a category color (DCELL)
 * 
 * See Rast_set_c_color() for detailed information.
 *
 * \param val raster cell value
 * \param r red value
 * \param g green value
 * \param b blue value
 * \param colors pointer to Colors structure which holds color info
 */
void Rast_set_d_color(DCELL val, int r, int g, int b, struct Colors *colors)
{
    if (Rast_is_d_null_value(&val))
	Rast_set_null_value_color(r, g, b, colors);
    else
	Rast_add_d_color_rule(&val, r, g, b, &val, r, g, b, colors);
}

/*!
 * \brief Set color for NULL-value
 *
 * Sets the color (in <i>colors</i>) for the NULL-value to
 * <i>red, green, blue</i>.
 *
 * \param red red value
 * \param grn green value
 * \param blu blue value
 * \param colors pointer to Colors structure which holds color info
 */
void Rast_set_null_value_color(int red, int grn, int blu,
			       struct Colors *colors)
{
    colors->null_red = red;
    colors->null_grn = grn;
    colors->null_blu = blu;
    colors->null_set = 1;
}

/*!
 * \brief Set default color value 
 *
 * Sets the default color (in <i>colors</i>) to <i>red, green,
 * blue</i>. This is the color for values which do not have an
 * explicit rule.
 *
 * \param red red value
 * \param grn green value
 * \param blu blue value
 * \param colors pointer to Colors structure which holds color info
 */
void Rast_set_default_color(int red, int grn, int blu, struct Colors *colors)
{
    colors->undef_red = red;
    colors->undef_grn = grn;
    colors->undef_blu = blu;
    colors->undef_set = 1;
}
