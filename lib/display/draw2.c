
#include <math.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "driver.h"
#include "path.h"
#include "clip.h"

struct vector
{
    double x, y;
};

/******************************************************************************/

static struct path path;

static int clip_mode = M_NONE;
static double epsilon = 0.0;
static struct path ll_path, clip_path, raw_path, eps_path;

static struct vector cur;

static struct rectangle clip;

static int window_set;

#define min(x,y) ((x) < (y) ? (x) : (y))
#define max(x,y) ((x) > (y) ? (x) : (y))

/******************************************************************************/

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

static int euclidify(struct path *p, int no_pole)
{
    double ux0 = clip.left;
    double ux1 = clip.rite;
    double x0, x1;
    int lo, hi, count;
    int i;

    x0 = x1 = p->vertices[0].x;

    for (i = 1; i < p->count; i++) {
	if (fabs(p->vertices[i].y) < 89.9)
	    p->vertices[i].x = p->vertices[i-1].x + coerce(p->vertices[i].x - p->vertices[i-1].x);

	x0 = min(x0, p->vertices[i].x);
	x1 = max(x1, p->vertices[i].x);
    }

    if (no_pole && fabs(p->vertices[p->count-1].x - p->vertices[0].x) > 180)
	return 0;

    lo = -shift_count(ux1 - x0);
    hi = shift_count(x1 - ux0);
    count = hi - lo + 1;

    for (i = 0; i < p->count; i++)
	p->vertices[i].x -= lo * 360;

    return count;
}

static void ll_wrap_path(struct path *dst, const struct path *src, int no_pole)
{
    int count, i, j;

    path_copy(dst, src);

    count = euclidify(dst, no_pole);

    for (i = 0; i < count; i++) {
	for (j = 0; j < src->count; j++) {
	    struct vertex *v = &dst->vertices[j];
	    path_append(dst, v->x - i * 360, v->y, v->mode);
	}
    }
}

static void conv_path(struct path *dst, const struct path *src)
{
    int i;

    path_copy(dst, src);

    for (i = 0; i < dst->count; i++) {
	struct vertex *v = &dst->vertices[i];
	v->x = D_u_to_d_col(v->x);
	v->y = D_u_to_d_row(v->y);
    }
}

static void reduce_path(struct path *dst, const struct path *src, double eps)
{
    struct vertex *v = &src->vertices[0];
    int i;

    path_reset(dst);
    path_append(dst, v->x, v->y, v->mode);

    for (i = 1; i < src->count - 1; i++) {
	struct vertex *v0 = &dst->vertices[dst->count-1];
	struct vertex *v1 = &src->vertices[i];
	struct vertex *v2 = &src->vertices[i+1];

	if (fabs(v1->x - v0->x) < eps && fabs(v1->y - v0->y) < eps &&
	    fabs(v1->x - v2->x) < eps && fabs(v1->y - v2->y) < eps &&
	    v0->mode != P_MOVE && v1->mode != P_MOVE && !v2->mode != P_MOVE)
	    continue;

	path_append(dst, v1->x, v1->y, v1->mode);
    }
    v = &src->vertices[src->count - 1];
    path_append(dst, v->x, v->y, v->mode);
}

/******************************************************************************/

/*!
 * \brief set clipping window
 *
 * Sets the clipping window to the pixel window that corresponds
 * to the current database region. This is the default.
 *
 *  \param t top
 *  \param b bottom
 *  \param l left
 *  \param r right
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

void D_set_clip_mode(int mode)
{
    clip_mode = mode;
}

void D_set_reduction(double e)
{
    epsilon = e;
}

void D_line_width(double d)
{
    COM_Line_width(d > 0 ? d : 0);
}

void D_get_text_box(const char *text, double *t, double *b, double *l, double *r)
{
    double T, B, L, R;

    COM_Get_text_box(text, &T, &B, &L, &R);

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

/* D_pos_abs(easting, northing):  move to an absolute position
	on the display using map coordinates */
void D_pos_abs(double x, double y)
{
    cur.x = x;
    cur.y = y;

    x = D_u_to_d_col(x);
    y = D_u_to_d_row(y);

    COM_Pos_abs(x, y);
}

void D_pos_rel(double x, double y)
{
    D_pos_abs(cur.x + x, cur.y + y);
}

/******************************************************************************/

