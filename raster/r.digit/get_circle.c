
/****************************************************************************
 *
 * MODULE:       r.digit
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Interactive tool used to draw and save vector features
 *               on a graphics monitor using a pointing device (mouse)
 *               and save to a raster map.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "local_proto.h"


static int first;
static int prevx, prevy;


int get_circle(FILE * fd, struct Categories *labels)
{
    static struct point		/* used to build 1/4 arc of the circle */
    {
	int x, y;
    } *points = NULL;

    char east[128], north[128];

    int cx, cy, px, py;
    int radius;
    int i, count;
    double r2, atx, aty;

    fprintf(stdout, "Mark the center of the circle\n");
    instructions(0);
    instructions(1);

    cx = cy = -9999;
    if (!get_point(&cx, &cy, east, north))
	return 0;

    px = cx;
    py = cy;
    first = 1;
    do {
	fprintf(stdout, "Mark a point on the perimeter\n");
	if (first)
	    instructions(0);
	instructions(1);
	first = 0;

	if (!get_point(&px, &py, east, north))
	    return 0;

	radius = (int)(hypot((double)(cx - px), (double)(cy - py)) + 0.5);
	if (radius <= 0) {
	    fprintf(stdout, "Circle too small\n");
	    instructions(1);
	}
    }
    while (radius <= 0);

    r2 = (double)radius *radius;

    points = (struct point *)G_realloc(points, sizeof(*points) * radius + 1);

    count = 0;
    for (atx = (double)radius, aty = 0.0; atx >= aty; /*empty */ ) {
	points[count].x = (int)(atx + 0.5);
	points[count].y = (int)(aty + 0.5);
	count++;
	aty += 1.0;
	atx = sqrt(r2 - aty * aty);
    }

    first = 1;
    fprintf(fd, "AREA (circle)\n");

    /* 0 to 45 degrees */
    for (i = 0; i < count; i++)
	draw_and_record(fd, points[i].x, points[i].y, cx, cy);

    /* 45 to 90 degrees */
    for (i = count - 1; i >= 0; i--)
	draw_and_record(fd, points[i].y, points[i].x, cx, cy);

    /* 90 to 135 degrees */
    for (i = 0; i < count; i++)
	draw_and_record(fd, -points[i].y, points[i].x, cx, cy);

    /* 135 to 180 degrees */
    for (i = count - 1; i >= 0; i--)
	draw_and_record(fd, -points[i].x, points[i].y, cx, cy);

    /* 180 to 225 degrees */
    for (i = 0; i < count; i++)
	draw_and_record(fd, -points[i].x, -points[i].y, cx, cy);

    /* 225 to 270 degrees */
    for (i = count - 1; i >= 0; i--)
	draw_and_record(fd, -points[i].y, -points[i].x, cx, cy);

    /* 270 to 315 degrees */
    for (i = 0; i < count; i++)
	draw_and_record(fd, points[i].y, -points[i].x, cx, cy);

    /* 315 to 360 degrees */
    for (i = count - 1; i >= 0; i--)
	draw_and_record(fd, points[i].x, -points[i].y, cx, cy);

    R_flush();

    get_category(fd, "circle", labels);
    return 1;
}

int draw_and_record(FILE * fd, int x, int y, int cx, int cy)
{
    char east[128], north[128];

    x += cx;
    y += cy;
    get_east_north(x, y, east, north);
    fprintf(fd, " %s %s\n", east, north);
    if (!first)
	black_and_white_line(prevx, prevy, x, y);
    first = 0;
    prevx = x;
    prevy = y;
    return 0;
}
