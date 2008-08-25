
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

static double *xi, *yi;
static int nalloc_i;

static double *xf, *yf;
static int nalloc_f;

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
    double t, b, l, r;

    D_get_src(&t, &b, &l, &r);
    D_set_clip(t, b, l, r);
}

void D_line_width(double d)
{
    R_line_width(d > 0 ? d : 0);
}

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
	double x1 = D_u_to_d_col(a.x);
	double y1 = D_u_to_d_row(a.y);
	double x2 = D_u_to_d_col(b.x);
	double y2 = D_u_to_d_row(b.y);

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
	    alloc_src(j + 1);
	    xf[j] = x0;
	    yf[j] = y0;
	    j++;
	    last = prev;
	}

	if (in1 || in0) {	/* inside or leaving */
	    alloc_src(j + 1);
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
    alloc_src(n + 10);

    if (cull_polyline_plane(&n, x, y, &pl_left))
	return;

    dealloc_src(&x, &y, 0);

    if (cull_polyline_plane(&n, x, y, &pl_rite))
	return;

    dealloc_src(&x, &y, 1);

    if (cull_polyline_plane(&n, x, y, &pl_bot))
	return;

    dealloc_src(&x, &y, 1);

    if (cull_polyline_plane(&n, x, y, &pl_top))
	return;

    dealloc_src(&x, &y, 1);

    n = do_convert(x, y, n);

    R_polyline_abs(xi, yi, n);
}

static void polyline_clip(const double *x, const double *y, int n)
{
    int i;

    for (i = 1; i < n; i++)
	line_clip(x[i - 1], y[i - 1], x[i], y[i]);
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
	    alloc_src(j + 1);
	    xf[j] = x0;
	    yf[j] = y0;
	    j++;
	    last = prev;
	}

	if (in1 || in0) {	/* inside or leaving */
	    alloc_src(j + 1);
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
    alloc_src(n + 10);

    if (cull_polygon_plane(&n, x, y, &pl_left))
	return;

    dealloc_src(&x, &y, 0);

    if (cull_polygon_plane(&n, x, y, &pl_rite))
	return;

    dealloc_src(&x, &y, 1);

    if (cull_polygon_plane(&n, x, y, &pl_bot))
	return;

    dealloc_src(&x, &y, 1);

    if (cull_polygon_plane(&n, x, y, &pl_top))
	return;

    dealloc_src(&x, &y, 1);

    n = do_convert(x, y, n);

    R_polygon_abs(xi, yi, n);
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
	    alloc_src(j + 1);
	    xf[j] = interpolate(x0, x1, d0, d1);
	    yf[j] = interpolate(y0, y1, d0, d1);
	    j++;
	}

	if (in1) {		/* point inside */
	    alloc_src(j + 1);
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
    alloc_src(n + 10);

    if (clip_polygon_plane(&n, x, y, &pl_left))
	return;

    dealloc_src(&x, &y, 0);

    if (clip_polygon_plane(&n, x, y, &pl_rite))
	return;

    dealloc_src(&x, &y, 1);

    if (clip_polygon_plane(&n, x, y, &pl_bot))
	return;

    dealloc_src(&x, &y, 1);

    if (clip_polygon_plane(&n, x, y, &pl_top))
	return;

    dealloc_src(&x, &y, 1);

    n = do_convert(x, y, n);

    R_polygon_abs(xi, yi, n);
}

