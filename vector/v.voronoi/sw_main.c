#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "sw_defs.h"
#include "defs.h"

int triangulate, sorted, plot, debug, mode3d;
struct Site *sites;
int nsites;
int siteidx;
int sqrt_nsites;
int nvertices;
struct Freelist sfl;
struct Site *bottomsite;
int nedges;
struct Freelist efl;
double xmin, xmax, ymin, ymax, deltax, deltay;
struct Freelist hfl;
struct Halfedge *ELleftend, *ELrightend;
int ELhashsize;
struct Halfedge **ELhash;
int PQhashsize;
struct Halfedge *PQhash;
int PQcount;
int PQmin;

struct Cell_head Window;
struct bound_box Box;
struct Map_info In, Out;
int Type;
int All;
int Field;

/* sort sites on y, then x, coord */
int scomp(const void *v1, const void *v2)
{
    struct Point *s1 = (struct Point *)v1;
    struct Point *s2 = (struct Point *)v2;

    if (s1->y < s2->y)
	return (-1);
    if (s1->y > s2->y)
	return (1);
    if (s1->x < s2->x)
	return (-1);
    if (s1->x > s2->x)
	return (1);
    return (0);
}

/* return a single in-storage site */
struct Site *nextone(void)
{
    struct Site *s;

    if (siteidx < nsites) {
	s = &sites[siteidx];
	siteidx++;
	return (s);
    }
    else
	return ((struct Site *)NULL);
}

/* removes duplicate sites that would break the voronoi alghoritm */
void removeDuplicates()
{
    int i, j;

    i = j = 1;
    while (i < nsites)
	if (mode3d) {
	    if (sites[i].coord.x == sites[i - 1].coord.x &&
		sites[i].coord.y == sites[i - 1].coord.y &&
		sites[i].coord.z == sites[i - 1].coord.z)
		i++;
	    else {
		if (i != j)
		    sites[j] = sites[i];
		i++;
		j++;;
	    }
	}
	else {
	    if (sites[i].coord.x == sites[i - 1].coord.x &&
		sites[i].coord.y == sites[i - 1].coord.y)
		i++;
	    else {
		if (i != j)
		    sites[j] = sites[i];
		i++;
		j++;;
	    }
	}

    if (j != nsites) {
	nsites = j;
	sites = (struct Site *)G_realloc(sites, nsites * sizeof(struct Site));
    }

}

/* read all sites, sort, and compute xmin, xmax, ymin, ymax */
int readsites(void)
{
    int nlines, ltype;
    struct line_pnts *Points;
    struct line_cats *Cats;
    
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    
    nlines = Vect_get_num_primitives(&In, GV_POINTS);

    nsites = 0;
    sites = (struct Site *)G_malloc(nlines * sizeof(struct Site));

    Vect_set_constraint_type(&In, GV_POINTS);
    Vect_set_constraint_field(&In, Field);
    
    while(TRUE) {
	ltype = Vect_read_next_line(&In, Points, Cats);
	if(ltype == -2)
	    break;

	G_percent(Vect_get_next_line_id(&In), nlines, 2);
		
	if (!All) {
	    if (!Vect_point_in_box(Points->x[0], Points->y[0], 0.0, &Box))
		continue;
	}

	sites[nsites].coord.x = Points->x[0];
	sites[nsites].coord.y = Points->y[0];
	if (mode3d) {
	    G_debug(3, "Points->z[0]: %f", Points->z[0]);
	    sites[nsites].coord.z = Points->z[0];
	}
	else
	    sites[nsites].coord.z = 0.0;

	if (nsites > 1) {
	    if (xmin > sites[nsites].coord.x)
		xmin = sites[nsites].coord.x;
	    if (xmax < sites[nsites].coord.x)
		xmax = sites[nsites].coord.x;
	    if (ymin > sites[nsites].coord.y)
		ymin = sites[nsites].coord.y;
	    if (ymax < sites[nsites].coord.y)
		ymax = sites[nsites].coord.y;
	}
	else {
	    xmin = xmax = sites[nsites].coord.x;
	    ymin = ymax = sites[nsites].coord.y;
	}

	nsites++;
    }

    if (nsites < 3) {
	const char *name = Vect_get_full_name(&In);
	Vect_close(&In);
	G_fatal_error(_("Found %d points/centroids in <%s>, but at least 3 are needed"),
	              nsites, name);
    }

    if (nsites < nlines)
	sites =
	    (struct Site *)G_realloc(sites,
				     (nsites) * sizeof(struct Site));

    if (xmin == Box.W)
	Box.W -= GRASS_EPSILON;
    if (xmax == Box.E)
	Box.E += GRASS_EPSILON;
    if (ymin == Box.S)
	Box.S -= GRASS_EPSILON;
    if (ymax == Box.N)
	Box.N += GRASS_EPSILON;


    qsort(sites, nsites, sizeof(struct Site), scomp);
    removeDuplicates();

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return 0;
}

/* read one site */
struct Site *readone(void)
{
    struct Site *s;

    s = (struct Site *)getfree(&sfl);
    s->refcnt = 0;
    s->sitenbr = siteidx;
    siteidx++;

    if (scanf("%lf %lf", &(s->coord.x), &(s->coord.y)) == EOF)
	return ((struct Site *)NULL);

    return (s);
}
