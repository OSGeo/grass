
/*
 * draw a line between two given points in the current color.
 *
 * Called by:
 *     Cont_abs() in ../lib/Cont_abs.c
 */

#include <stdlib.h>

#include "pngdriver.h"

static void store_xy(int x, int y)
{
    if (x < clip_left || x >= clip_rite || y < clip_top || y >= clip_bot)
	return;

    grid[y * width + x] = currentColor;
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

void PNG_draw_line(int x1, int y1, int x2, int y2)
{
    int dx, dy;
    int i;

    if (linewidth <= 1) {
	draw_line(x1, y1, x2, y2);
	modified = 1;
	return;
    }

    dx = abs(x2 - x1);
    dy = abs(y2 - y1);

    for (i = 0; i < linewidth; i++) {
	int k = i - linewidth / 2;

	if (dy > dx)
	    draw_line(x1 + k, y1, x2 + k, y2);
	else
	    draw_line(x1, y1 + k, x2, y2 + k);
    }

    modified = 1;
}
