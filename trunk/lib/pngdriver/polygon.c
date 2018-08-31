/*!
  \file lib/pngdriver/polygon.c

  \brief GRASS png display driver - draw polygon

  (C) 2003-2014 by Per Henrik Johansen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Per Henrik Johansen (original contributor)
  \author Glynn Clements  
*/

#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>

#include "path.h"
#include "pngdriver.h"

static int cmp_double(const void *aa, const void *bb)
{
    const double *a = aa;
    const double *b = bb;

    return
	*a > *b ?  1 :
	*a < *b ? -1 :
	0;
}

static void fill(double x0, double x1, double y)
{
    int yi = (int) floor(y);
    int xi0 = (int) floor(x0 + 0.5);
    int xi1 = (int) floor(x1 + 0.5);
    unsigned int *p;
    int x;

    if (yi >= png.clip_bot || yi < png.clip_top)
	return;

    if (xi0 > png.clip_rite)
	return;

    if (xi1 < png.clip_left)
	return;

    if (xi0 < png.clip_left)
	xi0 = png.clip_left;

    if (xi1 > png.clip_rite)
	xi1 = png.clip_rite;

    p = &png.grid[yi * png.width + xi0];

    for (x = xi0; x < xi1; x++)
	*p++ = png.current_color;
}

static void line(const struct vertex *p, int n, double y)
{
    static double *xs;
    static int max_x;
    int num_x = 0;
    int i;

    for (i = 1; i < n; i++) {
	const struct vertex *p0 = &p[i - 1];
	const struct vertex *p1 = &p[i];
	const struct vertex *tmp;
	double x;

	if (p0->y == p1->y)
	    continue;

	if (p0->y > p1->y)
	    tmp = p0, p0 = p1, p1 = tmp;

	if (p0->y > y)
	    continue;

	if (p1->y <= y)
	    continue;

	x = p1->x * (y - p0->y) + p0->x * (p1->y - y);
	x /= p1->y - p0->y;

	if (num_x >= max_x) {
	    max_x += 20;
	    xs = G_realloc(xs, max_x * sizeof(double));
	}

	xs[num_x++] = x;
    }

    qsort(xs, num_x, sizeof(double), cmp_double);

    for (i = 0; i + 1 < num_x; i += 2)
	fill(xs[i], xs[i + 1], y);
}

static void poly(const struct vertex *p, int n)
{
    double y0, y1, y;
    int i;

    if (n < 3)
	return;

    y0 = y1 = p[0].y;

    for (i = 1; i < n; i++) {
	if (y0 > p[i].y)
	    y0 = p[i].y;

	if (y1 < p[i].y)
	    y1 = p[i].y;
    }

    if (y0 > png.clip_bot || y1 < png.clip_top)
	return;

    if (y0 < png.clip_top)
	y0 = png.clip_top;

    if (y1 > png.clip_bot)
	y1 = png.clip_bot;

    for (y = floor(y0 + 0.5) + 0.5; y < y1; y++)
	line(p, n, y);
}

/*!
  \brief Draw polygon
*/
void png_polygon(struct path *p)
{
    if (p->vertices[p->count - 1].mode != P_CLOSE)
	path_close(p);

    poly(p->vertices, p->count);

    png.modified = 1;
}

