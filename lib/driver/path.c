
#include <grass/gis.h>
#include "driverlib.h"

static void begin_subpath(struct path *p)
{
    if (p->o_count >= p->o_alloc) {
	p->o_alloc += 100;
	p->offsets = G_realloc(p->offsets, p->o_alloc * sizeof(int));
    }

    p->offsets[p->o_count++] = p->count;
    p->cur_offset = p->count;
}

static void add_point(struct path *p, double x, double y)
{
    if (p->count >= p->alloc) {
	p->alloc = p->alloc ? p->alloc * 2 : 100;
	p->px = G_realloc(p->px, p->alloc * sizeof(double));
	p->py = G_realloc(p->py, p->alloc * sizeof(double));
    }

    p->px[p->count] = x;
    p->py[p->count] = y;
    p->count++;
}

void path_begin(struct path *p)
{
    p->count = 0;
    p->o_count = 0;
    begin_subpath(p);
}

void path_move(struct path *p, double x, double y)
{
    if (p->count > p->cur_offset)
	begin_subpath(p);
    add_point(p, x, y);
}

void path_cont(struct path *p, double x, double y)
{
    add_point(p, x, y);
}

void path_close(struct path *p)
{
    if (p->count <= p->cur_offset + 2)
	return;

    add_point(p, p->px[p->cur_offset], p->py[p->cur_offset]);
    begin_subpath(p);
}

void path_fill(struct path *p, void (*polygon)(const double *, const double *, int))
{
    int i;

    if (p->count > p->cur_offset)
	begin_subpath(p);

    for (i = 0; i < p->o_count - 1; i++) {
	int start = p->offsets[i];
	int end = p->offsets[i+1];
	(*polygon)(&p->px[start], &p->py[start], end - start);
    }

    path_reset(p);
}

void path_stroke(struct path *p, void (*line)(double, double, double, double))
{
    int i, j;

    if (p->count > p->cur_offset)
	begin_subpath(p);

    for (i = 0; i < p->o_count - 1; i++)
	for (j = p->offsets[i] + 1; j < p->offsets[i+1]; j++)
	    (*line)(p->px[j-1], p->py[j-1], p->px[j], p->py[j]);

    path_reset(p);
}

void path_reset(struct path *p)
{
    p->count = 0;
    p->o_count = 0;
}

