#include "cairodriver.h"

#define POINTSIZE 1.0
#define HALFPOINTSIZE (0.5*POINTSIZE)

void Cairo_draw_point(double x, double y)
{
    G_debug(3, "Cairo_draw_point: %f %f", x, y);

    cairo_rectangle(cairo,
		    x - HALFPOINTSIZE, y - HALFPOINTSIZE,
		    POINTSIZE, POINTSIZE);
    cairo_fill(cairo);
    modified = 1;
}
