/*!
   \file lib/vector/Vlib/dgraph.c

   \brief Vector library - intersection (lower level functions)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2008-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Rewritten by Rosen Matev (Google Summer of Code 2008)
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "dgraph.h"
#include "e_intersect.h"

#define LENGTH(DX, DY) (sqrt((DX*DX)+(DY*DY)))
#ifndef MIN
#define MIN(X,Y) ((X<Y)?X:Y)
#endif
#ifndef MAX
#define MAX(X,Y) ((X>Y)?X:Y)
#endif
#define PI M_PI

struct intersection_point
{
    double x;
    double y;
    int group;			/* IPs with very similar dist will be in the same group */
};

struct seg_intersection
{
    int with;			/* second segment */
    int ip;			/* index of the IP */
    double dist;		/* distance from first point of first segment to intersection point (IP) */
};

struct seg_intersection_list
{
    int count;
    int allocated;
    struct seg_intersection *a;
};

struct seg_intersections
{
    int ipcount;
    int ipallocated;
    struct intersection_point *ip;
    int ilcount;
    struct seg_intersection_list *il;
    int group_count;
};

struct seg_intersections *create_si_struct(int segments_count)
{
    struct seg_intersections *si;

    int i;

    si = G_malloc(sizeof(struct seg_intersections));
    si->ipcount = 0;
    si->ipallocated = segments_count + 16;
    si->ip = G_malloc((si->ipallocated) * sizeof(struct intersection_point));
    si->ilcount = segments_count;
    si->il = G_malloc(segments_count * sizeof(struct seg_intersection_list));
    for (i = 0; i < segments_count; i++) {
	si->il[i].count = 0;
	si->il[i].allocated = 0;
	si->il[i].a = NULL;
    }

    return si;
}

void destroy_si_struct(struct seg_intersections *si)
{
    int i;

    for (i = 0; i < si->ilcount; i++)
	G_free(si->il[i].a);
    G_free(si->il);
    G_free(si->ip);
    G_free(si);

    return;
}

/* internal use */
void add_ipoint1(struct seg_intersection_list *il, int with, double dist,
			int ip)
{
    struct seg_intersection *s;

    if (il->count == il->allocated) {
	il->allocated += 4;
	il->a =
	    G_realloc(il->a,
		      (il->allocated) * sizeof(struct seg_intersection));
    }
    s = &(il->a[il->count]);
    s->with = with;
    s->ip = ip;
    s->dist = dist;
    il->count++;

    return;
}

/* adds intersection point to the structure */
void add_ipoint(const struct line_pnts *Points, int first_seg, int second_seg,
		       double x, double y, struct seg_intersections *si)
{
    struct intersection_point *t;

    int ip;

    G_debug(4, "add_ipoint()");

    if (si->ipcount == si->ipallocated) {
	si->ipallocated += 16;
	si->ip =
	    G_realloc(si->ip,
		      (si->ipallocated) * sizeof(struct intersection_point));
    }
    ip = si->ipcount;
    t = &(si->ip[ip]);
    t->x = x;
    t->y = y;
    t->group = -1;
    si->ipcount++;

    add_ipoint1(&(si->il[first_seg]), second_seg,
		LENGTH((Points->x[first_seg] - x),
		       (Points->y[first_seg] - y)), ip);
    if (second_seg >= 0)
	add_ipoint1(&(si->il[second_seg]), first_seg,
		    LENGTH((Points->x[second_seg] - x),
			   (Points->y[second_seg] - y)), ip);
}

void sort_intersection_list(struct seg_intersection_list *il)
{
    int n, i, j, min;
    struct seg_intersection t;

    G_debug(4, "sort_intersection_list()");

    n = il->count;
    G_debug(4, "    n=%d", n);
    for (i = 0; i < n - 1; i++) {
	min = i;
	for (j = i + 1; j < n; j++) {
	    if (il->a[j].dist < il->a[min].dist) {
		min = j;
	    }
	}
	if (min != i) {
	    t = il->a[i];
	    il->a[i] = il->a[min];
	    il->a[min] = t;
	}
    }

    return;
}

