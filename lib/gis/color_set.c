#include <grass/gis.h>

/* for convenience, but to be avoided if possible */

/*!
 * \brief set a category color
 *
 * The <b>red, green</b>, and
 * <b>blue</b> intensities for the color associated with category <b>cat</b>
 * are set in the <b>colors</b> structure.  The intensities must be in the
 * range 0 -­ 255. Values below zero are set as zero, values above 255 are set as
 * 255.
 * <b>Use of this routine is discouraged because it defeats the new color
 * logic.</b> It is provided only for backward compatibility. Overuse can create
 * large color tables. <i>G_add_color_rule</i> should be used whenever
 * possible.
 * <b>Note.</b> The <b>colors</b> structure must have been initialized by
 * <i>G_init_color.</i>
 *
 *  \param cat
 *  \param red
 *  \param green
 *  \param blue
 *  \param colors
 *  \return int
 */

int G_set_color(CELL cat, int r, int g, int b, struct Colors *colors)
{
    CELL tmp = cat;

    if (G_is_c_null_value(&tmp))
	return G_set_null_value_color(r, g, b, colors);
    return G_add_color_rule(cat, r, g, b, cat, r, g, b, colors);
}

int G_set_d_color(DCELL val, int r, int g, int b, struct Colors *colors)
{
    DCELL tmp = val;

    if (G_is_d_null_value(&tmp))
	return G_set_null_value_color(r, g, b, colors);
    return G_add_d_raster_color_rule(&val, r, g, b, &val, r, g, b, colors);
}


/*!
 * \brief 
 *
 * Sets the color (in <em>colors</em>) for the NULL-value to <em>r,g,b</em>.
 *
 *  \param r
 *  \param g
 *  \param b
 *  \param colors
 *  \return int
 */

int G_set_null_value_color(int red, int grn, int blu, struct Colors *colors)
{
    colors->null_red = red;
    colors->null_grn = grn;
    colors->null_blu = blu;
    colors->null_set = 1;
    return 1;
}


/*!
 * \brief 
 *
 * Sets the default color (in <em>colors</em>) to <em>r,g,b</em>. This is
 * the color for values which do not have an explicit rule.
 *
 *  \param r
 *  \param g
 *  \param b
 *  \param colors
 *  \return int
 */

int G_set_default_color(int red, int grn, int blu, struct Colors *colors)
{
    colors->undef_red = red;
    colors->undef_grn = grn;
    colors->undef_blu = blu;
    colors->undef_set = 1;
    return 1;
}
