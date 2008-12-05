
#include <math.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "driverlib.h"

struct rectangle
{
    double left;
    double rite;
    double bot;
    double top;
};

struct vector
{
    double x, y;
};

static struct vector cur;

static struct rectangle clip;

static int window_set;

#define min(x,y) ((x) < (y) ? (x) : (y))
#define max(x,y) ((x) > (y) ? (x) : (y))

static double *xi, *yi;
static int nalloc_i;

static double *xf, *yf;
static int nalloc_f;

/******************************************************************************/

static void alloc_dst(int n)
{

    if (nalloc_i >= n)
	return;

    nalloc_i = n;
    xi = G_realloc(xi, nalloc_i * sizeof(double));
    yi = G_realloc(yi, nalloc_i * sizeof(double));
}

static void alloc_src(int n)
{

    if (nalloc_f >= n)
	return;

    nalloc_f = n + 10;
    xf = G_realloc(xf, nalloc_f * sizeof(double));
    yf = G_realloc(yf, nalloc_f * sizeof(double));
}

static void dealloc_src(const double **x, const double **y, int release)
{
    if (release) {
	G_free(*(double **)x);
	G_free(*(double **)y);
    }

    *x = xf;
    *y = yf;

    nalloc_f = 0;

    xf = NULL;
    yf = NULL;
}

static int do_reduce(double *x, double *y, int n)
{
    static double eps = 0.5;
    int i, j;

    for (i = 0, j = 1; j < n; j++) {
	if (fabs(x[j] - x[i]) < eps && fabs(y[j] - y[i]) < eps)
	    continue;
	i++;
	if (i == j)
	    continue;
	x[i] = x[j];
	y[i] = y[j];
    }
    return i + 1;
}

static int do_convert(const double *x, const double *y, int n)
{
    int i;

    alloc_dst(n);

    for (i = 0; i < n; i++) {
	xi[i] = D_u_to_d_col(x[i]);
	yi[i] = D_u_to_d_row(y[i]);
    }

    return do_reduce(xi, yi, n);
}

static void rel_to_abs(const double **px, const double **py, int n)
{
    const double *x = *px;
    const double *y = *py;
    int i;

    alloc_src(n);

    xf[0] = cur.x + x[0];
    yf[0] = cur.y + y[0];

    for (i = 1; i < n; i++) {
	xf[i] = xf[i-1] + x[i];
	yf[i] = yf[i-1] + y[i];
    }

    dealloc_src(px, py, 0);
}

static int shift_count(double dx)
{
    return (int)floor(dx / 360);
}

static double shift_angle(double dx)
{
    return shift_count(dx) * 360;
}

static double coerce(double x)
{
    x += 180;
    x -= shift_angle(x);
    x -= 180;
    return x;
}

static int euclidify(double *x, const double *y, int n, int no_pole)
{
    double ux0 = clip.left;
    double ux1 = clip.rite;
    double x0, x1;
    int lo, hi, count;
    int i;

    x0 = x1 = x[0];

    for (i = 1; i < n; i++) {
	if (fabs(y[i]) < 89.9)
	    x[i] = x[i - 1] + coerce(x[i] - x[i - 1]);

	x0 = min(x0, x[i]);
	x1 = max(x1, x[i]);
    }

    if (no_pole && fabs(x[n - 1] - x[0]) > 180)
	return 0;

    lo = -shift_count(ux1 - x0);
    hi = shift_count(x1 - ux0);
    count = hi - lo + 1;

    for (i = 0; i < n; i++)
	x[i] -= lo * 360;

    return count;
}

static void ll_wrap_path(const double *x, const double *y, int n,
			 void (*func) (const double *, const double *, int))
{
    double *xx = G_malloc(n * sizeof(double));
    int count, i;

    memcpy(xx, x, n * sizeof(double));
    count = euclidify(xx, y, n, 0);

    for (i = 0; i < count; i++) {
	int j;

	(*func) (xx, y, n);

	for (j = 0; j < n; j++)
	    xx[j] -= 360;
    }

    G_free(xx);
}

static void ll_wrap_line(double ax, double ay, double bx, double by,
			 void (*func)(double, double, double, double))
{
    double ux0 = clip.left;
    double ux1 = clip.rite;
    double x0, x1;
    int lo, hi, i;
    int ret;

    bx = ax + coerce(bx - ax);

    x0 = min(ax, bx);
    x1 = max(ax, bx);

    lo = -shift_count(ux1 - x0);
    hi = shift_count(x1 - ux0);

    ret = 0;

    for (i = lo; i <= hi; i++)
	(*func)(ax - i * 360, ay, bx - i * 360, by);
}

/******************************************************************************/

static void D_polydots_raw(const double *x, const double *y, int n)
{
    n = do_convert(x, y, n);
    R_polydots_abs(xi, yi, n);
}

static void D_polyline_raw(const double *x, const double *y, int n)
{
    n = do_convert(x, y, n);
    R_polyline_abs(xi, yi, n);
}

static void D_polygon_raw(const double *x, const double *y, int n)
{
    n = do_convert(x, y, n);
    R_polygon_abs(xi, yi, n);
}

static void D_line_raw(double x1, double y1, double x2, double y2)
{
    x1 = D_u_to_d_col(x1);
    y1 = D_u_to_d_row(y1);
    x2 = D_u_to_d_col(x2);
    y2 = D_u_to_d_row(y2);

    R_line_abs(x1, y1, x2, y2);
}