int compare(const void *a, const void *b)
{
    struct intersection_point *aa, *bb;

    aa = *((struct intersection_point **)a);
    bb = *((struct intersection_point **)b);

    if (aa->x < bb->x)
	return -1;
    else if (aa->x > bb->x)
	return 1;
    else
	return (aa->y < bb->y) ? -1 : ((aa->y > bb->y) ? 1 : 0);
}

/* O(Points->n_points) time */
double get_epsilon(struct line_pnts *Points)
{
    int i, np;

    double min, t;

    double *x, *y;

    np = Points->n_points;
    x = Points->x;
    y = Points->y;

    min = MAX(fabs(x[1] - x[0]), fabs(y[1] - y[0]));
    for (i = 1; i <= np - 2; i++) {
	t = MAX(fabs(x[i + 1] - x[i]), fabs(y[i + 1] - y[i]));
	if ((t > 0) && (t < min)) {
	    min = t;
	}
    }

    /* ??? is 0.001 ok ??? */
    return min * 0.000001;
}

/* currently O(n*n); future implementation O(nlogn) */
struct seg_intersections *find_all_intersections(const struct line_pnts *Points)
{
    int i, j, np;
    int group, t;
    int looped;
    /* double EPSILON = 0.00000001; */
    double EPSILON = GRASS_EPSILON;
    double *x, *y;
    double x1, y1, x2, y2;
    int res;

    /*int res2
       double x1_, y1_, x2_, y2_, z1_, z2_; */
    struct seg_intersections *si;
    struct seg_intersection_list *il;
    struct intersection_point **sorted;

    G_debug(3, "find_all_intersections()");

    np = Points->n_points;
    x = Points->x;
    y = Points->y;

    si = create_si_struct(np - 1);

    looped = ((x[0] == x[np - 1]) && (y[0] == y[np - 1]));
    G_debug(3, "    looped=%d", looped);

    G_debug(3, "    finding intersections...");
    for (i = 0; i < np - 1; i++) {
	for (j = i + 1; j < np - 1; j++) {
	    G_debug(4, "        checking %d-%d %d-%d", i, i + 1, j, j + 1);
	    /*res = segment_intersection_2d_e(x[i], y[i], x[i+1], y[i+1], x[j], y[j], x[j+1], y[j+1], &x1, &y1, &x2, &y2); */
	    res =
		segment_intersection_2d(x[i], y[i], x[i + 1], y[i + 1], x[j],
					y[j], x[j + 1], y[j + 1], &x1, &y1,
					&x2, &y2);

	    /*            res2 = segment_intersection_2d_e(x[i], y[i], x[i+1], y[i+1], x[j], y[j], x[j+1], y[j+1], &x1_, &y1_, &x2_, &y2_);
	       if ((res != res2) || ((res != 0) && (x1!=x1_ || y1!=y1_)) ) {
	       G_debug(1, "exact=%d orig=%d", res, res2);
	       segment_intersection_2d_test(x[i], y[i], x[i+1], y[i+1], x[j], y[j], x[j+1], y[j+1], &x1, &y1, &x2, &y2);
	       }
	     */
	    G_debug(4, "        intersection type = %d", res);
	    if (res == 1) {
		add_ipoint(Points, i, j, x1, y1, si);
	    }
	    else if ((res >= 2) && (res <= 5)) {
		add_ipoint(Points, i, j, x1, y1, si);
		add_ipoint(Points, i, j, x2, y2, si);
	    }
	}
    }
    if (!looped) {
	/* these are not really intersection points */
	add_ipoint(Points, 0, -1, Points->x[0], Points->y[0], si);
	add_ipoint(Points, np - 2, -1, Points->x[np - 1], Points->y[np - 1],
		   si);
    }
    G_debug(3, "    finding intersections...done");

