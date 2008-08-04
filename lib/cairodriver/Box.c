#include "cairodriver.h"

/* Box_abs: Draw a (filled) rectangle */

void Cairo_Box_abs(int x1, int y1, int x2, int y2)
{
    G_debug(3, "Cairo_Box_abs %d %d %d %d\n", x1, y1, x2, y2);

    cairo_rectangle(cairo, (double)x1, (double)y1, (double)x2 - x1,
		    (double)y2 - y1);
    cairo_fill(cairo);
    modified = 1;
}
