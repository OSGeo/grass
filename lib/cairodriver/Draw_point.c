#include "cairodriver.h"

#define POINTSIZE 1.0
#define HALFPOINTSIZE (0.5*POINTSIZE)

void Cairo_draw_point(int x, int y)
{
    G_debug(3, "Cairo_draw_point: %d %d", x, y);

    cairo_rectangle(cairo, (double)x - HALFPOINTSIZE,
		    (double)y - HALFPOINTSIZE, POINTSIZE, POINTSIZE);
    cairo_fill(cairo);
    modified = 1;
}