    G_debug(3, "    postprocessing...");
    if (si->ipallocated > si->ipcount) {
	si->ipallocated = si->ipcount;
	si->ip =
	    G_realloc(si->ip,
		      (si->ipcount) * sizeof(struct intersection_point));
    }
    for (i = 0; i < si->ilcount; i++) {
	il = &(si->il[i]);
	if (il->allocated > il->count) {
	    il->allocated = il->count;
	    il->a =
		G_realloc(il->a,
			  (il->count) * sizeof(struct seg_intersection));
	}

	if (il->count > 0) {
	    sort_intersection_list(il);
	    /* is it ok to use qsort here ? */
	}
    }

    /* si->ip will not be reallocated any more so we can use pointers */
    sorted = G_malloc((si->ipcount) * sizeof(struct intersection_point *));
    for (i = 0; i < si->ipcount; i++)
	sorted[i] = &(si->ip[i]);

    qsort(sorted, si->ipcount, sizeof(struct intersection_point *), compare);

    /* assign groups */
    group = 0;			/* next available group number */
    for (i = 0; i < si->ipcount; i++) {

	t = group;
	for (j = i - 1; j >= 0; j--) {
	    if (!FEQUAL(sorted[j]->x, sorted[i]->x, EPSILON))
		/*            if (!almost_equal(sorted[j]->x, sorted[i]->x, 16)) */
		break;
	    if (FEQUAL(sorted[j]->y, sorted[i]->y, EPSILON)) {
		/*            if (almost_equal(sorted[j]->y, sorted[i]->y, 16)) { */
		t = sorted[j]->group;
		break;
	    }
	}
	G_debug(4, "        group=%d, ip=%d", t,
		(int)(sorted[i] - &(si->ip[0])));
	sorted[i]->group = t;
	if (t == group)
	    group++;
    }
    si->group_count = group;

    G_debug(3, "    postprocessing...done");

    /* output contents of si */
    for (i = 0; i < si->ilcount; i++) {
	G_debug(4, "%d-%d :", i, i + 1);
	for (j = 0; j < si->il[i].count; j++) {
	    G_debug(4, "     %d-%d, group=%d", si->il[i].a[j].with,
		    si->il[i].a[j].with + 1, si->ip[si->il[i].a[j].ip].group);
	    G_debug(4, "            dist=%.18f", si->il[i].a[j].dist);
	    G_debug(4, "            x=%.18f, y=%.18f",
		    si->ip[si->il[i].a[j].ip].x, si->ip[si->il[i].a[j].ip].y);
	}
    }
    G_free(sorted);

    return si;
}

/* create's graph with n vertices and allocates memory for e edges */
/* trying to add more than e edges, produces fatal error */
struct planar_graph *pg_create_struct(int n, int e)
{
    struct planar_graph *pg;

    pg = G_malloc(sizeof(struct planar_graph));
    pg->vcount = n;
    pg->v = G_malloc(n * sizeof(struct pg_vertex));
    memset(pg->v, 0, n * sizeof(struct pg_vertex));
    pg->ecount = 0;
    pg->eallocated = MAX(e, 0);
    pg->e = NULL;
    pg->e = G_malloc(e * sizeof(struct pg_edge));

    return pg;
}

void pg_destroy_struct(struct planar_graph *pg)
{
    int i;

    for (i = 0; i < pg->vcount; i++) {
	G_free(pg->v[i].edges);
	G_free(pg->v[i].angles);
    }
    G_free(pg->v);
    G_free(pg->e);
    G_free(pg);
}

/* v1 and v2 must be valid */
int pg_existsedge(struct planar_graph *pg, int v1, int v2)
{
    struct pg_vertex *v;
    struct pg_edge *e;
    int i, ecount;

    if (pg->v[v1].ecount <= pg->v[v2].ecount)
	v = &(pg->v[v1]);
    else
	v = &(pg->v[v2]);

    ecount = v->ecount;
    for (i = 0; i < ecount; i++) {
	e = v->edges[i];
	if (((e->v1 == v1) && (e->v2 == v2)) ||
	    ((e->v1 == v2) && (e->v2 == v1)))
	    return 1;
    }

    return 0;
}

