#include "cairodriver.h"

void Cairo_Erase(void)
{
    G_debug(1, "Cairo_Erase");

    cairo_save(cairo);
    cairo_set_source_rgba(cairo, bgcolor_r, bgcolor_g, bgcolor_b, bgcolor_a);
    cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cairo);
    cairo_restore(cairo);

    modified = 1;
}
