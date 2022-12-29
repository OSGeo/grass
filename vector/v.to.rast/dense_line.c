#include <math.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "local.h"

static struct state {
    struct Cell_head window;
    double xconv, yconv;
    double left, right, top, bottom;
    int ymin, ymax;
    int dotted_fill_gap;

    int (*dot)(int, int);
} state;

static struct state *st = &state;

#define X(e) (st->left + st->xconv * ((e) - st->window.west))
#define Y(n) (st->top + st->yconv * (st->window.north - (n)))

#define EAST(x) (st->window.west + ((x)-st->left)/st->xconv)
#define NORTH(y) (st->window.north - ((y)-st->top)/st->yconv)

void dense_line(double x1, double y1, double x2, double y2,
                int (*point) (int, int));

static int ifloor(double x)
{
    int i;

    i = (int)x;
    if (i > x)
	i--;
    return i;
}

static int iceil(double x)
{
    int i;

    i = (int)x;
    if (i < x)
	i++;
    return i;
}


void setup_plot(double t, double b, double l, double r,
		  int (*dot) (int, int))
{
    G_get_set_window(&st->window);

    st->left = l;
    st->right = r;
    st->top = t;
    st->bottom = b;

    st->xconv = (st->right - st->left) / (st->window.east - st->window.west);
    st->yconv = (st->bottom - st->top) / (st->window.north - st->window.south);

    if (st->top < st->bottom) {
	st->ymin = iceil(st->top);
	st->ymax = ifloor(st->bottom);
    }
    else {
	st->ymin = iceil(st->bottom);
	st->ymax = ifloor(st->top);
    }

    st->dot = dot;
}


/* dense line plotting, alternative to G_plot_line2()
 * x1, y1, x2, y2 are col, row numbers */
void plot_line_dense(double east1, double north1, double east2, double north2)
{
    double x1, x2, y1, y2;

    y1 = Y(north1);
    y2 = Y(north2);

    if (st->window.proj == PROJECTION_LL) {
	if (east1 > east2)
	    while ((east1 - east2) > 180)
		east2 += 360;
	else if (east2 > east1)
	    while ((east2 - east1) > 180)
		east1 += 360;
	while (east1 > st->window.east) {
	    east1 -= 360.0;
	    east2 -= 360.0;
	}
	while (east1 < st->window.west) {
	    east1 += 360.0;
	    east2 += 360.0;
	}
	x1 = X(east1);
	x2 = X(east2);

	dense_line(x1, y1, x2, y2, st->dot);

	if (east2 > st->window.east || east2 < st->window.west) {
	    while (east2 > st->window.east) {
		east1 -= 360.0;
		east2 -= 360.0;
	    }
	    while (east2 < st->window.west) {
		east1 += 360.0;
		east2 += 360.0;
	    }
	    x1 = X(east1);
	    x2 = X(east2);
	    dense_line(x1, y1, x2, y2, st->dot);
	}
    }
    else {
	x1 = X(east1);
	x2 = X(east2);
	dense_line(x1, y1, x2, y2, st->dot);
    }
}

/* dense line plotting, alternative to G_bresenham_line()
 * x1, y1, x2, y2 are col, row numbers */
void dense_line(double x1, double y1, double x2, double y2,
                int (*point) (int, int))
{
    int ix1, ix2, iy1, iy2, idx, idy;
    int xinc, yinc;
    double dx, dy;

    G_debug(2, "dense line");

    if (x2 < x1) {
	double tmp;
	
	tmp = x1;
	x1 = x2;
	x2 = tmp;
	
	tmp = y1;
	y1 = y2;
	y2 = tmp;
    }

    ix1 = (int)x1;
    ix2 = (int)x2;
    iy1 = (int)y1;
    iy2 = (int)y2;

    idx = ix2 - ix1;
    idy = iy2 - iy1;

    dx = x2 - x1;
    dy = y2 - y1;
    
    xinc = yinc = 1;
    
    if (idx < 0) {
	xinc = -1;
	idx = -idx;
    }

    if (idy < 0) {
	yinc = -1;
	idy = -idy;
    }
    if (dx < 0)
	dx = -dx;
    if (dy < 0)
	dy = -dy;

    if (idx == 0) {
	while (iy1 != iy2) {
	    point(ix1, iy1);
	    iy1 += yinc;
	}
    }
    else if (idy == 0) {
	while (ix1 != ix2) {
	    point(ix1, iy1);
	    ix1 += xinc;
	}
    }
    else if (dx >= dy) {
	double m, a, yi;
	int xnext;

	m = (y2 - y1) / (x2 - x1);
	a = y1 - m * x1;
	
	/* find x for y = iy1 or y = iy1 + 1 */
	m = (x2 - x1) / (y2 - y1);
	a = x1 - m * y1;
	yi = iy1;
	if (iy1 < iy2)
	    yi += 1;
	xnext = a + m * yi;
	

	while (ix1 != ix2) {
	    point(ix1, iy1);

	    if (ix1 == xnext) {
		iy1 += yinc;
		point(ix1, iy1);
		if (iy1 != iy2) {
		    yi = iy1;
		    if (iy1 < iy2)
			yi += 1;
		    xnext = a + m * yi;
		}
		else
		    xnext = ix2;
	    }

	    ix1 += xinc;
	}
	if (iy1 != iy2)
	    point(ix1, iy1);
    }
    else if (dx < dy) {
	double m, a, xi;
	int ynext;

	if (y2 < y1) {
	    double tmp;
	    
	    tmp = x1;
	    x1 = x2;
	    x2 = tmp;
	    
	    tmp = y1;
	    y1 = y2;
	    y2 = tmp;

	    ix1 = (int)x1;
	    ix2 = (int)x2;
	    iy1 = (int)y1;
	    iy2 = (int)y2;

	    yinc = 1;
	    xinc = 1;
	    if (x2 < x1)
		xinc = -1;
	}

	/* find y for x = ix1 or x = ix1 + 1 */
	m = (y2 - y1) / (x2 - x1);
	a = y1 - m * x1;
	xi = ix1;
	if (ix1 < ix2)
	    xi += 1;
	ynext = a + m * xi;

	while (iy1 != iy2) {
	    point(ix1, iy1);

	    if (iy1 == ynext) {
		ix1 += xinc;
		point(ix1, iy1);
		if (ix1 != ix2) {
		    xi = ix1;
		    if (ix1 < ix2)
			xi += 1;
		    ynext = a + m * xi;
		}
		else
		    ynext = iy2;
	    }
	    iy1 += yinc;
	}
	if (ix1 != ix2)
	    point(ix1, iy1);
    }
    point(ix2, iy2);
}
