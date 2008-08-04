#include "cairodriver.h"

void Cairo_Set_window(int t, int b, int l, int r)
{
    G_debug(1, "Cairo_Set_window: %d %d %d %d", t, b, l, r);

    cairo_reset_clip(cairo);
    cairo_rectangle(cairo, (double)l, (double)t, (double)r - l,
		    (double)b - t);
    cairo_clip(cairo);
}
