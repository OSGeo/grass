#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "defs.h"

static int next_dist(int line, int side, double mf)
{
    double d, dist, nextdist, totaldist;
    int nextline, nlines;
    int node;
    static struct line_pnts *Points = NULL;

    G_debug(3, "next_dist()");

    if (!Points)
	Points = Vect_new_line_struct();

    Vect_read_line(&Out, Points, NULL, abs(line));
    dist = Vect_line_length(Points);

    if (line < 0)
	Vect_get_line_nodes(&Out, -line, &node, NULL);
    else
	Vect_get_line_nodes(&Out, line, NULL, &node);

    nlines = Vect_get_node_n_lines(&Out, node);

    if (nlines == 1)
	return 1;

    nextdist = totaldist = 0.;
    while (nlines > 1) {
	nextline = dig_angle_next_line(&(Out.plus), -line, side, GV_LINE, NULL);
	Vect_read_line(&Out, Points, NULL, abs(nextline));
	d = Vect_line_length(Points);
	nextdist += d;
	totaldist += d;

	if (nextline < 0)
	    Vect_get_line_nodes(&Out, -nextline, &node, NULL);
	else
	    Vect_get_line_nodes(&Out, nextline, NULL, &node);

	nlines = Vect_get_node_n_lines(&Out, node);
	if (nlines > 2)
	    nextdist = 0.;
	line = nextline;
    }

    if (totaldist > nextdist && dist > nextdist)
	return (totaldist < mf * dist);

    return (dist > nextdist);
}

static int loop_test(int line, int node, struct line_pnts *Points,
                     double l, double mf)
{
    int line1, line2, nextline;
    int n1, n2, nspikes, nout;
    double l1, minl, maxl;
    
    if (Vect_get_node_n_lines(&Out, node) != 3)
	return 0;
	
    line1 = line2 = 0;
    line1 = Vect_get_node_line(&Out, node, 0);
    if (abs(line1) == abs(line))
	line1 = 0;
    line2 = Vect_get_node_line(&Out, node, 1);
    if (abs(line2) == abs(line))
	line2 = 0;
    if (line1 == 0) {
	line1 = line2;
	line2 = 0;
    }
    if (line2 == 0)
	line2 = Vect_get_node_line(&Out, node, 2);
    
    if (abs(line1) == abs(line2))
	return 1;

    nextline = dig_angle_next_line(&(Out.plus), -line, GV_LEFT, GV_LINE, NULL);
    line1 = nextline;
    
    nspikes = 1;
    nout = 0;
    minl = 0;
    maxl = 0;
    do {
	if (nextline < 0)
	    Vect_get_line_nodes(&Out, -nextline, &n1, NULL);
	else
	    Vect_get_line_nodes(&Out, nextline, NULL, &n1);

	if (Vect_get_node_n_lines(&Out, n1) == 1)
	    return 0;

	if (n1 != node && Vect_get_node_n_lines(&Out, n1) > 2) {
	    nspikes++;
	    line2 = dig_angle_next_line(&(Out.plus), -nextline, GV_LEFT, GV_LINE, NULL);

	    if (line2 < 0)
		Vect_get_line_nodes(&Out, -line2, &n2, NULL);
	    else
		Vect_get_line_nodes(&Out, line2, NULL, &n2);

	    if (Vect_get_node_n_lines(&Out, n2) == 1) {
		Vect_read_line(&Out, Points, NULL, abs(line2));
		l1 = Vect_line_length(Points);
		if (minl == 0 || minl > l1) {
		    minl = l1;
		}
		if (maxl < l1) {
		    maxl = l1;
		}
	    }
	    else
		nout++;
	}
	
	nextline = dig_angle_next_line(&(Out.plus), -nextline, GV_RIGHT, GV_LINE, NULL);
	
    } while (abs(nextline) != abs(line1));

    if (minl == 0)
	minl = l;
    if (maxl == 0)
	maxl = mf * l;

    nspikes -= nout;

    if (mf > 1) {
	return (nspikes < 3 || (nspikes == 3 && (l > minl || mf * l > maxl)));

	if (l > minl)
	    return 1;
	if (nspikes == 3 && l < minl)
	    return (mf * l > maxl);
	else
	    return (nspikes < 3);
    }

    return (nspikes < 3 || (nspikes > 2 && l > minl));
}

