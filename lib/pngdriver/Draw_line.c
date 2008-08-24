
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

void PNG_draw_line(double x1, double y1, double x2, double y2)
{
    double ax[4], ay[4];
    double k = png.linewidth / 2;
    int dx, dy;

    if (png.linewidth <= 1) {
	draw_line(x1, y1, x2, y2);
	png.modified = 1;
	return;
    }

    if (dy > dx) {
	ax[0] = x1 - k;	ay[0] = y1;
	ax[1] = x1 + k;	ay[1] = y1;
	ax[2] = x2 + k;	ay[2] = y2;
	ax[3] = x2 - k;	ay[3] = y2;
    }
    else {
	ax[0] = x1;	ay[0] = y1 - k;
	ax[1] = x1;	ay[1] = y1 + k;
	ax[2] = x2;	ay[2] = y2 + k;
	ax[3] = x2;	ay[3] = y2 - k;
    }

    PNG_Polygon(ax, ay, 4);
}