static void do_path(int no_pole)
{
    struct path *p = &path;
    struct clip planes;
    int i;

    if (!window_set)
	D_clip_to_map();

    if (D_is_lat_lon()) {
	ll_wrap_path(&ll_path, p, no_pole);
	p = &ll_path;
    }

    switch (clip_mode) {
    case M_NONE:
	break;
    case M_CULL:
	D__set_clip_planes(&planes, &clip);
	D__cull_path(&clip_path, p, &planes);
	p = &clip_path;
	break;
    case M_CLIP:
	D__set_clip_planes(&planes, &clip);
	D__clip_path(&clip_path, p, &planes);
	p = &clip_path;
	break;
    }

    conv_path(&raw_path, p);
    p = &raw_path;

    if (epsilon > 0) {
	reduce_path(&eps_path, p, epsilon);
	p = &eps_path;
    }

    COM_Begin();
    for (i = 0; i < p->count; i++) {
	struct vertex *v = &p->vertices[i];
	switch (v->mode)
	{
	case P_MOVE:
	    COM_Move(v->x, v->y);
	    break;
	case P_CONT:
	    COM_Cont(v->x, v->y);
	    break;
	case P_CLOSE:
	    COM_Close();
	    break;
	}
    }
}

void D_begin(void)
{
    path_begin(&path);
}

void D_end(void)
{
}

/* D_move_abs(x,y):  move to an absolute position on the display using
	display pixel coordinates */
void D_move_abs(double x, double y)
{
    path_move(&path, x, y);

    cur.x = x;
    cur.y = y;
}

void D_cont_abs(double x, double y)
{
    path_cont(&path, x, y);

    cur.x = x;
    cur.y = y;
}

void D_close(void)
{
    path_close(&path);
}

void D_stroke(void)
{
    do_path(0);
    COM_Stroke();
}

void D_fill(void)
{
    do_path(1);
    COM_Fill();
}

void D_dots(void)
{
    struct path *p = &path;
    int i;

    if (!window_set)
	D_clip_to_map();

    for (i = 0; i < p->count; i++) {
	struct vertex *v = &p->vertices[i];
	double x = v->x;
	double y = v->y;

	if (D_is_lat_lon())
	    x = coerce(x);

	if (clip_mode != M_NONE) {
	    if (x < clip.left || x > clip.rite)
		continue;
	    if (y < clip.bot || y > clip.top)
		continue;
	}

	x = D_u_to_d_col(x);
	y = D_u_to_d_row(y);

	COM_Point(x, y);
    }
}

/******************************************************************************/

static void poly_abs(const double *x, const double *y, int n)
{
    int i;

    if (n < 2)
	return;

    D_begin();
    D_move_abs(x[0], y[0]);
    for (i = 1; i < n; i++)
	D_cont_abs(x[i], y[i]);
}

void D_polyline_abs(const double *x, const double *y, int n)
{
    poly_abs(x, y, n);
    D_stroke();
}

void D_polygon_abs(const double *x, const double *y, int n)
{
    poly_abs(x, y, n);
    D_close();
    D_fill();
}

void D_polydots_abs(const double *x, const double *y, int n)
{
    poly_abs(x, y, n);
    D_dots();
}

void D_line_abs(double x1, double y1, double x2, double y2)
{
    D_begin();
    D_move_abs(x1, y1);
    D_cont_abs(x2, y2);
    D_end();
    D_stroke();
}

void D_box_abs(double x1, double y1, double x2, double y2)
{
    struct vector save = cur;

    D_begin();
    D_move_abs(x1, y1);
    D_cont_abs(x2, y1);
    D_cont_abs(x2, y2);
    D_cont_abs(x1, y2);
    D_close();
    D_end();
    D_fill();

    cur = save;
}

/******************************************************************************/

static void poly_rel(const double *x, const double *y, int n)
{
    int i;

    if (n < 2)
	return;

    D_begin();
    D_move_rel(x[0], y[0]);
    for (i = 1; i < n; i++)
	D_cont_rel(x[i], y[i]);
}

void D_move_rel(double x, double y)
{
    D_move_abs(cur.x + x, cur.y + y);
}

void D_cont_rel(double x, double y)
{
    D_cont_abs(cur.x + x, cur.y + y);
}

void D_polydots_rel(const double *x, const double *y, int n)
{
    poly_rel(x, y, n);
    D_dots();
}

void D_polyline_rel(const double *x, const double *y, int n)
{
    poly_rel(x, y, n);
    D_stroke();
}

void D_polygon_rel(const double *x, const double *y, int n)
{
    poly_rel(x, y, n);
    D_close();
    D_fill();
}

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

void D_box_rel(double x2, double y2)
{
    D_box_abs(cur.x, cur.y, cur.x + x2, cur.y + y2);
}

/******************************************************************************/