/* for internal use */
void pg_addedge1(struct pg_vertex *v, struct pg_edge *e)
{
    if (v->ecount == v->eallocated) {
	v->eallocated += 4;
	v->edges =
	    G_realloc(v->edges, (v->eallocated) * sizeof(struct pg_edge *));
    }
    v->edges[v->ecount] = e;
    v->ecount++;
}

void pg_addedge(struct planar_graph *pg, int v1, int v2)
{
    struct pg_edge *e;

    G_debug(4, "pg_addedge(), v1=%d, v2=%d", v1, v2);

    if ((v1 == v2) || (v1 < 0) || (v1 >= pg->vcount) || (v2 < 0) ||
	(v2 >= pg->vcount)) {
	G_fatal_error("    pg_addedge(), v1 and/or v2 is invalid");
	return;
    }

    if (pg_existsedge(pg, v1, v2))
	return;

    if (pg->ecount == pg->eallocated) {
	G_fatal_error(_("Trying to add more edges to the planar_graph "
			"than the initial allocation size allows"));
    }
    e = &(pg->e[pg->ecount]);
    e->v1 = v1;
    e->v2 = v2;
    e->winding_left = 0;	/* winding is undefined if the corresponding side is not visited */
    e->winding_right = 0;
    e->visited_left = 0;
    e->visited_right = 0;
    pg->ecount++;
    pg_addedge1(&(pg->v[v1]), e);
    pg_addedge1(&(pg->v[v2]), e);

    return;
}

struct planar_graph *pg_create(const struct line_pnts *Points)
{
    struct seg_intersections *si;
    struct planar_graph *pg;
    struct intersection_point *ip;
    struct pg_vertex *vert;
    struct pg_edge *edge;

    int i, j, t, v;

    G_debug(3, "pg_create()");

    si = find_all_intersections(Points);
    pg = pg_create_struct(si->group_count, 2 * (si->ipcount));

    /* set vertices info */
    for (i = 0; i < si->ipcount; i++) {
	ip = &(si->ip[i]);
	t = ip->group;
	pg->v[t].x = ip->x;
	pg->v[t].y = ip->y;
    }

    /* add all edges */
    for (i = 0; i < si->ilcount; i++) {
	v = si->ip[si->il[i].a[0].ip].group;
	for (j = 1; j < si->il[i].count; j++) {
	    t = si->ip[si->il[i].a[j].ip].group;
	    if (t != v) {
		pg_addedge(pg, v, t);	/* edge direction is v ---> t */
		v = t;
	    }
	}
    }

    /* precalculate angles with 0x */
    for (i = 0; i < pg->vcount; i++) {
	vert = &(pg->v[i]);
	vert->angles = G_malloc((vert->ecount) * sizeof(double));
	for (j = 0; j < vert->ecount; j++) {
	    edge = vert->edges[j];
	    t = (edge->v1 != i) ? (edge->v1) : (edge->v2);
	    vert->angles[j] =
		atan2(pg->v[t].y - vert->y, pg->v[t].x - vert->x);
	}
    }

    destroy_si_struct(si);
    /*
       I'm not sure if shrinking of the allocated memory always preserves it's physical place.
       That's why I don't want to do this:
       if (pg->ecount < pg->eallocated) {
       pg->eallocated = pg->ecount;
       pg->e = G_realloc(pg->e, (pg->ecount)*sizeof(struct pg_edge));
       }
     */

    /* very time consuming */
    /*
       for (i = 0; i < pg->vcount; i++) {
       if (pg->v[i].ecount < pg->v[i].eallocated) {
       pg->v[i].eallocated = pg->v[i].ecount;
       pg->v[i].edges = G_realloc(pg->v[i].edges, (pg->v[i].ecount)*sizeof(struct pg_edges));
       }
       }
     */

    /* output pg */
    for (i = 0; i < pg->vcount; i++) {
	G_debug(4, "    vertex %d (%g, %g)", i, pg->v[i].x, pg->v[i].y);
	for (j = 0; j < pg->v[i].ecount; j++) {
	    G_debug(4, "        edge %d-%d", pg->v[i].edges[j]->v1,
		    pg->v[i].edges[j]->v2);
	}
    }

    return pg;
}
