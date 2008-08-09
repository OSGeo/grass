#include "cairodriver.h"

/* Box_abs: Draw a (filled) rectangle */

void Cairo_Box(double x1, double y1, double x2, double y2)
{
    G_debug(3, "Cairo_Box %f %f %f %f\n", x1, y1, x2, y2);

    cairo_rectangle(cairo, x1, y1, x2 - x1, y2 - y1);
    cairo_fill(cairo);
    modified = 1;
}
