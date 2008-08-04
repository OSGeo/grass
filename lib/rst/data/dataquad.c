
/*-
 * Written by H. Mitasova, I. Kosinovsky, D. Gerdes Fall 1993
 * University of Illinois
 * US Army Construction Engineering Research Lab  
 * Copyright 1993, H. Mitasova (University of Illinois),
 * I. Kosinovsky, (USA-CERL), and D.Gerdes (USA-CERL)   
 *
 * Modified by H.Mitasova November 1996 to include variable smoothing
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <grass/dataquad.h>

/* sm added to point structure */
struct triple *quad_point_new(double x, double y, double z, double sm)
/* Initializes POINT structure with given arguments */
{
    struct triple *point;

    if (!(point = (struct triple *)malloc(sizeof(struct triple)))) {
	return NULL;
    }

    point->x = x;
    point->y = y;
    point->z = z;
    point->sm = sm;

    return point;
}


struct quaddata *quad_data_new(double x_or, double y_or, double xmax,
			       double ymax, int rows, int cols, int n_points,
			       int kmax)
/* Initializes QUADDATA structure with given arguments */
{
    struct quaddata *data;
    int i;

    if (!(data = (struct quaddata *)malloc(sizeof(struct quaddata)))) {
	return NULL;
    }

    data->x_orig = x_or;
    data->y_orig = y_or;
    data->xmax = xmax;
    data->ymax = ymax;
    data->n_rows = rows;
    data->n_cols = cols;
    data->n_points = n_points;
    data->points =
	(struct triple *)malloc(sizeof(struct triple) * (kmax + 1));
    if (!data->points)
	return NULL;
    for (i = 0; i <= kmax; i++) {
	data->points[i].x = 0.;
	data->points[i].y = 0.;
	data->points[i].z = 0.;
	data->points[i].sm = 0.;
    }

    return data;
}





int quad_compare(struct triple *point, struct quaddata *data)
/* returns the quadrant the point should be inserted in */
/* called by divide() */
{
    int cond1, cond2, cond3, cond4, rows, cols;
    double ew_res, ns_res;

    ew_res = (data->xmax - data->x_orig) / data->n_cols;
    ns_res = (data->ymax - data->y_orig) / data->n_rows;


    if (data == NULL)
	return -1;
    if (data->n_rows % 2 == 0) {
	rows = data->n_rows / 2;
    }
    else {
	rows = (int)(data->n_rows / 2) + 1;
    }

    if (data->n_cols % 2 == 0) {
	cols = data->n_cols / 2;
    }
    else {
	cols = (int)(data->n_cols / 2) + 1;
    }
    cond1 = (point->x >= data->x_orig);
    cond2 = (point->x >= data->x_orig + ew_res * cols);
    cond3 = (point->y >= data->y_orig);
    cond4 = (point->y >= data->y_orig + ns_res * rows);
    if (cond1 && cond3) {
	if (cond2 && cond4)
	    return NE;
	if (cond2)
	    return SE;
	if (cond4)
	    return NW;
	return SW;
    }
    else
	return 0;
}


int quad_add_data(struct triple *point, struct quaddata *data, double dmin)
/* Adds POINT to a given DATA . Called by tree function insert_quad() */
/* and by data function quad_divide_data() */
{
    int n, i, cond;
    double xx, yy, r;

    cond = 1;
    if (data == NULL) {
	fprintf(stderr, "add_data: data is NULL \n");
	return -5;
    }
    for (i = 0; i < data->n_points; i++) {
	xx = data->points[i].x - point->x;
	yy = data->points[i].y - point->y;
	r = xx * xx + yy * yy;
	if (r <= dmin) {
	    cond = 0;
	    break;
	}
    }

    if (cond) {
	n = (data->n_points)++;
	data->points[n].x = point->x;
	data->points[n].y = point->y;
	data->points[n].z = point->z;
	data->points[n].sm = point->sm;
    }
    return cond;
}




int quad_intersect(struct quaddata *data_inter, struct quaddata *data)
/* Checks if region defined by DATA intersects the region defined
   by data_inter. Called by tree function MT_region_data() */
{
    double xmin, xmax, ymin, ymax;

    xmin = data_inter->x_orig;
    xmax = data_inter->xmax;
    ymin = data_inter->y_orig;
    ymax = data_inter->ymax;

    if (((data->x_orig >= xmin) && (data->x_orig <= xmax)
	 && (((data->y_orig >= ymin) && (data->y_orig <= ymax))
	     || ((ymin >= data->y_orig) && (ymin <= data->ymax))
	 )
	)
	|| ((xmin >= data->x_orig) && (xmin <= data->xmax)
	    && (((ymin >= data->y_orig) && (ymin <= data->ymax))
		|| ((data->y_orig >= ymin) && (data->y_orig <= ymax))
	    )
	)
	) {
	return 1;
    }
    else
	return 0;
}




