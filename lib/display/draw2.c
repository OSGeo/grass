
/*******************************************************************
 * Line drawing in the current window.
 *
 * Clip window:
 *   D_set_clip_window (top, bottom ,left, right)
 *      establish clipping region for subseqent line drawing.
 *   D_set_clip_window_to_map_window ()
 *     set clipping to pixels corresponding to the current map region
 *     (default)
 *   D_set_clip_window_to_screen_window ()
 *     set clipping to full extent of the window (ie disables clipping on screen)
 *
 * Moves.
 *   D_move_abs(x,y)   move to x,y.
 *   D_move_rel(x,y)   move to +x,+y.
 *      Set current position. Position is not clipped.
 *
 * Draw line 
 *   D_cont_abs(x,y)   draw to x,y.
 *   D_cont_rel(x,y)   draw to +x,+y.
 *      Line draw from current position. New postion is not clipped.
 *      The lines drawn are clipped however.
 *      Return values indicate the nature of the clipping:
 *        0 no clipping
 *        1 part of the line is drawn
 *       -1 none of the line is drawn
 *   
 *
 */

#include <math.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>

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

struct plane
{
    double x, y, k;
};

static struct vector cur;

static struct rectangle clip;

static struct plane pl_left = { -1, 0, 0 };
static struct plane pl_rite = { 1, 0, 0 };
static struct plane pl_bot = { 0, -1, 0 };
static struct plane pl_top = { 0, 1, 0 };

static int window_set;

#define min(x,y) ((x) < (y) ? (x) : (y))
#define max(x,y) ((x) > (y) ? (x) : (y))

#define round(x) ((int) floor(0.5 + (x)))

static int *xi, *yi;
static int nalloc_i;

static double *xf, *yf;
static int nalloc_f;

static void alloc_int(int n)
{

    if (nalloc_i >= n)
	return;

    nalloc_i = n;
    xi = G_realloc(xi, nalloc_i * sizeof(int));
    yi = G_realloc(yi, nalloc_i * sizeof(int));
}

static void alloc_float(int n)
{

    if (nalloc_f >= n)
	return;

    nalloc_f = n + 10;
    xf = G_realloc(xf, nalloc_f * sizeof(double));
    yf = G_realloc(yf, nalloc_f * sizeof(double));
}

static void dealloc_float(const double **x, const double **y, int release)
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

static int do_filter(int *x, int *y, int n)
{
    int i, j;

    for (i = 0, j = 1; j < n; j++) {
	if (x[j] == x[i] && y[j] == y[i])
	    continue;
	i++;
	if (i == j)
	    continue;
	x[i] = x[j];
	y[i] = y[j];
    }

    return i + 1;
}

static void do_round(const double *x, const double *y, int n)
{
    int i;

    alloc_int(n);

    for (i = 0; i < n; i++) {
	xi[i] = round(D_u_to_d_col(x[i]));
	yi[i] = round(D_u_to_d_row(y[i]));
    }
}

static void do_floor(const double *x, const double *y, int n)
{
    int i;

    alloc_int(n);

    for (i = 0; i < n; i++) {
	xi[i] = floor(D_u_to_d_col(x[i]));
	yi[i] = floor(D_u_to_d_row(y[i]));
    }
}

static double dist_plane(double x, double y, const struct plane *p)
{
    return x * p->x + y * p->y + p->k;
}

static double interpolate(double a, double b, double ka, double kb)
{
    return (a * kb - b * ka) / (kb - ka);
}

static int clip_plane(struct vector *a, struct vector *b,
		      const struct plane *p, int *clipped)
{
    double ka = dist_plane(a->x, a->y, p);
    double kb = dist_plane(b->x, b->y, p);
    double kab;

    /* both outside */
    if (ka > 0 && kb > 0)
	return 1;

    /* both inside */
    if (ka <= 0 && kb <= 0)
	return 0;

    *clipped = 1;

    /* a outside - swap a and b */
    if (ka >= 0) {
	struct vector *t;
	double kt;

	t = a;
	a = b;
	b = t;
	kt = ka;
	ka = kb;
	kb = kt;
    }

    kab = kb - ka;

    b->x = interpolate(a->x, b->x, ka, kb);
    b->y = interpolate(a->y, b->y, ka, kb);

    return 0;
}

