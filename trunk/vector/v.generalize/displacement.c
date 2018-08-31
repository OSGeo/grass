
/****************************************************************
 *
 * MODULE:     v.generalize
 *
 * AUTHOR(S):  Daniel Bundala
 *
 * PURPOSE:    Methods for displacement
 *
 * COPYRIGHT:  (C) 2002-2005 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "point.h"
#include "matrix.h"

/* snakes method modified for displacement.
 * Function returns something. This function affects only the
 * lines specified in varray (or all lines if varray is null).
 Other lines are copied */
int snakes_displacement(struct Map_info *In, struct Map_info *Out,
			double threshold, double alpha, double beta,
			double gama, double delta, int iterations,
			struct cat_list *cat_list, int layer)
{

    int n_points;
    int n_lines;
    int i, j, index, pindex, iter, type;
    int with_z = 0;
    struct line_pnts *Points;
    struct line_cats *Cats;
    MATRIX k, dx, dy, fx, fy, kinv, dx_old, dy_old;
    POINT *parray;
    POINT *pset;
    int *point_index;
    int *first, *line_index, *need, *sel, *tmp_index;
    double threshold2;
    int selected;

    /* initialize structrures and read the number of points */
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    n_lines = Vect_get_num_lines(In);
    n_points = 0;

    for (i = 1; i <= n_lines; i++) {
	type = Vect_read_line(In, Points, Cats, i);
	if (layer > 0 && !Vect_cats_in_constraint(Cats, layer, cat_list))
	    continue;
	if (type & GV_LINE)
	    n_points += Points->n_points;
    }

    parray = (POINT *) G_calloc(n_points, sizeof(POINT));
    pset = (POINT *) G_calloc(n_points, sizeof(POINT));
    point_index = (int *)G_calloc(n_points, sizeof(int));
    first = (int *)G_calloc(n_points, sizeof(int));
    line_index = (int *)G_calloc(n_points, sizeof(int));
    need = (int *)G_calloc(n_points, sizeof(int));
    sel = (int *)G_calloc(n_points, sizeof(int));
    tmp_index = (int *)G_calloc(n_points, sizeof(int));

    /* read points 
     * TODO: some better/faster method for determining whether two points are the same */
    G_percent_reset();
    G_message(_("Reading data..."));
    index = 0;
    pindex = 0;
    for (i = 1; i <= n_lines; i++) {
	G_percent(i, n_lines, 1);
	type = Vect_read_line(In, Points, Cats, i);
	if (type != GV_LINE)
	    continue;
	if (layer > 0 && !Vect_cats_in_constraint(Cats, layer, cat_list))
	    continue;

	for (j = 0; j < Points->n_points; j++) {
	    int q, findex;
	    POINT cur;

	    point_assign(Points, j, with_z, &cur, 0);
	    /* check whether we alerady have point with the same
	     * coordinates */
	    findex = pindex;
	    for (q = 0; q < pindex; q++)
		if (point_dist_square(cur, pset[q]) < 0.5) {
		    findex = q;
		    break;
		}

	    point_index[index] = findex;
	    if (findex == pindex) {
		point_assign(Points, j, with_z, &pset[pindex], 0);
		pindex++;
	    }
	    first[index] = (j == 0);
	    line_index[index] = i;
	    point_assign(Points, j, with_z, &parray[index], 0);
	    index++;
	}
    }

    threshold2 = threshold * threshold;
    /*select only the points which need to be displaced */
    for (i = 0; i < index; i++) {
	if (need[point_index[i]])
	    continue;
	for (j = 1; j < index; j++) {
	    if (line_index[i] == line_index[j] || first[j] ||
		point_index[i] == point_index[j] ||
		point_index[i] == point_index[j - 1])
		continue;
	    double d =
		point_dist_segment_square(parray[i], parray[j], parray[j - 1],
					  with_z);

	    if (d < 4 * threshold2)
		need[point_index[i]] = 1;
	}
    }

    /* then for each selected point ensure that the neighbours to the both
     * sides are selected as well */
    for (i = 0; i < index; i++) {
	int l = line_index[i];

	tmp_index[i] = -1;
	if (!need[point_index[i]])
	    continue;
	for (j = -2; j <= 2; j++)
	    if (i + j >= 0 && i + j < index && line_index[i + j] == l)
		sel[point_index[i + j]] = 1;
    }

    /* finally, recalculate indices */
    selected = 0;
    for (i = 0; i < pindex; i++)
	if (sel[i])
	    tmp_index[i] = selected++;

    for (i = 0; i < index; i++)
	point_index[i] = tmp_index[point_index[i]];
    pindex = selected;

    G_debug(3, "Number of conflicting points: %d", pindex);

    /* initialize matrices */
    matrix_init(pindex, pindex, &k);
    matrix_init(pindex, 1, &dx);
    matrix_init(pindex, 1, &dy);
    matrix_init(pindex, 1, &fx);
    matrix_init(pindex, 1, &fy);
    matrix_init(pindex, 1, &dx_old);
    matrix_init(pindex, 1, &dy_old);

    matrix_mult_scalar(0.0, &k);

    double a = 2.0 * alpha + 6.0 * beta;
    double b = -alpha - 4.0 * beta;
    double c = beta;