int quad_division_check(struct quaddata *data, int kmax)
/* Checks if DATA needs to be divided. If data->points is empty,
   returns -1; if its not empty but there aren't enough points
   in DATA for division returns 0. Othervise (if its not empty and
   there are too many points) returns 1. Called by MT_insert() */
{
    if (data->points == NULL)
	return -1;
    if (data->n_points < kmax)
	return 0;
    else
	return 1;
}



struct quaddata **quad_divide_data(struct quaddata *data, int kmax,
				   double dmin)
/* Divides DATA into 4 new datas reinserting data->points in
   them by calling data function quad_compare() to detrmine
   were to insert. Called by MT_divide(). Returns array of 4 new datas */
{
    struct quaddata **datas;
    int cols1, cols2, rows1, rows2, i;	/*j1, j2, jmin = 0; */
    double dx, dy;		/* x2, y2, dist, mindist; */
    double xr, xm, xl, yr, ym, yl;	/* left, right, middle coord */
    double ew_res, ns_res;

    ew_res = (data->xmax - data->x_orig) / data->n_cols;
    ns_res = (data->ymax - data->y_orig) / data->n_rows;

    if ((data->n_cols <= 1) || (data->n_rows <= 1)) {
	fprintf(stderr,
		"Points are too concentrated -- please increase DMIN\n");
	exit(0);
    }

    if (data->n_cols % 2 == 0) {
	cols1 = data->n_cols / 2;
	cols2 = cols1;
    }
    else {
	cols2 = (int)(data->n_cols / 2);
	cols1 = cols2 + 1;
    }
    if (data->n_rows % 2 == 0) {
	rows1 = data->n_rows / 2;
	rows2 = rows1;
    }
    else {
	rows2 = (int)(data->n_rows / 2);
	rows1 = rows2 + 1;
    }

    dx = cols1 * ew_res;
    dy = rows1 * ns_res;

    xl = data->x_orig;
    xm = xl + dx;
    xr = data->xmax;
    yl = data->y_orig;
    ym = yl + dy;
    yr = data->ymax;

    if (!(datas = (struct quaddata **)malloc(sizeof(struct quaddata *) * 5))) {
	return NULL;
    }
    datas[NE] = quad_data_new(xm, ym, xr, yr, rows2, cols2, 0, kmax);
    datas[SW] = quad_data_new(xl, yl, xm, ym, rows1, cols1, 0, kmax);
    datas[SE] = quad_data_new(xm, yl, xr, ym, rows1, cols2, 0, kmax);
    datas[NW] = quad_data_new(xl, ym, xm, yr, rows2, cols1, 0, kmax);
    for (i = 0; i < data->n_points; i++) {
	switch (quad_compare(data->points + i, data)) {
	case SW:
	    {
		quad_add_data(data->points + i, datas[SW], dmin);
		break;
	    }
	case SE:
	    {
		quad_add_data(data->points + i, datas[SE], dmin);
		break;
	    }
	case NW:
	    {
		quad_add_data(data->points + i, datas[NW], dmin);
		break;
	    }
	case NE:
	    {
		quad_add_data(data->points + i, datas[NE], dmin);
		break;
	    }
	}
    }
    data->points = NULL;
    return datas;
}




int
quad_get_points(struct quaddata *data_inter, struct quaddata *data, int MAX)
/* Gets such points from DATA that lie within region determined by
   data_inter. Called by tree function region_data(). */
{
    int i, ind;
    int n = 0;
    int l = 0;
    double xmin, xmax, ymin, ymax;
    struct triple *point;

    xmin = data_inter->x_orig;
    xmax = data_inter->xmax;
    ymin = data_inter->y_orig;
    ymax = data_inter->ymax;
    for (i = 0; i < data->n_points; i++) {
	point = data->points + i;
	if (l >= MAX)
	    return MAX + 1;
	if ((point->x > xmin) && (point->x < xmax)
	    && (point->y > ymin) && (point->y < ymax)) {
	    ind = data_inter->n_points++;
	    data_inter->points[ind].x = point->x;
	    data_inter->points[ind].y = point->y;
	    data_inter->points[ind].z = point->z;
	    data_inter->points[ind].sm = point->sm;
	    l = l + 1;

	}
    }
    n = l;
    return (n);
}