static int do_clip(struct vector *a, struct vector *b)
{
    int clipped = 0;

    if (a->x < clip.left && b->x < clip.left)
	return -1;
    if (a->x > clip.rite && b->x > clip.rite)
	return -1;
    if (a->y < clip.bot && b->y < clip.bot)
	return -1;
    if (a->y > clip.top && b->y > clip.top)
	return -1;

    if (clip_plane(a, b, &pl_left, &clipped))
	return -1;
    if (clip_plane(a, b, &pl_rite, &clipped))
	return -1;
    if (clip_plane(a, b, &pl_bot, &clipped))
	return -1;
    if (clip_plane(a, b, &pl_top, &clipped))
	return -1;

    return clipped;
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

static void do_ll_wrap(const double *x, const double *y, int n,
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

    pl_left.k = clip.left;
    pl_rite.k = -clip.rite;
    pl_bot.k = clip.bot;
    pl_top.k = -clip.top;

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
    D_set_clip(D_get_u_north(),
	       D_get_u_south(), D_get_u_west(), D_get_u_east());
}

/*!
 * \brief move to pixel
 *
 * Move without drawing to
 * pixel location <b>x,y</b>, even if it falls outside the clipping window.
 *
 *  \param x
 *  \param y
 */

void D_move_clip(double x, double y)
{
    cur.x = x;
    cur.y = y;
}

/*!
 * \brief line to x,y
 *
 * Draws a line from the
 * current position to pixel location <b>x,y.</b> Any part of the line that
 * falls outside the clipping window is not drawn.
 * <b>Note.</b> The new position is <b>x,y</b>, even if it falls outside the
 * clipping window. Returns 0 if the line was contained entirely in the clipping
 * window, 1 if the line had to be clipped to draw it.
 *
 *  \param x
 *  \param y
 *  \return int
 */

static int line_clip(double x1, double y1, double x2, double y2)
{
    struct vector a, b;
    int clipped;

    a.x = x1;
    a.y = y1;

    b.x = x2;
    b.y = y2;

    clipped = do_clip(&a, &b);

    if (clipped >= 0) {
	int x1 = round(D_u_to_d_col(a.x));
	int y1 = round(D_u_to_d_row(a.y));
	int x2 = round(D_u_to_d_col(b.x));
	int y2 = round(D_u_to_d_row(b.y));

	R_move_abs(x1, y1);
	R_cont_abs(x2, y2);
    }

    return clipped;
}

static int line_clip_ll(double ax, double ay, double bx, double by)
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
	ret |= line_clip(ax + i * 360, ay, bx + i * 360, by);

    return ret;
}

int D_cont_clip(double x, double y)
{
    int ret;

    if (!window_set)
	D_clip_to_map();

    if (D_is_lat_lon())
	ret = line_clip_ll(cur.x, cur.y, x, y);
    else
	ret = line_clip(cur.x, cur.y, x, y);

    cur.x = x;
    cur.y = y;

    return ret;
}

void D_polydots_clip(const double *x, const double *y, int n)
{
    double ux0 = clip.left;
    int i, j;

    if (!window_set)
	D_clip_to_map();

    alloc_float(n);

    for (i = j = 0; i < n; i++) {
	double xx = x[i];
	double yy = y[i];

	if (D_is_lat_lon())
	    xx -= shift_angle(x[i] - ux0);

	if (xx < clip.left || xx > clip.rite)
	    continue;
	if (yy < clip.bot || yy > clip.top)
	    continue;

	xf[j] = xx;
	yf[j] = yy;
	j++;
    }

    do_floor(xf, yf, n);
    n = do_filter(xi, yi, n);

    R_polydots_abs(xi, yi, j);
}

static int cull_polyline_plane(int *pn, const double *x, const double *y,
			       const struct plane *p)
{
    int n = *pn;
    int last = -1;
    int prev = 0;
    double x0 = x[prev];
    double y0 = y[prev];
    double d0 = dist_plane(x0, y0, p);
    int i, j;

    for (i = 0, j = 0; i < n; i++) {
	double x1 = x[i];
	double y1 = y[i];
	double d1 = dist_plane(x1, y1, p);
	int in0 = d0 <= 0;
	int in1 = d1 <= 0;

	if (!in0 && in1 && last != prev) {	/* entering */
	    alloc_float(j + 1);
	    xf[j] = x0;
	    yf[j] = y0;
	    j++;
	    last = prev;
	}

	if (in1 || in0) {	/* inside or leaving */
	    alloc_float(j + 1);
	    xf[j] = x1;
	    yf[j] = y1;
	    j++;
	    last = i;
	}

	x0 = x1;
	y0 = y1;
	d0 = d1;
	prev = i;
    }

    *pn = j;

    return (j == 0);
}

