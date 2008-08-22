#include "cairodriver.h"

void Cairo_Erase(void)
{
    G_debug(1, "Cairo_Erase");

    cairo_save(cairo);
    cairo_set_source_rgba(cairo, ca.bgcolor_r, ca.bgcolor_g, ca.bgcolor_b, ca.bgcolor_a);
    cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cairo);
    cairo_restore(cairo);

    ca.modified = 1;
}
