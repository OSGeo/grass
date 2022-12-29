
#include <grass/gis.h>
#include "path.h"

void path_init(struct path *p)
{
    p->vertices = NULL;
    p->count = 0;
    p->alloc = 0;
    p->start = -1;
}

void path_free(struct path *p)
{
    if (p->vertices)
	G_free(p->vertices);

    p->count = 0;
    p->alloc = 0;
    p->start = -1;
}

void path_alloc(struct path *p, int n)
{
    if (p->alloc >= n)
	return;

    p->alloc = n;
    p->vertices = G_realloc(p->vertices, p->alloc * sizeof(struct vertex));
}

void path_reset(struct path *p)
{
    p->count = 0;
    p->start = -1;
}

void path_append(struct path *p, double x, double y, int mode)
{
    struct vertex *v;

    if (p->count >= p->alloc)
	path_alloc(p, p->alloc ? p->alloc * 2 : 100);

    v = &p->vertices[p->count++];

    v->x = x;
    v->y = y;
    v->mode = mode;
}

void path_copy(struct path *dst, const struct path *src)
{
    int i;

    path_reset(dst);
    path_alloc(dst, src->count);

    for (i = 0; i < src->count; i++) {
	struct vertex *v = &src->vertices[i];
	path_append(dst, v->x, v->y, v->mode);
    }

    dst->start = src->start;
}

void path_begin(struct path *p)
{
    p->count = 0;
    p->start = -1;
}

void path_move(struct path *p, double x, double y)
{
    p->start = p->count;
    path_append(p, x, y, P_MOVE);
}

void path_cont(struct path *p, double x, double y)
{
    path_append(p, x, y, P_CONT);
}

void path_close(struct path *p)
{
    struct vertex *v;

    if (p->start < 0)
	return;

    v = &p->vertices[p->start];
    path_append(p, v->x, v->y, P_CLOSE);

    p->start = -1;
}

void path_stroke(struct path *p, void (*line)(double, double, double, double))
{
    int i;

    for (i = 1; i < p->count; i++) {
	struct vertex *v0 = &p->vertices[i-1];
	struct vertex *v1 = &p->vertices[i];

	if (v1->mode != P_MOVE)
	    (*line)(v0->x, v0->y, v1->x, v1->y);
    }

    path_reset(p);
}

