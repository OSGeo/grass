#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include "sw_defs.h"
#include "defs.h"

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
	siteidx += 1;
	return (s);
    }
    else
	return ((struct Site *)NULL);
}

/* removes duplicate sites that would break the voronoi alghoritm */
void removeDuplicates()
{
    int i, j;
    int foundDupe;

    i = j = 1;
    foundDupe = 0;
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
	sites = (struct Site *)G_realloc(sites, nsites * sizeof(*sites));
    }

}

/* read all sites, sort, and compute xmin, xmax, ymin, ymax */
int readsites(void)
{
    int i, nlines, line;
    struct line_pnts *Points;

    Points = Vect_new_line_struct();

    nlines = Vect_get_num_lines(&In);

    nsites = 0;
    sites = (struct Site *)myalloc(nlines * sizeof(*sites));

    for (line = 1; line <= nlines; line++) {
	int type;

	type = Vect_read_line(&In, Points, NULL, line);
	if (!(type & GV_POINTS))
	    continue;

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

	sites[nsites].sitenbr = nsites;
	sites[nsites].refcnt = 0;
	nsites += 1;
	if (nsites % 4000 == 0)
	    sites =
		(struct Site *)G_realloc(sites,
					 (nsites + 4000) * sizeof(*sites));
    }

    qsort(sites, nsites, sizeof(*sites), scomp);
    removeDuplicates();
    xmin = sites[0].coord.x;
    xmax = sites[0].coord.x;
    for (i = 1; i < nsites; i += 1) {
	if (sites[i].coord.x < xmin)
	    xmin = sites[i].coord.x;
	if (sites[i].coord.x > xmax)
	    xmax = sites[i].coord.x;
    }
    ymin = sites[0].coord.y;
    ymax = sites[nsites - 1].coord.y;
    return 0;
}

/* read one site */
struct Site *readone(void)
{
    struct Site *s;

    s = (struct Site *)getfree(&sfl);
    s->refcnt = 0;
    s->sitenbr = siteidx;
    siteidx += 1;

    if (scanf("%lf %lf", &(s->coord.x), &(s->coord.y)) == EOF)
	return ((struct Site *)NULL);

    return (s);
}
