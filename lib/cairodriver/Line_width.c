#include "cairodriver.h"

#define MIN_WIDTH 1

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

static double previous_width = -1;

void Cairo_Line_width(double width)
{
    G_debug(1, "Cairo_Line_width: %f", width);

    width = MAX(MIN_WIDTH, width);
    if (width != previous_width)
	cairo_set_line_width(cairo, width);
}
