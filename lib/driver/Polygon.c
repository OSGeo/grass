#include <stdlib.h>
#include <grass/gis.h>
#include "driver.h"
#include "driverlib.h"

struct point
{
    double x, y;
};

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
    COM_Box_abs(x0, y, x1, y + 1);
}

static void line(const struct point *p, int n, double y)
{
    static double *xs;
    static double max_x;
    int num_x = 0;
    int i;

    for (i = 0; i < n; i++) {
	const struct point *p0 = &p[i];
	const struct point *p1 = &p[i + 1];
	const struct point *tmp;
	double fx;
	long x;

	if (p0->y == p1->y)
	    continue;

	if (p0->y > p1->y)
	    tmp = p0, p0 = p1, p1 = tmp;

	if (p0->y > y)
	    continue;

	if (p1->y <= y)
	    continue;

	fx = (double)p1->x * (y - p0->y) + (double)p0->x * (p1->y - y);
	fx /= p1->y - p0->y;
	x = fx < -0x7fffffff ? -0x7fffffff :
	    fx > 0x7fffffff ? 0x7fffffff : (long)fx;

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

static void poly(const struct point *p, int n)
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

    if (y0 > screen_bottom || y1 < screen_top)
	return;

    if (y0 < screen_top)
	y0 = screen_top;

    if (y1 > screen_bottom)
	y1 = screen_bottom;

    for (y = y0; y < y1; y++)
	line(p, n, y);
}

static void fill_polygon(const double *xarray, const double *yarray, int count)
{
    static struct point *points;
    static int max_points;
    int i;

    if (max_points < count + 1) {
	max_points = count + 1;
	points = G_realloc(points, sizeof(struct point) * max_points);
    }

    for (i = 0; i < count; i++) {
	points[i].x = xarray[i];
	points[i].y = yarray[i];
    }

    points[count].x = xarray[0];
    points[count].y = yarray[0];

    poly(points, count);
}

void COM_Polygon_abs(const double *xarray, const double *yarray, int number)
{
    if (driver->Polygon) {
	(*driver->Polygon) (xarray, yarray, number);
	return;
    }

    fill_polygon(xarray, yarray, number);
}

void COM_Polygon_rel(const double *xarray, const double *yarray, int number)
{
    static double *xa, *ya;
    static int nalloc;
    int i;

    if (number > nalloc) {
	nalloc = number;
	xa = G_realloc(xa, (size_t) nalloc * sizeof(double));
	ya = G_realloc(ya, (size_t) nalloc * sizeof(double));
    }

    xa[0] = xarray[0] + cur_x;
    ya[0] = yarray[0] + cur_y;

    for (i = 1; i < number; i++) {
	xa[i] = xa[i - 1] + xarray[i];
	ya[i] = ya[i - 1] + yarray[i];
    }

    COM_Polygon_abs(xa, ya, number);
}
