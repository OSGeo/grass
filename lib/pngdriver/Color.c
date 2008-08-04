/*
 * Identify a color that has been set in the reset_color() (found in Reset_clr.c
 * file in this directory).  Subsequent graphics calls will use this color.
 *
 * Called by:
 *      Color() in ../lib/Color.c
 */

#include <grass/gis.h>
#include "pngdriver.h"

void PNG_color(int number)
{
    if (number >= NCOLORS || number < 0) {
	G_warning("Color: can't set color %d\n", number);
	return;
    }

    if (true_color) {
	int r = (number >> 16) & 0xFF;
	int g = (number >> 8) & 0xFF;
	int b = (number >> 0) & 0xFF;

	currentColor = get_color(r, g, b, 0);
    }
    else
	currentColor = number;
}
