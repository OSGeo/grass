#include "cairodriver.h"

void Cairo_Set_window(double t, double b, double l, double r)
{
    G_debug(1, "Cairo_Set_window: %f %f %f %f", t, b, l, r);

    cairo_reset_clip(cairo);
    cairo_rectangle(cairo, l, t, r - l, b - t);
    cairo_clip(cairo);
}
