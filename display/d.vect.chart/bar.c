#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/display.h>
#include <grass/symbol.h>
#include "global.h"

int
bar(double cx, double cy, int size, double scale, double *val, int ncols,
    COLOR * ocolor, COLOR * colors, int y_center, double *max_reference,
    int do3d)
{
    int i;
    double max;
    double x0, y0, x1, y1, dx, dy;
    double bw;			/* bar width */
    double pixel;		/* pixel size */
    struct line_pnts *Points, *max_Points;

    G_debug(4, "bar(): cx = %f cy = %f", cx, cy);

    Points = Vect_new_line_struct();
    max_Points = Vect_new_line_struct();

    pixel = D_d_to_u_col(2) - D_d_to_u_col(1);	/* do it better */

    /* Bottom (y0) */
    max = 0;
    for (i = 0; i < ncols; i++) {
	if (val[i] > max)
	    max = val[i];
    }

    /* debug */
    /* printf("%f \n", max_reference); */

    if (y_center == 0)
	/* draw the columns with the bottom at the y value of the point  */
	y0 = cy;
    else
	/* center the columns around the y value of the point */
	y0 = cy - scale * max * pixel / 2;

    /* Left (x0) */
    x0 = cx - size * pixel / 2;

    bw = size * pixel / ncols;
    dx = bw / 5.0;
    dy = dx * 1.5;

    if (max_reference) {
	/* Draw polygon outlining max value in dataset with no fill color */
	for (i = 0; i < ncols; i++) {
	    Vect_reset_line(max_Points);
	    Vect_append_point(max_Points, x0 + i * bw, y0, 0);
	    Vect_append_point(max_Points, x0 + (i + 1) * bw, y0, 0);
	    Vect_append_point(max_Points, x0 + (i + 1) * bw,
			      y0 + scale * max_reference[i] * pixel, 0);
	    Vect_append_point(max_Points, x0 + i * bw,
			      y0 + scale * max_reference[i] * pixel, 0);
	    Vect_append_point(max_Points, x0 + i * bw, y0, 0);

	    /* the outline color : default is black */
	    D_RGB_color(ocolor->r, ocolor->g, ocolor->b);
	    D_polyline_abs(max_Points->x, max_Points->y, max_Points->n_points);
	}
    }

    /* Draw polygon for each value */
    for (i = 0; i < ncols; i++) {
	Vect_reset_line(Points);
	Vect_append_point(Points, x0 + i * bw, y0, 0);
	Vect_append_point(Points, x0 + (i + 1) * bw, y0, 0);
	Vect_append_point(Points, x0 + (i + 1) * bw,
			  y0 + scale * val[i] * pixel, 0);
	Vect_append_point(Points, x0 + i * bw, y0 + scale * val[i] * pixel,
			  0);
	Vect_append_point(Points, x0 + i * bw, y0, 0);

	if (!colors[i].none) {
	    D_RGB_color(colors[i].r, colors[i].g, colors[i].b);
	    D_polygon_abs(Points->x, Points->y, Points->n_points);
	}

	D_RGB_color(ocolor->r, ocolor->g, ocolor->b);
	D_polyline_abs(Points->x, Points->y, Points->n_points);
	
	if (do3d) {
	    /* up */
	    Vect_reset_line(Points);
	    y1 = y0 + scale * val[i] * pixel;
	    Vect_append_point(Points, x0 + i * bw, y1, 0);
	    Vect_append_point(Points, x0 + i * bw + dx, y1 + dy, 0);
	    Vect_append_point(Points, x0 + (i + 1) * bw + dx, y1 + dy, 0);
	    Vect_append_point(Points, x0 + (i + 1) * bw, y1, 0);
	    Vect_append_point(Points, x0 + i * bw, y1, 0);

	    if (!colors[i].none) {
		D_RGB_color(colors[i].r, colors[i].g, colors[i].b);
		D_polygon_abs(Points->x, Points->y, Points->n_points);
	    }

	    D_RGB_color(ocolor->r, ocolor->g, ocolor->b);
	    /* do not draw the same line twice */
	    Points->n_points = 4;
	    D_polyline_abs(Points->x, Points->y, Points->n_points);

	    /* right */
	    Vect_reset_line(Points);
	    x1 = x0 + (i + 1) * bw;
	    Vect_append_point(Points, x1 + dx + 0.5 * pixel, y1 + dy, 0);
	    Vect_append_point(Points, x1 + dx + 0.5 * pixel, y0 + dy, 0);
	    Vect_append_point(Points, x1, y0, 0);
	    Vect_append_point(Points, x1, y1, 0);
	    Vect_append_point(Points, x1 + dx + 0.5 * pixel, y1 + dy, 0);

	    if (!colors[i].none && val[i] > 0) {
		D_RGB_color(colors[i].r, colors[i].g, colors[i].b);
		D_polygon_abs(Points->x, Points->y, Points->n_points);
	    }

	    D_RGB_color(ocolor->r, ocolor->g, ocolor->b);
	    /* do not draw the same line twice */
	    Points->n_points = 3;
	    D_polyline_abs(Points->x, Points->y, Points->n_points);
	}
    }

    /* tidy up */
    Vect_destroy_line_struct(Points);
    Vect_destroy_line_struct(max_Points);

    return 0;
}
