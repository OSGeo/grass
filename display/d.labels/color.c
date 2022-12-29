
/****************************************************************************
 *
 * FILE:         d.paint.labels/color.c
 * AUTHOR(S):    Hamish Bowman, Dunedin New Zealand
 * PURPOSE:      lib fns for working with RGBA_Color struct.
 *		 Inspired by ps.map's ps_colors.c
 * COPYRIGHT:    (C) 2007 by Hamish Bowman, and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/**  ??? move these into libgis ??? (libraster for set_color_from_RGBA()) **/

#include <grass/gis.h>
#include <grass/colors.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/glocale.h>

/* fill RGBA array from RGB components (0-255) */
void set_RGBA_from_components(RGBA_Color * color,
			      const unsigned char r,
			      const unsigned char g, const unsigned char b)
{
    color->a = RGBA_COLOR_OPAQUE;

    color->r = r;
    color->g = g;
    color->b = b;
}


/* 
 * \brief Parses color string and fill RGBA array
 *
 * If the color is valid the color's alpha value is set to RGBA_COLOR_OPAQUE.
 * The 'none' case is handed by setting the alpha channel to RGBA_COLOR_NONE.
 *
 *  Returns: 1 - OK
 *           2 - NONE
 *           0 - Error 
 *
 *  \param color_string
 *  \param RGBA_Color struct to be populated
 *  \return int
 * 
 */
int set_RGBA_from_str(RGBA_Color * color, const char *clr_str)
{
    int r, g, b;
    int ret;

    ret = G_str_to_color(clr_str, &r, &g, &b);

    if (ret == 1) {
	color->a = RGBA_COLOR_OPAQUE;
	color->r = (unsigned char)r;
	color->g = (unsigned char)g;
	color->b = (unsigned char)b;
    }
    else if (ret == 2)
	color->a = RGBA_COLOR_NONE;
    else
	G_fatal_error(_("[%s]: No such color"), clr_str);

    return ret;
}

/* set RGBA "color=none" flag */
void unset_RGBA(RGBA_Color * color)
{
    color->a = RGBA_COLOR_NONE;
}

/* tests if RGBA "color=none" */
int RGBA_has_color(const RGBA_Color * color)
{
    if (color->a != RGBA_COLOR_NONE)
	return TRUE;
    else
	return FALSE;
}


/* set active display color from values in the RGBA array */
void set_color_from_RGBA(const RGBA_Color * color)
{
    if (RGBA_has_color(color)) {
	G_debug(5, "setting display color to [%d:%d:%d]",
		color->r, color->g, color->b);

	D_RGB_color(color->r, color->g, color->b);
    }
    else
	G_debug(5, "skipped setting display color as it was set to \"none\"");
}
