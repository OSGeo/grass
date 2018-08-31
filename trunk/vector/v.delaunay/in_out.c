/***************************************************************
 *
 * MODULE:       v.delaunay
 *
 * AUTHOR(S):    Martin Pavlovsky (Google SoC 2008, Paul Kelly mentor)
 *               Based on "dct" by Geoff Leach, Department of Computer 
 *               Science, RMIT.
 *
 * PURPOSE:      Creates a Delaunay triangulation vector map
 *
 * COPYRIGHT:    (C) RMIT 1993
 *               (C) 2008-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 * 
 * The following notices apply to portions of the code originally
 * derived from work by Geoff Leach of RMIT:
 *
 *   Author: Geoff Leach, Department of Computer Science, RMIT.
 *   email: gl@cs.rmit.edu.au
 *
 *   Date: 6/10/93
 *
 *   Version 1.0
 *   
 *   Copyright (c) RMIT 1993. All rights reserved.
 *
 *   License to copy and use this software purposes is granted provided 
 *   that appropriate credit is given to both RMIT and the author.
 *
 *   License is also granted to make and use derivative works provided
 *   that appropriate credit is given to both RMIT and the author.
 *
 *   RMIT makes no representations concerning either the merchantability 
 *   of this software or the suitability of this software for any particular 
 *   purpose.  It is provided "as is" without express or implied warranty 
 *   of any kind.
 *
 *   These notices must be retained in any copies of any part of this software.
 * 
 **************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "defs.h"
#include "data_types.h"
#include "memory.h"
#include "edge.h"

/* compare first according to x-coordinate, if equal then y-coordinate */
int cmp(const void *a, const void *b)
{
    struct vertex *p1 = (struct vertex *)a;
    struct vertex *p2 = (struct vertex *)b;

    if (p1->x < p2->x)
	return 1;
    else if (p1->x > p2->x)
	return -1;
    else {
	if (p1->y < p2->y)
	    return 1;
	else if (p1->y > p2->y)
	    return -1;
    }
    
    return 0;
}

void output_edges(unsigned int n, int mode3d, int type,
		  struct Map_info *Out)
{
    struct edge *e_start, *e;
    struct vertex *u, *v;
    unsigned int i;
    double x1, y1, z1, x2, y2, z2;

    static struct line_pnts *Points = NULL;
    static struct line_cats *Cats = NULL;

    if (!Points) {
	Points = Vect_new_line_struct();
	Cats = Vect_new_cats_struct();
    }

    G_message(_("Writing edges..."));
    for (i = 0; i < n; i++) {
	G_percent(i, n, 2);
	u = &(sites[i]);
	e_start = e = u->entry_pt;
	do {
	    v = OTHER_VERTEX(e, u);
	    if (cmp(u, v) == 1) {
		x1 = u->x;
		y1 = u->y;
		z1 = u->z;
		x2 = v->x;
		y2 = v->y;
		z2 = v->z;

		Vect_reset_line(Points);

		Vect_append_point(Points, x1, y1, z1);
		Vect_append_point(Points, x2, y2, z2);
		Vect_write_line(Out, type, Points, Cats);
	    }
	    e = NEXT(e, u);
	} while (!SAME_EDGE(e, e_start));
    }
    G_percent(1, 1, 1);
}

/* Print the ring of triangles about each vertex. */

void output_triangles(unsigned int n,
		      int mode3d, int type, struct Map_info *Out)
{
    struct edge *e_start, *e, *next;
    struct vertex *u, *v, *w;
    unsigned int i;
    struct vertex *temp;

    struct line_pnts *Points = Vect_new_line_struct();
    struct line_cats *Cats = Vect_new_cats_struct();

    double x1, y1, z1, x2, y2, z2, x3, y3, z3;

    for (i = 0; i < n; i++) {
	u = &(sites[i]);
	e_start = e = u->entry_pt;
	do {
	    v = OTHER_VERTEX(e, u);
	    if (cmp(u, v) == 1) {
		next = NEXT(e, u);
		w = OTHER_VERTEX(next, u);
		if (cmp(u, w) == 1)
		    if (SAME_EDGE(NEXT(next, w), PREV(e, v))) {
			/* Triangle. */
			if (cmp(w, v) == 1) {
			    temp = v;
			    v = w;
			    w = temp;
			}
			x1 = u->x;
			y1 = u->y;
			x2 = v->x;
			y2 = v->y;
			x3 = w->x;
			y3 = w->y;
			z1 = u->z;
			z2 = v->z;
			z3 = w->z;

			Vect_reset_line(Points);
			Vect_append_point(Points, x1, y1, z1);
			Vect_append_point(Points, x2, y2, z2);
			Vect_write_line(Out, type, Points, Cats);

			Vect_reset_line(Points);
			Vect_append_point(Points, x2, y2, z2);
			Vect_append_point(Points, x3, y3, z3);
			Vect_write_line(Out, type, Points, Cats);

			Vect_reset_line(Points);
			Vect_append_point(Points, x3, y3, z3);
			Vect_append_point(Points, x1, y1, z1);
			Vect_write_line(Out, type, Points, Cats);
		    }
	    }
	    /* Next edge around u. */
	    e = NEXT(e, u);
	} while (!SAME_EDGE(e, e_start));
    }
}

void remove_duplicates(unsigned int *size)
{
    unsigned int n = *size;
    unsigned int prev = 0;
    unsigned int next;

    if (n > 0) {
	for (next = 1; next < n; next++) {
	    if (sites[prev].x != sites[next].x ||
		sites[prev].y != sites[next].y)
		sites[++prev] = sites[next];
	}
	*size = prev + 1;
    }
}

/* returns number of sites read */
int read_sites(int mode3d, int complete_map, struct Map_info* map_in,
	       struct bound_box Box, int field)
{
    int nlines, line, nsites;
    struct line_pnts *Points;
    struct line_cats *Cats;
    
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    nlines = Vect_get_num_lines(map_in);
    alloc_sites(nlines);
    
    nsites = 0;
    G_message(_("Reading point features..."));
    for (line = 1; line <= nlines; line++) {
	int type;

	G_percent(line, nlines, 2);
	type = Vect_read_line(map_in, Points, Cats, line);
	if (!(type & GV_POINTS))
	    continue;
	
	if (field != -1 && Vect_cat_get(Cats, field, NULL) == 0)
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
	else {
	    sites[nsites].z = 0.0;
	}
	/* Initialise entry edge vertices. */
	sites[nsites].entry_pt = NULL;

	nsites++;
#if 0
	/* number 100 was arbitrarily chosen */
	if (nsites == allocated && line != nlines){
	    allocated += 100;
	    realloc_sites(allocated);
	}
#endif
    }
    if (nsites != nlines)
	realloc_sites(nsites);
    alloc_edges(nsites);

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return nsites;
}
