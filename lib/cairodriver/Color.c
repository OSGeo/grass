#include "cairodriver.h"

/* "cached" color (to avoid more color change calls than necessary) */
/* TODO: find a proper solution for initialization */
int previous_color = 0x7FFFFFFF;

void Cairo_color(int color)
{
    G_debug(3, "Cairo_color: %d", color);

    if (color != previous_color) {
	int r = (color >> 16) & 0xFF;
	int g = (color >> 8) & 0xFF;
	int b = (color >> 0) & 0xFF;

	cairo_set_source_rgba(cairo, CAIROCOLOR(r), CAIROCOLOR(g),
			      CAIROCOLOR(b), 1.0);
	previous_color = color;

	G_debug(3, "Set color to: %g %g %g", CAIROCOLOR(r), CAIROCOLOR(g),
		CAIROCOLOR(b));
    }
}

int Cairo_lookup_color(int r, int g, int b)
{
    G_debug(3, "Cairo_lookup_color: %d %d %d", r, g, b);

    return (r << 16) + (g << 8) + (b << 0);
}