static void box_clip(double x1, double y1, double x2, double y2)
{
    x1 = max(clip.left, min(clip.rite, x1));
    x2 = max(clip.left, min(clip.rite, x2));
    y1 = max(clip.top, min(clip.bot, y1));
    y2 = max(clip.top, min(clip.bot, y2));

    x1 = D_u_to_d_col(x1);
    x2 = D_u_to_d_col(x2);
    y1 = D_u_to_d_row(y1);
    y2 = D_u_to_d_row(y2);

    R_box_abs(x1, y1, x2, y2);
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

static void box_cull(double x1, double y1, double x2, double y2)
{
    if (x1 > clip.rite && x2 > clip.rite)
	return;
    if (x1 < clip.left && x2 < clip.left)
	return;
    if (y1 > clip.bot && y2 > clip.bot)
	return;
    if (y1 < clip.top && y2 < clip.top)
	return;

    x1 = D_u_to_d_col(x1);
    y1 = D_u_to_d_row(y1);
    x2 = D_u_to_d_col(x2);
    y2 = D_u_to_d_row(y2);

    R_box_abs(x1, y1, x2, y2);
}

static void box_cull_ll(double x1, double y1, double x2, double y2)
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

static void polydots_cull(const double *x, const double *y, int n)
{
    double ux0 = clip.left;
    int i, j;

    alloc_src(n);

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

    n = do_convert(xf, yf, n);

    R_polydots_abs(xi, yi, j);
}

static int line_cull(double x1, double y1, double x2, double y2)
{
    if (x1 > clip.rite && x2 > clip.rite)
	return 1;
    if (x1 < clip.left && x2 < clip.left)
	return 1;
    if (y1 > clip.bot && y2 > clip.bot)
	return 1;
    if (y1 < clip.top && y2 < clip.top)
	return 1;

    x1 = D_u_to_d_col(x1);
    y1 = D_u_to_d_row(y1);
    x2 = D_u_to_d_col(x2);
    y2 = D_u_to_d_row(y2);

    R_move_abs(x1, y1);
    R_cont_abs(x2, y2);

    return 0;
}

static int line_cull_ll(double ax, double ay, double bx, double by)
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

    ret = 1;

    for (i = lo; i <= hi; i++)
	ret &= line_cull(ax + i * 360, ay, bx + i * 360, by);

    return ret;
}

int D_cont_abs_cull(double x, double y)
{
    int ret;

    if (!window_set)
	D_clip_to_map();

    if (D_is_lat_lon())
	ret = line_cull_ll(cur.x, cur.y, x, y);
    else
	ret = line_cull(cur.x, cur.y, x, y);

    cur.x = x;
    cur.y = y;

    return ret;
}

int D_cont_rel_cull(double x, double y)
{
    return D_cont_abs_cull(cur.x + x, cur.y + y);
}

int D_line_abs_cull(double x1, double y1, double x2, double y2)
{
    D_move_rel(x1, y1);
    return D_cont_rel_cull(x2, y2);
}

int D_line_rel_cull(double x1, double y1, double x2, double y2)
{
    D_move_abs(x1, y1);
    return D_cont_abs_cull(x2, y2);
}

void D_polydots_abs_cull(const double *x, const double *y, int n)
{
    if (!window_set)
	D_clip_to_map();

    polydots_cull(x, y, n);
}

void D_polydots_rel_cull(const double *x, const double *y, int n)
{
    rel_to_abs(&x, &y, n);
    D_polydots_abs_cull(x, y, n);
    dealloc_src(&x, &y, 1);
}

void D_polyline_abs_cull(const double *x, const double *y, int n)
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

void D_polyline_rel_cull(const double *x, const double *y, int n)
{
    rel_to_abs(&x, &y, n);
    D_polyline_abs_cull(x, y, n);
    dealloc_src(&x, &y, 1);
}

void D_polygon_abs_cull(const double *x, const double *y, int n)
{
    if (!window_set)
	D_clip_to_map();

    if (D_is_lat_lon())
	do_ll_wrap(x, y, n, polygon_cull);
    else
	polygon_cull(x, y, n);
}

void D_polygon_rel_cull(const double *x, const double *y, int n)
{
    rel_to_abs(&x, &y, n);
    D_polygon_abs_cull(x, y, n);
    dealloc_src(&x, &y, 1);
}

void D_box_abs_cull(double x1, double y1, double x2, double y2)
{
    if (!window_set)
	D_clip_to_map();

    if (D_is_lat_lon())
	box_cull_ll(x1, y1, x2, y2);
    else
	box_cull(x1, y1, x2, y2);
}

void D_box_rel_cull(double x2, double y2)
{
    D_box_abs_cull(cur.x, cur.y, x2, y2);
}

int D_cont_abs_clip(double x, double y)
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

int D_cont_rel_clip(double x, double y)
{
    return D_cont_abs_clip(cur.x + x, cur.y + y);
}

int D_line_abs_clip(double x1, double y1, double x2, double y2)
{
    D_move_abs(x1, y1);
    return D_cont_abs_clip(x2, y2);
}

int D_line_rel_clip(double x1, double y1, double x2, double y2)
{
    D_move_rel(x1, y1);
    return D_cont_rel_clip(x2, y2);
}

