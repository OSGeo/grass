#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>
#include "data_types.h"
#include "memory.h"
#include "edge.h"
#include "geom_primitives.h"

void output_edges(struct vertex *sites_sorted[], unsigned int n, int mode3d,
		  int Type, struct Map_info map_out)
{
    struct edge *e_start, *e;
    struct vertex *u, *v;
    unsigned int i;

    static struct line_pnts *Points = NULL;
    static struct line_cats *Cats = NULL;

    if (!Points) {
	Points = Vect_new_line_struct();
	Cats = Vect_new_cats_struct();
    }
    double x1, y1, z1, x2, y2, z2;

    for (i = 0; i < n; i++) {
	u = sites_sorted[i];
	e_start = e = u->entry_pt;
	do {
	    v = OTHER_VERTEX(e, u);
	    if (u < v) {
		x1 = sites[u - sites].x;
		y1 = sites[u - sites].y;
		x2 = sites[v - sites].x;
		y2 = sites[v - sites].y;

		Vect_reset_line(Points);

		if (mode3d) {
		    z1 = sites[u - sites].z;
		    z2 = sites[v - sites].z;
		    Vect_append_point(Points, x1, y1, z1);
		    Vect_append_point(Points, x2, y2, z2);
		}
		else {
		    Vect_append_point(Points, x1, y1, 0.0);
		    Vect_append_point(Points, x2, y2, 0.0);
		}
		Vect_write_line(&map_out, Type, Points, Cats);
	    }
	    e = NEXT(e, u);
	} while (!SAME_EDGE(e, e_start));
    }
}

/* Print the ring of triangles about each vertex. */

void output_triangles(struct vertex *sites_sorted[], unsigned int n,
		      int mode3d, int Type, struct Map_info map_out)
{
    struct edge *e_start, *e, *next;
    struct vertex *u, *v, *w;
    unsigned int i;
    struct vertex *temp;

    struct line_pnts *Points = Vect_new_line_struct();
    struct line_cats *Cats = Vect_new_cats_struct();

    double x1, y1, z1, x2, y2, z2, x3, y3, z3;

    for (i = 0; i < n; i++) {
	u = sites_sorted[i];
	e_start = e = u->entry_pt;
	do {
	    v = OTHER_VERTEX(e, u);
	    if (u < v) {
		next = NEXT(e, u);
		w = OTHER_VERTEX(next, u);
		if (u < w)
		    if (SAME_EDGE(NEXT(next, w), PREV(e, v))) {
			/* Triangle. */
			if (v > w) {
			    temp = v;
			    v = w;
			    w = temp;
			}
			x1 = sites[u - sites].x;
			y1 = sites[u - sites].y;
			x2 = sites[v - sites].x;
			y2 = sites[v - sites].y;
			x3 = sites[w - sites].x;
			y3 = sites[w - sites].y;

			if (mode3d) {
			    z1 = sites[u - sites].z;
			    z2 = sites[v - sites].z;
			    z3 = sites[w - sites].z;
			    Vect_reset_line(Points);
			    Vect_append_point(Points, x1, y1, z1);
			    Vect_append_point(Points, x2, y2, z2);
			    Vect_write_line(&map_out, Type, Points, Cats);

			    Vect_reset_line(Points);
			    Vect_append_point(Points, x2, y2, z2);
			    Vect_append_point(Points, x3, y3, z3);
			    Vect_write_line(&map_out, Type, Points, Cats);

			    Vect_reset_line(Points);
			    Vect_append_point(Points, x3, y3, z3);
			    Vect_append_point(Points, x1, y1, z1);
			    Vect_write_line(&map_out, Type, Points, Cats);
			}
			else {
			    Vect_reset_line(Points);
			    Vect_append_point(Points, x1, y1, 0.0);
			    Vect_append_point(Points, x2, y2, 0.0);
			    Vect_write_line(&map_out, Type, Points, Cats);

			    Vect_reset_line(Points);
			    Vect_append_point(Points, x2, y2, 0.0);
			    Vect_append_point(Points, x3, y3, 0.0);
			    Vect_write_line(&map_out, Type, Points, Cats);

			    Vect_reset_line(Points);
			    Vect_append_point(Points, x3, y3, 0.0);
			    Vect_append_point(Points, x1, y1, 0.0);
			    Vect_write_line(&map_out, Type, Points, Cats);
			}
		    }
	    }
	    /* Next edge around u. */
	    e = NEXT(e, u);
	} while (!SAME_EDGE(e, e_start));
    }
}

void remove_duplicates(struct vertex *list[], unsigned int *size)
{
    int n = *size - 1;
    int left = 0;
    int right = 1;
    int shift = 0;
    int empty = 0;

    if (n > 0)
	for (; right < n; left++, right++)
	    if (list[left]->x == list[right]->x &&
		list[left]->y == list[right]->y) {
		if (shift == 0) {
		    shift = 1;
		    empty = right;
		}
		(*size)--;
	    }
	    else if (shift == 1)
		list[empty++] = list[right];
}

/* returns number of sites read */
int read_sites(int mode3d, int complete_map, struct Map_info map_in,
	       BOUND_BOX Box)
{
    int nlines, line, allocated, nsites;
    struct line_pnts *Points;

    Points = Vect_new_line_struct();
    nlines = Vect_get_num_lines(&map_in);
    alloc_sites(nlines);
    allocated = nlines;

    nsites = 0;
    for (line = 1; line <= nlines; line++) {
	int type;

	type = Vect_read_line(&map_in, Points, NULL, line);
	if (!(type & GV_POINTS))
	    continue;
	if (!complete_map) {
	    if (!Vect_point_in_box(Points->x[0], Points->y[0], 0.0, &Box))
		continue;
	}
	sites[nsites].x = Points->x[0];
	sites[nsites].y = Points->y[0];
	if (mode3d) {
	    G_debug(3, "Points->z[0]: %f", Points->z[0]);
	    sites[nsites].z = Points->z[0];
	}
	/* Initialise entry edge vertices. */
	sites[nsites].entry_pt = MY_NULL;

	nsites += 1;
	/* number 100 was arbitrarily chosen */
#if 0
	if (nsites == allocated && line != nlines){
	    allocated += 100;
	    realloc_sites(allocated);
	}
#endif
    }
    if (nsites != nlines)
	realloc_sites(nsites);
    alloc_edges(nsites);

    return nsites;
}
