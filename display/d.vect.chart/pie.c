#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/display.h>
#include <grass/symbol.h>
#include "global.h"

#define PI  M_PI

int
pie(double cx, double cy, int size, double *val, int ncols, COLOR * ocolor,
    COLOR * colors)
{
    int i, j, n;
    double a, end_ang, ang, tot_sum, sum, step, r;
    double x, y;
    struct line_pnts *Points;

    G_debug(4, "pie(): cx = %f cy = %f", cx, cy);

    Points = Vect_new_line_struct();

    /* Calc sum */
    tot_sum = 0;
    for (i = 0; i < ncols; i++)
	tot_sum += val[i];

    step = PI / 180;
    r = (D_d_to_u_col(2) - D_d_to_u_col(1)) * size / 2;	/* do it better */
    /* Draw polygon for each value */
    sum = 0;
    ang = 0;
    for (i = 0; i < ncols; i++) {
	if (val[i] == 0)
	    continue;

	sum += val[i];

	end_ang = 2 * PI * sum / tot_sum;

	Vect_reset_line(Points);

	if (val[i] != tot_sum)    /* all in one slice, don't draw line to center */
	    Vect_append_point(Points, cx, cy, 0);

	n = (int)ceil((end_ang - ang) / step);
	for (j = 0, a = ang; j <= n; j++, a += step) {
	    if (a > end_ang)
		a = end_ang;
	    x = cx + r * cos(a);
	    y = cy + r * sin(a);
	    Vect_append_point(Points, x, y, 0);
	}
	ang = end_ang;

	if (val[i] != tot_sum)    /* all in one slice, don't draw line to center */
	    Vect_append_point(Points, cx, cy, 0);

	if (!colors[i].none) {
	    D_RGB_color(colors[i].r, colors[i].g, colors[i].b);
	    D_polygon_abs(Points->x, Points->y, Points->n_points);
	}

	D_RGB_color(ocolor->r, ocolor->g, ocolor->b);
	D_polyline_abs(Points->x, Points->y, Points->n_points);
    }

    Vect_destroy_line_struct(Points);

    return 0;
}
