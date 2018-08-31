
#include <math.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "path.h"
#include "clip.h"

/******************************************************************************/

static double dist_plane(double x, double y, const struct plane *p)
{
    return x * p->x + y * p->y + p->k;
}

static double interpolate(double a, double b, double ka, double kb)
{
    return (a * kb - b * ka) / (kb - ka);
}

static void clip_path_plane(struct path *dst, const struct path *src,
			    const struct plane *p)
{
    struct vertex *v0 = &src->vertices[src->count - 1];
    double x0 = v0->x;
    double y0 = v0->y;
    double d0 = dist_plane(x0, y0, p);
    int i;

    path_reset(dst);

    for (i = 0; i < src->count; i++) {
	struct vertex *v1 = &src->vertices[i];
	double x1 = v1->x;
	double y1 = v1->y;
	double d1 = dist_plane(x1, y1, p);
	int in0 = d0 <= 0;
	int in1 = d1 <= 0;

	if (in0 && !in1) {
	    /* leaving */
	    double x = interpolate(x0, x1, d0, d1);
	    double y = interpolate(y0, y1, d0, d1);
	    path_cont(dst, x, y);
	}

	if (!in0 && in1) {
	    /* entering */
	    double x = interpolate(x0, x1, d0, d1);
	    double y = interpolate(y0, y1, d0, d1);
	    path_move(dst, x, y);
	}

	if (in1)
	    /* inside */
	    path_cont(dst, x1, y1);

	x0 = x1;
	y0 = y1;
	d0 = d1;
    }
}

/******************************************************************************/

static void cull_path_plane(struct path *dst, const struct path *src,
			    const struct plane *p)
{
    int last = -1;
    int prev = src->count - 1;
    struct vertex *v0 = &src->vertices[prev];
    double x0 = v0->x;
    double y0 = v0->y;
    double d0 = dist_plane(x0, y0, p);
    int i;

    path_reset(dst);

    for (i = 0; i < src->count; i++) {
	struct vertex *v1 = &src->vertices[i];
	double x1 = v1->x;
	double y1 = v1->y;
	double d1 = dist_plane(x1, y1, p);
	int in0 = d0 <= 0;
	int in1 = d1 <= 0;

	if (!in0 && in1 && last != prev) {
	    /* entering */
	    path_move(dst, x0, y0);
	    last = prev;
	}

	if (in1 || in0) {
	    /* inside or leaving */
	    path_cont(dst, x1, y1);
	    last = i;
	}

	x0 = x1;
	y0 = y1;
	d0 = d1;
	prev = i;
    }
}

/******************************************************************************/

void D__set_clip_planes(struct clip *clip, const struct rectangle *rect)
{
    clip->left.x = -1;
    clip->left.y = 0;
    clip->left.k = rect->left;

    clip->rite.x = 1;
    clip->rite.y = 0;
    clip->rite.k = -rect->rite;

    clip->bot.x = 0;
    clip->bot.y = -1;
    clip->bot.k = rect->bot;

    clip->top.x = 0;
    clip->top.y = 1;
    clip->top.k = -rect->top;
}

void D__cull_path(struct path *dst, const struct path *src, const struct clip *clip)
{
    struct path tmp1, tmp2;

    path_init(&tmp1);
    path_init(&tmp2);

    cull_path_plane(&tmp1, src, &clip->left);
    cull_path_plane(&tmp2, &tmp1, &clip->rite);
    cull_path_plane(&tmp1, &tmp2, &clip->bot);
    cull_path_plane(dst, &tmp1, &clip->top);

    path_free(&tmp1);
    path_free(&tmp2);
}

void D__clip_path(struct path *dst, const struct path *src, const struct clip *clip)
{
    struct path tmp1, tmp2;

    path_init(&tmp1);
    path_init(&tmp2);

    clip_path_plane(&tmp1, src, &clip->left);
    clip_path_plane(&tmp2, &tmp1, &clip->rite);
    clip_path_plane(&tmp1, &tmp2, &clip->bot);
    clip_path_plane(dst, &tmp1, &clip->top);

    path_free(&tmp1);
    path_free(&tmp2);
}

/******************************************************************************/