static int break_loop(int line, int node, struct line_pnts *Points)
{
    int line1, line2, firstline, nextline;
    int n1;
    double l1, l2;
    
    if (Vect_get_node_n_lines(&Out, node) != 3)
	return 0;
	
    line1 = line2 = 0;
    line1 = Vect_get_node_line(&Out, node, 0);
    if (abs(line1) == abs(line))
	line1 = 0;
    line2 = Vect_get_node_line(&Out, node, 1);
    if (abs(line2) == abs(line))
	line2 = 0;
    if (line1 == 0) {
	line1 = line2;
	line2 = 0;
    }
    if (line2 == 0)
	line2 = Vect_get_node_line(&Out, node, 2);
    
    if (abs(line1) == abs(line2))
	return 1;

    nextline = dig_angle_next_line(&(Out.plus), -line, GV_LEFT, GV_LINE, NULL);
    firstline = nextline;

    do {
	if (nextline < 0)
	    Vect_get_line_nodes(&Out, -nextline, &n1, NULL);
	else
	    Vect_get_line_nodes(&Out, nextline, NULL, &n1);

	if (Vect_get_node_n_lines(&Out, n1) == 1)
	    return 0;

	nextline = dig_angle_next_line(&(Out.plus), -nextline, GV_RIGHT, GV_LINE, NULL);
	
    } while (abs(nextline) != abs(firstline));

    if (abs(nextline) != abs(firstline)) {
	G_warning("no loop ???");
	return 0;
    }

    Vect_read_line(&Out, Points, NULL, abs(line1));
    l1 = Vect_line_length(Points);

    Vect_read_line(&Out, Points, NULL, abs(line2));
    l2 = Vect_line_length(Points);

    if (l1 > l2)
	Vect_delete_line(&Out, abs(line1));
    else
	Vect_delete_line(&Out, abs(line2));

    Vect_merge_lines(&Out, GV_LINE, NULL, NULL);

    return 1;
}

static int length_test(int line, int node, struct line_pnts *Points,
                       double l, double mf)
{
    int line1, line2, nlines;
    int n1;
    double l1, l2;
    
    if (Vect_get_node_n_lines(&Out, node) != 3)
	return 0;

    line1 = Vect_get_node_line(&Out, node, 0);
    line2 = 0;
    if (abs(line1) == abs(line))
	line1 = 0;
    line2 = Vect_get_node_line(&Out, node, 1);
    if (abs(line2) == abs(line))
	line2 = 0;
    if (line1 == 0) {
	line1 = line2;
	line2 = 0;
    }
    if (line2 == 0)
	line2 = Vect_get_node_line(&Out, node, 2);

    if (line1 < 0)
	Vect_get_line_nodes(&Out, -line1, &n1, NULL);
    else
	Vect_get_line_nodes(&Out, line1, NULL, &n1);

    nlines = Vect_get_node_n_lines(&Out, n1);
    if (nlines > 1)
	return 0;

    if (line2 < 0)
	Vect_get_line_nodes(&Out, -line2, &n1, NULL);
    else
	Vect_get_line_nodes(&Out, line2, NULL, &n1);

    nlines = Vect_get_node_n_lines(&Out, n1);
    if (nlines > 1)
	return 0;

    Vect_read_line(&Out, Points, NULL, abs(line1));
    l1 = Vect_line_length(Points);

    Vect_read_line(&Out, Points, NULL, abs(line2));
    l2 = Vect_line_length(Points);

    if (l1 > mf * l2) {
	if (mf * l < l1 && l < l2)
	    return 0;
    }
    if (l2 > mf * l1) {
	if (mf * l < l2 && l < l1)
	    return 0;
    }

    return (mf * l > l1 || mf * l > l2);
}

