#include "cairodriver.h"

void do_polygon(const int *xarray, const int *yarray, int count)
{
    int i;

    cairo_move_to(cairo, xarray[0], yarray[0]);
    for (i = 1; i < count; i++)
	cairo_line_to(cairo, xarray[i], yarray[i]);
}

void Cairo_Polygon_abs(const int *xarray, const int *yarray, int count)
{
    G_debug(3, "Cairo_Polygon_abs (%d points)", count);
    do_polygon(xarray, yarray, count);
    cairo_fill(cairo);
}

void Cairo_Polyline_abs(const int *xarray, const int *yarray, int count)
{
    G_debug(3, "Cairo_Polyline_abs (%d points)", count);
    do_polygon(xarray, yarray, count);
    cairo_stroke(cairo);
}
