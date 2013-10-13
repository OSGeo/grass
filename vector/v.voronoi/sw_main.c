#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "sw_defs.h"
#include "defs.h"

int sorted, plot, debug, mode3d;
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
int Field;
int in_area;
int skeleton;
double segf;

int nsites_alloc;

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

int addsite(double x, double y, double z, int id)
{
    if (nsites >= nsites_alloc) {
	nsites_alloc += 100;
	sites =
	    (struct Site *)G_realloc(sites,
				     (nsites_alloc) * sizeof(struct Site));
    }
    sites[nsites].coord.x = x;
    sites[nsites].coord.y = y;
    sites[nsites].coord.z = z;

    sites[nsites].sitenbr = id;
    sites[nsites].refcnt = 0;

    if (nsites > 0) {
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

    return nsites;
}

/* read all sites, sort, and compute xmin, xmax, ymin, ymax */
int readsites(void)
{
    int nlines, ltype;
    struct line_pnts *Points;
    struct line_cats *Cats;
    double z = 0.;
    
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

	if (!(ltype & GV_POINTS))
	    continue;
	/* G_percent(Vect_get_next_line_id(&In), nlines, 2); */

	if (!Vect_point_in_box(Points->x[0], Points->y[0], 0.0, &Box))
	    continue;

	if (mode3d) {
	    G_debug(3, "Points->z[0]: %f", Points->z[0]);
	    z = Points->z[0];
	}

	addsite(Points->x[0], Points->y[0], z, nsites);
    }

    if (nsites < 2) {
	const char *name = Vect_get_full_name(&In);
	Vect_close(&In);
	G_fatal_error(_("Found %d points/centroids in <%s>, but at least 2 are needed"),
	              nsites, name);
    }

    if (nsites < nlines)
	sites =
	    (struct Site *)G_realloc(sites,
				     (nsites) * sizeof(struct Site));

    qsort(sites, nsites, sizeof(struct Site), scomp);
    removeDuplicates();

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return 0;
}

/* valid boundary: one area with centroid */
int n_areas(int line, int *aid)
{
    int larea, rarea, ncentroids;
    
    ncentroids = 0;
    Vect_get_line_areas(&In, line, &larea, &rarea);
    
    if (larea < 0)
	larea = Vect_get_isle_area(&In, -larea);
    if (larea > 0) {
	if (Vect_get_area_centroid(&In, larea) > 0) {
	    ncentroids++;
	    *aid = larea;
	}
    }
    if (rarea < 0)
	rarea = Vect_get_isle_area(&In, -rarea);
    if (rarea > 0) {
	if (Vect_get_area_centroid(&In, rarea) > 0) {
	    ncentroids++;
	    *aid = rarea;
	}
    }
    
    return ncentroids;
}