static void polyline_cull(const double *x, const double *y, int n)
{
    alloc_float(n + 10);

    if (cull_polyline_plane(&n, x, y, &pl_left))
	return;

    dealloc_float(&x, &y, 0);

    if (cull_polyline_plane(&n, x, y, &pl_rite))
	return;

    dealloc_float(&x, &y, 1);

    if (cull_polyline_plane(&n, x, y, &pl_bot))
	return;

    dealloc_float(&x, &y, 1);

    if (cull_polyline_plane(&n, x, y, &pl_top))
	return;

    dealloc_float(&x, &y, 1);

    do_floor(x, y, n);
    n = do_filter(xi, yi, n);

    R_polyline_abs(xi, yi, n);
}

void D_polyline_cull(const double *x, const double *y, int n)
{
    if (n < 2)
	return;

    if (!window_set)
	D_clip_to_map();

    if (D_is_lat_lon())
	do_ll_wrap(x, y, n, polyline_cull);
    else
	polyline_cull(x, y, n);
}

static void polyline_clip(const double *x, const double *y, int n)
{
    int i;

    for (i = 1; i < n; i++)
	line_clip(x[i - 1], y[i - 1], x[i], y[i]);
}

void D_polyline_clip(const double *x, const double *y, int n)
{
    if (n < 2)
	return;

    if (!window_set)
	D_clip_to_map();

    if (D_is_lat_lon())
	do_ll_wrap(x, y, n, polyline_clip);
    else
	polyline_clip(x, y, n);
}

static int cull_polygon_plane(int *pn, const double *x, const double *y,
			      const struct plane *p)
{
    int n = *pn;
    int last = -1;
    int prev = n - 1;
    double x0 = x[prev];
    double y0 = y[prev];
    double d0 = dist_plane(x0, y0, p);
    int i, j;

    for (i = j = 0; i < n; i++) {
	double x1 = x[i];
	double y1 = y[i];
	double d1 = dist_plane(x1, y1, p);
	int in0 = d0 <= 0;
	int in1 = d1 <= 0;

	if (!in0 && in1 && last != prev) {	/* entering */
	    alloc_float(j + 1);
	    xf[j] = x0;
	    yf[j] = y0;
	    j++;
	    last = prev;
	}

	if (in1 || in0) {	/* inside or leaving */
	    alloc_float(j + 1);
	    xf[j] = x1;
	    yf[j] = y1;
	    j++;
	    last = i;
	}

	x0 = x1;
	y0 = y1;
	d0 = d1;
	prev = i;
    }

    *pn = j;

    return (j == 0);
}

static void polygon_cull(const double *x, const double *y, int n)
{
    alloc_float(n + 10);

    if (cull_polygon_plane(&n, x, y, &pl_left))
	return;

    dealloc_float(&x, &y, 0);

    if (cull_polygon_plane(&n, x, y, &pl_rite))
	return;

    dealloc_float(&x, &y, 1);

    if (cull_polygon_plane(&n, x, y, &pl_bot))
	return;

    dealloc_float(&x, &y, 1);

    if (cull_polygon_plane(&n, x, y, &pl_top))
	return;

    dealloc_float(&x, &y, 1);

    do_round(x, y, n);
    n = do_filter(xi, yi, n);

    R_polygon_abs(xi, yi, n);
}

void D_polygon_cull(const double *x, const double *y, int n)
{
    if (!window_set)
	D_clip_to_map();

    if (D_is_lat_lon())
	do_ll_wrap(x, y, n, polygon_cull);
    else
	polygon_cull(x, y, n);
}