/* thin the skeletons */
int thin_skeleton(double thresh)
{
    int i;
    int node, n1, n2;
    struct line_pnts *Points;
    struct ilist *list;
    double l, minl, maxl;
    int nlines, line, minline, line2;
    int counter = 1;
    double morphof = 1.6180339887;

    Points = Vect_new_line_struct();
    list = Vect_new_list();

    if (thresh < 0)
	morphof = 1;

    Vect_merge_lines(&Out, GV_LINE, NULL, NULL);

    while (TRUE) {
	G_verbose_message(_("Pass %d"), counter++);
	Vect_reset_list(list);

	for (node = 1; node <= Vect_get_num_nodes(&Out); node++) {
	    if (!Vect_node_alive(&Out, node))
		continue;
	    nlines = Vect_get_node_n_lines(&Out, node);
	    
	    if (nlines > 1)
		continue;

	    line = Vect_get_node_line(&Out, node, 0);
	    if (line < 0)
		Vect_get_line_nodes(&Out, -line, &n1, NULL);
	    else
		Vect_get_line_nodes(&Out, line, NULL, &n1);

	    nlines = Vect_get_node_n_lines(&Out, n1);
	    
	    if (nlines < 3)
		continue;

	    Vect_read_line(&Out, Points, NULL, abs(line));
	    minline = line;
	    minl = Vect_line_length(Points);

	    if (nlines == 3) {
		if (loop_test(line, n1, Points, minl, morphof))
		    continue;
		if (length_test(line, n1, Points, minl, morphof))
		    continue;
	    }

	    maxl = 0;
	    for (i = 0; i < nlines; i++) {
		line2 = Vect_get_node_line(&Out, n1, i);
		if (abs(line2) == abs(minline) || abs(line2) == abs(line))
		    continue;
		if (line2 < 0)
		    Vect_get_line_nodes(&Out, -line2, &n2, NULL);
		else
		    Vect_get_line_nodes(&Out, line2, NULL, &n2);
		if (Vect_get_node_n_lines(&Out, n2) > 1)
		    continue;
		Vect_read_line(&Out, Points, NULL, abs(line2));
		l = Vect_line_length(Points);
		if (minl > l) {
		    minl = l;
		    minline = line2;
		}
		if (maxl == 0 || maxl < l) {
		    maxl = l;
		}
	    }
	    if (thresh < 0) {
		G_ilist_add(list, minline);
	    }
	    else  {
		if (minl < thresh)
		    G_ilist_add(list, minline);
	    }
	}
	if (list->n_values == 0)
	    break;
	nlines = 0;
	for (i = 0; i < list->n_values; i++) {
	    line = list->value[i];
	    if (Vect_line_alive(&Out, abs(line))) {
		if (next_dist(line, GV_RIGHT, morphof))
		    continue;
		if (next_dist(line, GV_LEFT, morphof))
		    continue;
		Vect_delete_line(&Out, abs(line));
		nlines++;
	    }
	}
	if (nlines == 0)
	    break;
	else
	    Vect_merge_lines(&Out, GV_LINE, NULL, NULL);
    }

    if (thresh >= 0)
	return 0;

    for (node = 1; node <= Vect_get_num_nodes(&Out); node++) {
	if (!Vect_node_alive(&Out, node))
	    continue;
	nlines = Vect_get_node_n_lines(&Out, node);
	
	if (nlines > 1)
	    continue;

	line = Vect_get_node_line(&Out, node, 0);
	if (line < 0)
	    Vect_get_line_nodes(&Out, -line, &n1, NULL);
	else
	    Vect_get_line_nodes(&Out, line, NULL, &n1);

	nlines = Vect_get_node_n_lines(&Out, n1);
	
	if (nlines == 3) {
	    break_loop(line, n1, Points);
	}
    }

    while (TRUE) {
	G_verbose_message(_("Pass %d"), counter++);
	Vect_reset_list(list);

	for (node = 1; node <= Vect_get_num_nodes(&Out); node++) {
	    if (!Vect_node_alive(&Out, node))
		continue;
	    nlines = Vect_get_node_n_lines(&Out, node);
	    
	    if (nlines > 1)
		continue;

	    line = Vect_get_node_line(&Out, node, 0);
	    if (line < 0)
		Vect_get_line_nodes(&Out, -line, &n1, NULL);
	    else
		Vect_get_line_nodes(&Out, line, NULL, &n1);

	    nlines = Vect_get_node_n_lines(&Out, n1);
	    
	    if (nlines < 3)
		continue;

	    Vect_read_line(&Out, Points, NULL, abs(line));
	    minline = line;
	    minl = Vect_line_length(Points);

	    if (nlines == 3) {
		if (break_loop(line, n1, Points))
		    continue;
	    }

	    for (i = 0; i < nlines; i++) {
		line2 = Vect_get_node_line(&Out, n1, i);
		if (abs(line2) == abs(minline) || abs(line2) == abs(line))
		    continue;
		if (line2 < 0)
		    Vect_get_line_nodes(&Out, -line2, &n2, NULL);
		else
		    Vect_get_line_nodes(&Out, line2, NULL, &n2);
		if (Vect_get_node_n_lines(&Out, n2) > 1)
		    continue;
		Vect_read_line(&Out, Points, NULL, abs(line2));
		l = Vect_line_length(Points);
		if (minl > l) {
		    minl = l;
		    minline = line2;
		}
	    }
	    if (thresh < 0 || minl < thresh)
		G_ilist_add(list, minline);
	}
	if (list->n_values == 0)
	    break;
	nlines = 0;
	for (i = 0; i < list->n_values; i++) {
	    line = list->value[i];
	    if (Vect_line_alive(&Out, abs(line))) {
		if (next_dist(line, GV_RIGHT, morphof))
		    continue;
		if (next_dist(line, GV_LEFT, morphof))
		    continue;
		Vect_delete_line(&Out, abs(line));
		nlines++;
	    }
	}
	if (nlines == 0)
	    break;
	else
	    Vect_merge_lines(&Out, GV_LINE, NULL, NULL);
    }

    return 0;
}

