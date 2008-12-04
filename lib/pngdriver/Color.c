/*
 * Identify a color that has been set in the reset_color() (found in Reset_clr.c
 * file in this directory).  Subsequent graphics calls will use this color.
 *
 * Called by:
 *      Color() in ../lib/Color.c
 */

#include <grass/gis.h>
#include "pngdriver.h"

void PNG_color_rgb(int r, int g, int b)
{
    png.current_color = png_get_color(r, g, b, 0);
}