static int clip_polygon_plane(int *pn, const double *x, const double *y,
			      const struct plane *p)
{
    int n = *pn;
    double x0 = x[n - 1];
    double y0 = y[n - 1];
    double d0 = dist_plane(x0, y0, p);
    int i, j;

    for (i = j = 0; i < n; i++) {
	double x1 = x[i];
	double y1 = y[i];
	double d1 = dist_plane(x1, y1, p);
	int in0 = d0 <= 0;
	int in1 = d1 <= 0;

	if (in0 != in1) {	/* edge crossing */
	    alloc_float(j + 1);
	    xf[j] = interpolate(x0, x1, d0, d1);
	    yf[j] = interpolate(y0, y1, d0, d1);
	    j++;
	}

	if (in1) {		/* point inside */
	    alloc_float(j + 1);
	    xf[j] = x[i];
	    yf[j] = y[i];
	    j++;
	}

	x0 = x1;
	y0 = y1;
	d0 = d1;
    }

    *pn = j;

    return (j == 0);
}

static void polygon_clip(const double *x, const double *y, int n)
{
    alloc_float(n + 10);

    if (clip_polygon_plane(&n, x, y, &pl_left))
	return;

    dealloc_float(&x, &y, 0);

    if (clip_polygon_plane(&n, x, y, &pl_rite))
	return;

    dealloc_float(&x, &y, 1);

    if (clip_polygon_plane(&n, x, y, &pl_bot))
	return;

    dealloc_float(&x, &y, 1);

    if (clip_polygon_plane(&n, x, y, &pl_top))
	return;

    dealloc_float(&x, &y, 1);

    do_round(x, y, n);
    n = do_filter(xi, yi, n);

    R_polygon_abs(xi, yi, n);
}

void D_polygon_clip(const double *x, const double *y, int n)
{
    if (!window_set)
	D_clip_to_map();

    if (D_is_lat_lon())
	do_ll_wrap(x, y, n, polygon_clip);
    else
	polygon_clip(x, y, n);
}

static void box_clip(double x1, double y1, double x2, double y2)
{
    double t, b, l, r;
    int ti, bi, li, ri;

    l = max(clip.left, min(x1, x2));
    r = min(clip.rite, max(x1, x2));
    b = max(clip.bot, min(y1, y2));
    t = min(clip.top, max(y1, y2));

    li = round(D_u_to_d_col(l));
    ri = round(D_u_to_d_col(r));
    bi = round(D_u_to_d_row(b));
    ti = round(D_u_to_d_row(t));

    R_box_abs(li, ti, ri, bi);
}

static void box_clip_ll(double x1, double y1, double x2, double y2)
{
    double ux0 = clip.left;
    double ux1 = clip.rite;
    int lo, hi, i;

    x2 = x1 + coerce(x2 - x1);

    lo = -shift_count(ux1 - x1);
    hi = shift_count(x2 - ux0);

    for (i = lo; i <= hi; i++)
	box_clip(x1 + i * 360, y1, x2 + i * 360, y2);
}

void D_box_clip(double x1, double y1, double x2, double y2)
{
    if (!window_set)
	D_clip_to_map();

    if (D_is_lat_lon())
	box_clip_ll(x1, y1, x2, y2);
    else
	box_clip(x1, y1, x2, y2);
}

void D_move(double x, double y)
{
    int xi = round(D_u_to_d_col(x));
    int yi = round(D_u_to_d_row(y));

    R_move_abs(xi, yi);
}

void D_cont(double x, double y)
{
    int xi = round(D_u_to_d_col(x));
    int yi = round(D_u_to_d_row(y));

    R_cont_abs(xi, yi);
}

void D_polydots(const double *x, const double *y, int n)
{
    do_floor(x, y, n);
    n = do_filter(xi, yi, n);
    R_polydots_abs(xi, yi, n);
}

void D_polyline(const double *x, const double *y, int n)
{
    do_floor(x, y, n);
    n = do_filter(xi, yi, n);
    R_polyline_abs(xi, yi, n);
}

void D_polygon(const double *x, const double *y, int n)
{
    do_round(x, y, n);
    n = do_filter(xi, yi, n);
    R_polygon_abs(xi, yi, n);
}

void D_box(double x1, double y1, double x2, double y2)
{
    double l = min(x1, x2);
    double r = max(x1, x2);
    double b = min(y1, y2);
    double t = max(y1, y2);
    int li = round(D_u_to_d_col(l));
    int ri = round(D_u_to_d_col(r));
    int bi = round(D_u_to_d_row(b));
    int ti = round(D_u_to_d_row(t));

    R_box_abs(li, ti, ri, bi);
}

void D_line_width(double d)
{
    int w = round(d);

    if (w < 0)
	w = 0;

    R_line_width(w);
}