int tie_up(void)
{
    int i;
    int node;
    int nlines;
    double xmin, ymin, x, y;
    double dx, dy, dist, distmin;
    struct line_pnts *Points;
    struct line_pnts **IPoints;
    struct line_cats *Cats;
    int isl_allocated;
    int area, isle, n_isles;
    int ntied = 0;
 
    Points = Vect_new_line_struct();
    isl_allocated = 10;
    IPoints = G_malloc(isl_allocated * sizeof(struct line_pnts *));
    for (i = 0; i < isl_allocated; i++)
	IPoints[i] = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    for (node = 1; node <= Vect_get_num_nodes(&Out); node++) {
	if (!Vect_node_alive(&Out, node))
	    continue;
	nlines = Vect_get_node_n_lines(&Out, node);
	
	if (nlines > 1)
	    continue;

	Vect_get_node_coor(&Out, node, &x, &y, NULL);

	/* find area for this node */
	area = Vect_find_area(&In, x, y);
	
	if (area == 0)
	    G_fatal_error(_("Node is outside any input area"));

	/* get area outer ring */
	Vect_get_area_points(&In, area, Points);

	/* get area inner rings */
	n_isles = Vect_get_area_num_isles(&In, area);
	if (n_isles > isl_allocated) {
	    IPoints = (struct line_pnts **)
		G_realloc(IPoints, (1 + n_isles) * sizeof(struct line_pnts *));
	    for (i = isl_allocated; i < n_isles; i++)
		IPoints[i] = Vect_new_line_struct();
	    isl_allocated = n_isles;
	}
	for (i = 0; i < n_isles; i++) {
	    Vect_get_isle_points(&In, Vect_get_area_isle(&In, area, i),
	                         IPoints[i]);
	}

	distmin = 1. / 0.; /* +inf */
	xmin = x;
	ymin = y;

	/* find closest point to outer ring */
	/* must be an existing vertex */
	for (i = 0; i < Points->n_points - 1; i++) {
	    dx = x - Points->x[i];
	    dy = y - Points->y[i];
	    
	    dist = dx * dx + dy * dy;
	    
	    if (distmin > dist) {
		distmin = dist;
		xmin = Points->x[i];
		ymin = Points->y[i];
	    }
	}

	/* find closest point to inner rings */
	/* must be an existing vertex */
	for (isle = 0; isle < n_isles; isle++) {
	    for (i = 0; i < IPoints[isle]->n_points - 1; i++) {
		dx = x - IPoints[isle]->x[i];
		dy = y - IPoints[isle]->y[i];
		
		dist = dx * dx + dy * dy;
		
		if (distmin > dist) {
		    distmin = dist;
		    xmin = IPoints[isle]->x[i];
		    ymin = IPoints[isle]->y[i];
		}
	    }
	}

	if (xmin != x && ymin != y) {
	    Vect_get_area_cats(&In, area, Cats);
	    Vect_reset_line(Points);
	    Vect_append_point(Points, xmin, ymin, 0);
	    Vect_append_point(Points, x, y, 0);
	    Vect_write_line(&Out, GV_LINE, Points, Cats);
	    ntied++;
	}
    }
   
    return ntied;
}
