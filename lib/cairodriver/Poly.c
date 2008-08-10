#include "cairodriver.h"

void do_polygon(const double *xarray, const double *yarray, int count)
{
    int i;

    cairo_move_to(cairo, xarray[0], yarray[0]);
    for (i = 1; i < count; i++)
	cairo_line_to(cairo, xarray[i], yarray[i]);
}

void Cairo_Polygon(const double *xarray, const double *yarray, int count)
{
    G_debug(3, "Cairo_Polygon (%d points)", count);
    do_polygon(xarray, yarray, count);
    cairo_fill(cairo);
}

void Cairo_Polyline(const double *xarray, const double *yarray, int count)
{
    G_debug(3, "Cairo_Polyline (%d points)", count);
    do_polygon(xarray, yarray, count);
    cairo_stroke(cairo);
}

void Cairo_Polydots(const double *xarray, const double *yarray, int count)
{
    int i;

    G_debug(3, "Cairo_Polydots (%d points)", count);
    for (i = 1; i < count; i++)
	Cairo_draw_point(xarray[0], yarray[0]);
}