/* read all boundaries, sort, and compute xmin, xmax, ymin, ymax */
int readbounds(void)
{
    int line, nlines, ltype, node, nnodes;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int i;
    int area_id;
    double x, y, z, x1, y1, z1;
    double maxdist, sdist, l, dx, dy, dz;
    int nconnected;
    struct ilist *linelist, *arealist;
    
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    
    nlines = Vect_get_num_lines(&In);

    nsites = 0;
    nsites_alloc = nlines * 2;
    sites = (struct Site *)G_malloc(nsites_alloc * sizeof(struct Site));

    Vect_set_constraint_type(&In, GV_BOUNDARY);
    Vect_set_constraint_field(&In, Field);
    
    l = 0;
    maxdist = 0;
    for (line = 1; line <= nlines; line++) {

	if (!Vect_line_alive(&In, line))
	    continue;
	ltype = Vect_get_line_type(&In, line);

	if (!(ltype & GV_BOUNDARY))
	    continue;

	area_id = 0;
	if (n_areas(line, &area_id) != 1)
	    continue;

	Vect_read_line(&In, Points, Cats, line);
	Vect_line_prune(Points);
	
	l += Vect_line_length(Points);
	nsites += Points->n_points;
    }
    if (nsites)
	maxdist = segf * l / nsites;
    G_verbose_message("Maximum segment length: %g", maxdist);
    
    nsites = 0;
    z = 0.;
    z1 = 0;
    dz = 0;
    for (line = 1; line <= nlines; line++) {

	if (!Vect_line_alive(&In, line))
	    continue;
	ltype = Vect_get_line_type(&In, line);

	if (!(ltype & GV_BOUNDARY))
	    continue;

	area_id = 0;
	if (n_areas(line, &area_id) != 1)
	    continue;

	Vect_read_line(&In, Points, Cats, line);
	Vect_line_prune(Points);

	if (nsites + Points->n_points > nsites_alloc) {
	    nsites_alloc = nsites + Points->n_points;
	    sites =
		(struct Site *)G_realloc(sites,
					 (nsites_alloc) * sizeof(struct Site));
	}

	for (i = 0; i < Points->n_points; i++) {
	    if (!Vect_point_in_box(Points->x[i], Points->y[i], 0.0, &Box))
		continue;

	    x = Points->x[i];
	    y = Points->y[i];
	    if (mode3d) {
		G_debug(3, "Points->z[i]: %f", Points->z[i]);
		z = Points->z[i];
	    }

	    if (i > 0 && i < Points->n_points - 1)
		addsite(x, y, z, area_id);
	    
	    /* densify */
	    if (maxdist > 0 && i < Points->n_points - 1) {
		dx = Points->x[i + 1] - Points->x[i];
		dy = Points->y[i + 1] - Points->y[i];
		if (mode3d)
		    dz = Points->z[i + 1] - Points->z[i];
		l = sqrt(dx * dx + dy * dy);
		
		if (l > maxdist) {
		    int n = ceil(l / maxdist) + 0.5;
		    double step = l / n;
		    
		    while (--n) {
			sdist = (step * n) / l;
			x1 = x + sdist * dx;
			y1 = y + sdist * dy;
			if (mode3d)
			    z1 = z + sdist * dz;

			addsite(x1, y1, z1, area_id);
		    }
		}
	    }
	}
    }
    
    /* process nodes */
    nnodes = Vect_get_num_nodes(&In);
    linelist = Vect_new_list();
    arealist = Vect_new_list();
    
    for (node = 1; node <= nnodes; node++) {
	Vect_get_node_coor(&In, node, &x, &y, &z);
	if (!mode3d)
	    z = 0.;

	if (!Vect_point_in_box(x, y, 0.0, &Box))
	    continue;

	/* count number of connected boundaries
	 * must be >= 2 */
	/* count number of valid boundaries */
	nlines = Vect_get_node_n_lines(&In, node);
	nconnected = 0;
	Vect_reset_list(linelist);
	Vect_reset_list(arealist);
	
	for (i = 0; i < nlines; i++) {
	    line = Vect_get_node_line(&In, node, i);
	    
	    ltype = Vect_get_line_type(&In, abs(line));
	    
	    if (!(ltype & GV_BOUNDARY))
		continue;
	    
	    if (n_areas(abs(line), &area_id) == 1) {
		Vect_list_append(linelist, line);
		Vect_list_append(arealist, area_id);
		nconnected++;
	    }
	}
	
	if (arealist->n_values == 1) {
	    
	    area_id = arealist->value[0];
	    addsite(x, y, z, area_id);
	}
	else if (arealist->n_values > 1) {
	    /* displacement */
	    double displace;
	    
	    if (fabs(x) > fabs(y))
		displace = d_ulp(fabs(x));
	    else
		displace = d_ulp(fabs(y));
	    
	    displace *= 2;
	    
	    for (i = 0; i < linelist->n_values; i++) {

		line = linelist->value[i];
		
		if (n_areas(abs(line), &area_id) != 1)
		    G_fatal_error(_("All boundaries in the list should be valid"));

		Vect_read_line(&In, Points, Cats, abs(line));
		Vect_line_prune(Points);
		
		if (Points->n_points < 2)
		    G_fatal_error(_("Boundary is degenerate"));

		if (line < 0) {
		    dx = Points->x[Points->n_points - 2] - Points->x[Points->n_points - 1]; 
		    dy = Points->y[Points->n_points - 2] - Points->y[Points->n_points - 1]; 
		}
		else {
		    dx = Points->x[1] - Points->x[0]; 
		    dy = Points->y[1] - Points->y[0]; 
		}
		l = sqrt(dx * dx + dy * dy);
		if (displace > l)
		    displace = l;
	    }

	    for (i = 0; i < linelist->n_values; i++) {

		line = linelist->value[i];
		
		if (n_areas(abs(line), &area_id) != 1)
		    G_fatal_error(_("All boundaries in the list should be valid"));

		Vect_read_line(&In, Points, Cats, abs(line));
		Vect_line_prune(Points);
		
		if (Points->n_points < 2)
		    G_fatal_error(_("Boundary is degenerate"));

		if (line < 0) {
		    dx = Points->x[Points->n_points - 2] - Points->x[Points->n_points - 1]; 
		    dy = Points->y[Points->n_points - 2] - Points->y[Points->n_points - 1]; 
		}
		else {
		    dx = Points->x[1] - Points->x[0]; 
		    dy = Points->y[1] - Points->y[0]; 
		}
		l = sqrt(dx * dx + dy * dy);
		if (l > displace * 2) {
		    sdist = displace / l;
		    x1 = x + sdist * dx;
		    y1 = y + sdist * dy;
		    z1 = 0;

		    addsite(x1, y1, z1, area_id);
		}
	    }
	}
    }

    if (nsites < 2) {
	const char *name = Vect_get_full_name(&In);
	Vect_close(&In);
	G_fatal_error(_("Found %d vertices in <%s>, but at least 2 are needed"),
	              nsites, name);
    }


    qsort(sites, nsites, sizeof(struct Site), scomp);
    removeDuplicates();

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);
    Vect_destroy_list(linelist);
    Vect_destroy_list(arealist);

    return 0;
}
