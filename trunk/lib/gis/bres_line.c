/*
 * \file lib/gis/bres_line.c
 *
 * \brief GIS Library - Bresenham line routines.
 *
 * (C) 2001-2014 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <grass/gis.h>

/*!
 * \brief Bresenham line algorithm.
 *
 * Draws a line from <i>x1,y1</i> to <i>x2,y2</i> using Bresenham's 
 * algorithm. A routine to plot points must be provided, as is defined 
 * as: point(x, y) plot a point at x,y.
 *
 * This routine does not require a previous call to G_setup_plot() to
 * function correctly, and is independent of all following routines.
 *
 * \param x0,y0 first point
 * \param x1,y1 end point
 * \param point pointer to point plotting function
 */
void G_bresenham_line(int x0, int y0, int x1, int y1, int (*point) (int, int))
{
    int dx, dy;
    int xinc, yinc;

    int res1;
    int res2;

    xinc = 1;
    yinc = 1;
    if ((dx = x1 - x0) < 0) {
	xinc = -1;
	dx = -dx;
    }

    if ((dy = y1 - y0) < 0) {
	yinc = -1;
	dy = -dy;
    }
    res1 = 0;
    res2 = 0;

    if (dx > dy) {
	while (x0 != x1) {
	    point(x0, y0);
	    if (res1 > res2) {
		res2 += dx - res1;
		res1 = 0;
		y0 += yinc;
	    }
	    res1 += dy;
	    x0 += xinc;
	}
    }
    else if (dx < dy) {
	while (y0 != y1) {
	    point(x0, y0);
	    if (res1 > res2) {
		res2 += dy - res1;
		res1 = 0;
		x0 += xinc;
	    }
	    res1 += dx;
	    y0 += yinc;
	}
    }
    else {
	while (x0 != x1) {
	    point(x0, y0);
	    y0 += yinc;
	    x0 += xinc;
	}
    }

    point(x1, y1);
}