    /* build matrix */
    for (i = 0; i < index; i++) {
	int r = point_index[i];
	int l = line_index[i];

	if (r == -1)
	    continue;
	k.a[r][r] += a;
	if (i + 1 < index && line_index[i + 1] == l &&
	    point_index[i + 1] != -1)
	    k.a[r][point_index[i + 1]] += b;
	if (i + 2 < index && line_index[i + 2] == l &&
	    point_index[i + 2] != -1)
	    k.a[r][point_index[i + 2]] += c;
	if (i >= 1 && line_index[i - 1] == l && point_index[i - 1] != -1)
	    k.a[r][point_index[i - 1]] += b;
	if (i >= 2 && line_index[i - 2] == l && point_index[i - 2] != -1)
	    k.a[r][point_index[i - 2]] += c;
    }

    matrix_add_identity(gama, &k);
    matrix_mult_scalar(0.0, &dx);
    matrix_mult_scalar(0.0, &dy);

    /*calculate the inverse */
    G_message(_("Inverting matrix..."));
    if (!matrix_inverse(&k, &kinv, 1))
	G_fatal_error(_("Unable to calculate the inverse matrix"));

    G_percent_reset();
    G_message(_("Resolving conflicts..."));
    for (iter = 0; iter < iterations; iter++) {
	int conflicts = 0;

	G_percent(iter, iterations, 1);

	matrix_mult_scalar(0.0, &fx);
	matrix_mult_scalar(0.0, &fy);

	matrix_mult_scalar(0.0, &dx_old);
	matrix_mult_scalar(0.0, &dy_old);

	matrix_add(&dx_old, &dx, &dx_old);
	matrix_add(&dy_old, &dy, &dy_old);

	/* calculate force vectors */
	for (i = 0; i < index; i++) {

	    double cx, cy, f;

	    if (point_index[i] == -1)
		continue;
	    cx = dx.a[point_index[i]][0];
	    cy = dy.a[point_index[i]][0];
	    f = sqrt(cx * cx + cy * cy) * alpha;
	    f /= threshold2;
	    fx.a[point_index[i]][0] -= cx * f;
	    fy.a[point_index[i]][0] -= cy * f;

	    for (j = 1; j < index; j++) {
		if (line_index[i] == line_index[j] || first[j] ||
		    point_index[i] == point_index[j] ||
		    point_index[i] == point_index[j - 1])
		    continue;
		/* if ith point is close to some segment then
		 * apply force to ith point. If the distance
		 * is zero, do not move the points */
		double d, pdist;
		POINT in;
		int status;

		d = dig_distance2_point_to_line(parray[i].x, parray[i].y,
						parray[i].z, parray[j].x,
						parray[j].y, parray[j].z,
						parray[j - 1].x,
						parray[j - 1].y,
						parray[j - 1].z, with_z,
						&in.x, &in.y, &in.z, &pdist,
						&status);

		POINT dir;

		if (d == 0.0 || d > threshold2)
		    continue;
		d = sqrt(d);
		point_subtract(parray[i], in, &dir);
		point_scalar(dir, 1.0 / d, &dir);
		point_scalar(dir, 1.0 - d / threshold, &dir);
		fx.a[point_index[i]][0] += dir.x;
		fy.a[point_index[i]][0] += dir.y;
		conflicts++;
	    }
	}

	/* calculate new displacement */
	matrix_mult_scalar(delta, &fx);
	matrix_mult_scalar(delta, &fy);
	matrix_mult_scalar(gama, &dx);
	matrix_mult_scalar(gama, &dy);

	matrix_add(&dx, &fx, &fx);
	matrix_add(&dy, &fy, &fy);

	matrix_mult(&kinv, &fx, &dx);
	matrix_mult(&kinv, &fy, &dy);

	for (i = 0; i < index; i++) {
	    if (point_index[i] == -1)
		continue;
	    parray[i].x +=
		dx.a[point_index[i]][0] - dx_old.a[point_index[i]][0];
	    parray[i].y +=
		dy.a[point_index[i]][0] - dy_old.a[point_index[i]][0];
	}

    }
    index = 0;
    for (i = 1; i <= n_lines; i++) {
	int type = Vect_read_line(In, Points, Cats, i);

	if (type != GV_LINE ||
	    (layer > 0 && !Vect_cats_in_constraint(Cats, layer, cat_list))) {
	    Vect_write_line(Out, type, Points, Cats);
	    continue;
	}
	for (j = 0; j < Points->n_points; j++) {
	    Points->x[j] = parray[index].x;
	    Points->y[j] = parray[index].y;
	    index++;
	}
	Vect_write_line(Out, type, Points, Cats);
    }

    G_free(parray);
    G_free(pset);
    G_free(point_index);
    G_free(first);
    G_free(line_index);
    G_free(need);
    G_free(sel);
    G_free(tmp_index);
    matrix_free(&k);
    matrix_free(&kinv);
    matrix_free(&dx);
    matrix_free(&dy);
    matrix_free(&fx);
    matrix_free(&fy);
    matrix_free(&dx_old);
    matrix_free(&dy_old);

    return 0;
}
