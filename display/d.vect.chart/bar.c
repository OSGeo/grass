#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/symbol.h>
#include "global.h"

int
bar(double cx, double cy, int size, double scale, double *val, int ncols,
    COLOR * ocolor, COLOR * colors, int y_center, double *max_reference)
{
    int i, j;
    double max;
    double x0, y0;
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
	    R_RGB_color(ocolor->r, ocolor->g, ocolor->b);
	    for (j = 1; j < max_Points->n_points; j++) {
		G_plot_line(max_Points->x[j], max_Points->y[j],
			    max_Points->x[j - 1], max_Points->y[j - 1]);
	    }
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
	    R_RGB_color(colors[i].r, colors[i].g, colors[i].b);
	    D_polygon(Points->x, Points->y, Points->n_points);
	}

	R_RGB_color(ocolor->r, ocolor->g, ocolor->b);
	for (j = 1; j < Points->n_points; j++) {
	    G_plot_line(Points->x[j], Points->y[j], Points->x[j - 1],
			Points->y[j - 1]);
	}
    }

    /* tidy up */
    Vect_destroy_line_struct(Points);
    Vect_destroy_line_struct(max_Points);

    return 0;
}
