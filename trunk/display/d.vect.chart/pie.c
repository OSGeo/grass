#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/display.h>
#include <grass/symbol.h>
#include "global.h"

#define PI  M_PI

static int init = 0;
static double sa = 0.0;
static double ca = 1.0;


/* rotates x and y about the origin (xo,yo) by angle radians */
static void rotate(double *x, double *y, double xo, double yo, int do3d)
{
    double tmpx, tmpy;

    if (!do3d)
	return;

    /* first translate */
    tmpx = *x - xo;
    tmpy = *y - yo;

    /* now rotate */
    *x = tmpx * ca - tmpy * sa;
    *y = tmpx * sa + tmpy * ca;

    /* now translate back */
    *x += xo;
    *y += yo;

    return;
}


int
pie(double cx, double cy, int size, double *val, int ncols, COLOR * ocolor,
    COLOR * colors, int do3d)
{
    int i, j, n;
    double a, end_ang, ang, tot_sum, sum, step, r, rminor;
    double x, y;
    struct line_pnts *Points;

    G_debug(4, "pie(): cx = %f cy = %f", cx, cy);

    Points = Vect_new_line_struct();

    if (!init) {
	sa = sin(-6 / 180.0 * PI);
	ca = cos(-6 / 180.0 * PI);
	init = 1;
    }

    /* Calc sum */
    tot_sum = 0;
    for (i = 0; i < ncols; i++)
	tot_sum += val[i];
    
    if (tot_sum == 0) {
	Vect_destroy_line_struct(Points);
	return 0;    /* nothing to draw */
    }

    step = PI / 180;
    r = (D_d_to_u_col(2) - D_d_to_u_col(1)) * size / 2;	/* do it better */
    rminor = r;

    if (do3d) {
	int first, np;

	rminor = r * 2.0 / 3.0;

	/* Draw lower polygon for each value */
	sum = 0;
	ang = 0;
	for (i = 0; i < ncols; i++) {
	    if (val[i] == 0)
		continue;

	    sum += val[i];

	    end_ang = 2 * PI * sum / tot_sum;

	    if (end_ang <= PI) {
		ang = end_ang;
		continue;
	    }

	    Vect_reset_line(Points);

	    n = (int)ceil((end_ang - ang) / step);

	    /* upper -> lower */
	    a = ang;
	    if (ang < PI)
		a = PI;

	    x = cx + r * cos(a);
	    y = cy + rminor * sin(a);
	    rotate(&x, &y, cx, cy, do3d);
	    Vect_append_point(Points, x, y, 0);

	    /* lower */
	    first = ang < PI ? 1 : 0;
	    for (j = 0, a = ang; j <= n; j++, a += step) {
		if (j == n)
		    a = end_ang;
		if (a > PI) {
		    if (first) {
			x = cx + r * cos(PI);
			y = cy + rminor * sin(PI) - r / 5;
			rotate(&x, &y, cx, cy, do3d);
			Vect_append_point(Points, x, y, 0);
			first = 0;
		    }
		    x = cx + r * cos(a);
		    y = cy + rminor * sin(a) - r / 5;
		    rotate(&x, &y, cx, cy, do3d);
		    Vect_append_point(Points, x, y, 0);
		}
	    }
	    np = Points->n_points + 1;

	    /* upper */
	    first = end_ang > PI ? 1 : 0;
	    for (j = 0, a = end_ang; j <= n; j++, a -= step) {
		if (j == n)
		    a = ang;
		if (a > PI) {
		    x = cx + r * cos(a);
		    y = cy + rminor * sin(a);
		    rotate(&x, &y, cx, cy, do3d);
		    Vect_append_point(Points, x, y, 0);
		}
		else if (first) {
		    x = cx + r * cos(PI);
		    y = cy + rminor * sin(PI);
		    rotate(&x, &y, cx, cy, do3d);
		    Vect_append_point(Points, x, y, 0);
		    first = 0;
		}
	    }

	    ang = end_ang;
	    
	    if (Points->n_points == 0)
		continue;

	    if (!colors[i].none) {
		D_RGB_color(colors[i].r, colors[i].g, colors[i].b);
		D_polygon_abs(Points->x, Points->y, Points->n_points);
	    }

	    D_RGB_color(ocolor->r, ocolor->g, ocolor->b);
	    Points->n_points = np;
	    D_polyline_abs(Points->x, Points->y, Points->n_points);
	}
    }

    /* Draw polygon for each value */
    sum = 0;
    ang = 0;
    for (i = 0; i < ncols; i++) {
	if (val[i] == 0)
	    continue;

	sum += val[i];

	end_ang = 2 * PI * sum / tot_sum;

	Vect_reset_line(Points);

	if (val[i] != tot_sum) {    /* all in one slice, don't draw line to center */
	    x = cx;
	    y = cy;
	    rotate(&x, &y, cx, cy, do3d);
	    Vect_append_point(Points, x, y, 0);
	}

	n = (int)ceil((end_ang - ang) / step);
	for (j = 0, a = ang; j <= n; j++, a += step) {
	    if (a > end_ang)
		a = end_ang;
	    x = cx + r * cos(a);
	    y = cy + rminor * sin(a);
	    rotate(&x, &y, cx, cy, do3d);
	    Vect_append_point(Points, x, y, 0);
	}
	ang = end_ang;

	if (val[i] != tot_sum) {    /* all in one slice, don't draw line to center */
	    x = cx;
	    y = cy;
	    rotate(&x, &y, cx, cy, do3d);
	    Vect_append_point(Points, x, y, 0);
	}

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
