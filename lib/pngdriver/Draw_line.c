
/*
 * draw a line between two given points in the current color.
 *
 * Called by:
 *     Cont_abs() in ../lib/Cont_abs.c
 */

#include <stdlib.h>
#include <math.h>

#include "pngdriver.h"

static void store_xy(int x, int y)
{
    if (x < png.clip_left || x >= png.clip_rite || y < png.clip_top || y >= png.clip_bot)
	return;

    png.grid[y * png.width + x] = png.current_color;
}

static void draw_line(int x1, int y1, int x2, int y2)
{
    int x, y, x_end, y_end;
    int xinc, yinc, error;
    int delta_x, delta_y;

    x = x1;
    x_end = x2;
    y = y1;
    y_end = y2;

    if (x == x_end && y == y_end) {
	store_xy(x, y);
	return;
    }

    /* generate equation */
    delta_y = y_end - y;
    delta_x = x_end - x;

    /* figure out which way to move x */
    xinc = 1;
    if (delta_x < 0) {
	delta_x = -delta_x;
	xinc = -1;
    }

    /* figure out which way to move y */
    yinc = 1;
    if (delta_y < 0) {
	delta_y = -delta_y;
	yinc = -1;
    }

    if (delta_x > delta_y) {
	/* always move x, decide when to move y */
	/* initialize the error term, and double delta x and delta y */
	delta_y = delta_y * 2;
	error = delta_y - delta_x;
	delta_x = delta_y - (delta_x * 2);

	while (x != x_end) {

	    store_xy(x, y);

	    if (error > 0) {
		y += yinc;
		error += delta_x;
	    }
	    else
		error += delta_y;

	    x += xinc;
	}
    }
    else {
	/* always move y, decide when to move x */
	/* initialize the error term, and double delta x and delta y */
	delta_x = delta_x * 2;
	error = delta_x - delta_y;
	delta_y = delta_x - (delta_y * 2);

	while (y != y_end) {

	    store_xy(x, y);

	    if (error > 0) {
		x += xinc;
		error += delta_y;
	    }
	    else
		error += delta_x;

	    y += yinc;
	}
    }

    store_xy(x, y);
}

void PNG_draw_line(double fx1, double fy1, double fx2, double fy2)
{
    int x1 = (int) floor(fx1 + 0.5);
    int y1 = (int) floor(fy1 + 0.5);
    int x2 = (int) floor(fx2 + 0.5);
    int y2 = (int) floor(fy2 + 0.5);
    int dx, dy;
    int i;

    if (png.linewidth <= 1) {
	draw_line(x1, y1, x2, y2);
	png.modified = 1;
	return;
    }

    dx = abs(x2 - x1);
    dy = abs(y2 - y1);

    for (i = 0; i < png.linewidth; i++) {
	int k = i - png.linewidth / 2;

	if (dy > dx)
	    draw_line(x1 + k, y1, x2 + k, y2);
	else
	    draw_line(x1, y1 + k, x2, y2 + k);
    }

    png.modified = 1;
}