void D_polydots_abs_clip(const double *x, const double *y, int n)
{
    if (!window_set)
	D_clip_to_map();

    polydots_cull(x, y, n);
}

void D_polydots_rel_clip(const double *x, const double *y, int n)
{
    rel_to_abs(&x, &y, n);
    D_polydots_abs_clip(x, y, n);
    dealloc_src(&x, &y, 1);
}

void D_polyline_abs_clip(const double *x, const double *y, int n)
{
    if (!window_set)
	D_clip_to_map();

    if (n < 2)
	return;

    if (D_is_lat_lon())
	do_ll_wrap(x, y, n, polyline_clip);
    else
	polyline_clip(x, y, n);
}

void D_polyline_rel_clip(const double *x, const double *y, int n)
{
    rel_to_abs(&x, &y, n);
    D_polyline_abs_clip(x, y, n);
    dealloc_src(&x, &y, 1);
}

void D_polygon_abs_clip(const double *x, const double *y, int n)
{
    if (!window_set)
	D_clip_to_map();

    if (D_is_lat_lon())
	do_ll_wrap(x, y, n, polygon_clip);
    else
	polygon_clip(x, y, n);
}

void D_polygon_rel_clip(const double *x, const double *y, int n)
{
    rel_to_abs(&x, &y, n);
    D_polygon_abs_clip(x, y, n);
    dealloc_src(&x, &y, 1);
}

void D_box_abs_clip(double x1, double y1, double x2, double y2)
{
    if (!window_set)
	D_clip_to_map();

    if (D_is_lat_lon())
	box_clip_ll(x1, y1, x2, y2);
    else
	box_clip(x1, y1, x2, y2);
}

void D_box_rel_clip(double x2, double y2)
{
    D_box_abs_clip(cur.x, cur.y, x2, y2);
}

void D_move_abs(double x, double y)
{
    cur.x = x;
    cur.y = y;

    x = D_u_to_d_col(x);
    y = D_u_to_d_row(y);

    R_move_abs(x, y);
}

void D_move_rel(double x, double y)
{
    D_move_abs(cur.x + x, cur.y + y);
}

void D_cont_abs(double x, double y)
{
    cur.x = x;
    cur.y = y;

    x = D_u_to_d_col(x);
    y = D_u_to_d_row(y);

    R_cont_abs(x, y);
}

void D_cont_rel(double x, double y)
{
    D_cont_abs(cur.x + x, cur.y + y);
}

void D_line_abs(double x1, double y1, double x2, double y2)
{
    D_move_abs(x1, y1);
    D_cont_abs(x2, y2);
}

void D_line_rel(double x1, double y1, double x2, double y2)
{
    D_move_rel(x1, y1);
    D_cont_rel(x2, y2);
}

void D_polydots_abs(const double *x, const double *y, int n)
{
    n = do_convert(x, y, n);
    R_polydots_abs(xi, yi, n);
}

void D_polydots_rel(const double *x, const double *y, int n)
{
    rel_to_abs(&x, &y, n);
    D_polydots_abs(x, y, n);
    dealloc_src(&x, &y, 1);
}

void D_polyline_abs(const double *x, const double *y, int n)
{
    n = do_convert(x, y, n);
    R_polyline_abs(xi, yi, n);
}

void D_polyline_rel(const double *x, const double *y, int n)
{
    rel_to_abs(&x, &y, n);
    D_polyline_abs(x, y, n);
    dealloc_src(&x, &y, 1);
}

void D_polygon_abs(const double *x, const double *y, int n)
{
    n = do_convert(x, y, n);
    R_polygon_abs(xi, yi, n);
}

void D_polygon_rel(const double *x, const double *y, int n)
{
    rel_to_abs(&x, &y, n);
    D_polygon_abs(x, y, n);
    dealloc_src(&x, &y, 1);
}

void D_box_abs(double x1, double y1, double x2, double y2)
{
    x1 = D_u_to_d_col(x1);
    x2 = D_u_to_d_col(x2);
    y1 = D_u_to_d_row(y1);
    y2 = D_u_to_d_row(y2);

    R_box_abs(x1, y1, x2, y2);
}

void D_box_rel(double x2, double y2)
{
    D_box_abs(cur.x, cur.y, x2, y2);
}