static void D_box_raw(double x1, double y1, double x2, double y2)
{
    x1 = D_u_to_d_col(x1);
    x2 = D_u_to_d_col(x2);
    y1 = D_u_to_d_row(y1);
    y2 = D_u_to_d_row(y2);

    R_box_abs(x1, y1, x2, y2);
}

/******************************************************************************/

/*!
 * \brief set clipping window
 *
 * Sets the clipping window to the pixel window that corresponds
 * to the current database region. This is the default.
 *
 *  \param top
 *  \param bottom
 *  \param left
 *  \param right
 */

void D_set_clip(double t, double b, double l, double r)
{
    clip.left = min(l, r);
    clip.rite = max(l, r);
    clip.bot = min(b, t);
    clip.top = max(b, t);

    window_set = 1;
}

/*!
 * \brief set clipping window to map window
 *
 * Sets the clipping window to the pixel window that corresponds to the
 * current database region. This is the default.
 *
 *  \param ~
 */

void D_clip_to_map(void)
{
    double t, b, l, r;

    D_get_src(&t, &b, &l, &r);
    D_set_clip(t, b, l, r);
}

void D_line_width(double d)
{
    R_line_width(d > 0 ? d : 0);
}

void D_get_text_box(const char *text, double *t, double *b, double *l, double *r)
{
    double T, B, L, R;

    R_get_text_box(text, &T, &B, &L, &R);

    *t = D_d_to_u_row(T);
    *b = D_d_to_u_row(B);
    *l = D_d_to_u_col(L);
    *r = D_d_to_u_col(R);

    if (*t < *b) {
	double tmp = *t; *t = *b; *b = tmp;
    }

    if (*r < *l) {
	double tmp = *r; *r = *l; *l = tmp;
    }
}

/******************************************************************************/

void D_polydots_abs(const double *x, const double *y, int n)
{
    if (!window_set)
	D_clip_to_map();

    if (D_is_lat_lon())
	ll_wrap_path(x, y, n, D_polydots_raw);
    else
	D_polydots_raw(x, y, n);
}

void D_polyline_abs(const double *x, const double *y, int n)
{
    if (!window_set)
	D_clip_to_map();

    if (n < 2)
	return;

    if (D_is_lat_lon())
	ll_wrap_path(x, y, n, D_polyline_raw);
    else
	D_polyline_raw(x, y, n);
}

void D_polygon_abs(const double *x, const double *y, int n)
{
    if (!window_set)
	D_clip_to_map();

    if (D_is_lat_lon())
	ll_wrap_path(x, y, n, D_polygon_raw);
    else
	D_polygon_raw(x, y, n);
}

void D_line_abs(double x1, double y1, double x2, double y2)
{
    if (!window_set)
	D_clip_to_map();

    if (D_is_lat_lon())
	ll_wrap_line(x1, y1, x2, y2, D_line_raw);
    else
	D_line_raw(x1, y1, x2, y2);
}

void D_box_abs(double x1, double y1, double x2, double y2)
{
    if (!window_set)
	D_clip_to_map();

    if (D_is_lat_lon())
	ll_wrap_line(x1, y1, x2, y2, D_box_raw);
    else
	D_box_raw(x1, y1, x2, y2);
}

/******************************************************************************/

void D_line_rel(double x1, double y1, double x2, double y2)
{
    cur.x += x1;
    cur.y += y1;

    x1 = cur.x;
    y1 = cur.y;

    cur.x += x2;
    cur.y += y2;

    x2 = cur.x;
    y2 = cur.y;

    D_line_abs(x1, y1, x2, y2);
}

void D_polydots_rel(const double *x, const double *y, int n)
{
    rel_to_abs(&x, &y, n);
    D_polydots_abs(x, y, n);
    dealloc_src(&x, &y, 1);
}

void D_polyline_rel(const double *x, const double *y, int n)
{
    rel_to_abs(&x, &y, n);
    D_polyline_abs(x, y, n);
    dealloc_src(&x, &y, 1);
}

void D_polygon_rel(const double *x, const double *y, int n)
{
    rel_to_abs(&x, &y, n);
    D_polygon_abs(x, y, n);
    dealloc_src(&x, &y, 1);
}

void D_box_rel(double x2, double y2)
{
    D_box_abs(cur.x, cur.y, cur.x + x2, cur.y + y2);
}

/******************************************************************************/

static struct path path;

void D_pos_abs(double x, double y)
{
    cur.x = x;
    cur.y = y;

    x = D_u_to_d_col(x);
    y = D_u_to_d_row(y);

    R_pos_abs(x, y);
}

void D_pos_rel(double x, double y)
{
    D_pos_abs(cur.x + x, cur.y + y);
}

void D_move_abs(double x, double y)
{
    path_move(&path, x, y);

    cur.x = x;
    cur.y = y;
}

void D_move_rel(double x, double y)
{
    D_move_abs(cur.x + x, cur.y + y);
}

void D_cont_abs(double x, double y)
{
    path_cont(&path, x, y);

    cur.x = x;
    cur.y = y;
}

void D_cont_rel(double x, double y)
{
    D_cont_abs(cur.x + x, cur.y + y);
}

/******************************************************************************/

void D_begin(void)
{
    path_begin(&path);
}

void D_end(void)
{
}

void D_close(void)
{
    path_close(&path);
}

void D_stroke(void)
{
    path_fill(&path, D_polyline_abs);
}

void D_fill(void)
{
    path_fill(&path, D_polygon_abs);
}

/******************************************************************************/

