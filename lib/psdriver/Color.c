
#include <grass/gis.h>
#include "psdriver.h"

void PS_color(int number)
{
    if (number >= NCOLORS || number < 0) {
	G_warning("Color: can't set color %d\n", number);
	return;
    }

    if (true_color) {
	int r = (number >> 16) & 0xFF;
	int g = (number >> 8) & 0xFF;
	int b = (number >> 0) & 0xFF;

	output("%d %d %d COLOR\n", r, g, b);
    }
    else
	output("%d GRAY\n", number);
}
