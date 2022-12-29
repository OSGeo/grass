
/*
 * draw a line between two given points in the current color.
 *
 * Called by:
 *     Cont_abs() in ../lib/Cont_abs.c
 */

#include <stdlib.h>
#include <math.h>

#include "pngdriver.h"

static void store_xy(double x, double y)
{
    int xi = (int) floor(x);
    int yi = (int) floor(y);

    if (x < png.clip_left || x >= png.clip_rite || y < png.clip_top || y >= png.clip_bot)
	return;

    png.grid[yi * png.width + xi] = png.current_color;
}

static void swap(double *a, double *b)
{
    double t = *a; *a = *b; *b = t;
}

static void draw_line(double x1, double y1, double x2, double y2)
{
    double x, y;
    double dx, dy;

    if (fabs(y1 - y2) > fabs(x1 - x2)) {
	if (y1 > y2) {
	    swap(&y1, &y2);
	    swap(&x1, &x2);
	}

	dy = y2 - y1;
	dx = x2 - x1;

	for (y = floor(y1) + 0.5; y < y2; y++) {
	    x = x1 + (y - y1) * dx / dy;
	    store_xy(x, y);
	}
    }
    else {
	if (x1 > x2) {
	    swap(&x1, &x2);
	    swap(&y1, &y2);
	}

	dx = x2 - x1;
	dy = y2 - y1;

	for (x = floor(x1) + 0.5; x < x2; x++) {
	    y = y1 + (x - x1) * dy / dx;
	    store_xy(x, y);
	}
    }
}

void png_draw_line(double x1, double y1, double x2, double y2)
{
    struct path path;
    struct vertex vertices[5];
    double k = png.linewidth / 2;
    double dx, dy;

    if (png.linewidth <= 1) {
	draw_line(x1, y1, x2, y2);
	png.modified = 1;
	return;
    }

    path.vertices = vertices;
    path.count = 0;
    path.alloc = 5;
    path.start = -1;

    /* FIXME: rendering issues (#1283) */
    dx = fabs(x2 - x1);
    dy = fabs(y2 - y1);

    if (dy > dx) {
	path_move(&path, x1 - k, y1);
	path_cont(&path, x1 + k, y1);
	path_cont(&path, x2 + k, y2);
	path_cont(&path, x2 - k, y2);
	path_close(&path);
    }
    else {
	path_move(&path, x1, y1 - k);
	path_cont(&path, x1, y1 + k);
	path_cont(&path, x2, y2 + k);
	path_cont(&path, x2, y2 - k);
	path_close(&path);
    }

    png_polygon(&path);
}

