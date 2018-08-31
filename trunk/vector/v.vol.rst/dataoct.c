/*
 ****************************************************************************
 *
 * MODULE:       v.vol.rst: program for 3D (volume) interpolation and geometry
 *               analysis from scattered point data using regularized spline
 *               with tension
 *
 * AUTHOR(S):    Original program (1989) and various modifications:
 *               Lubos Mitas
 *
 *               GRASS 4.2, GRASS 5.0 version and modifications:
 *               H. Mitasova,  I. Kosinovsky, D. Gerdes, J. Hofierka
 *
 * PURPOSE:      v.vol.rst interpolates the values to 3-dimensional grid from
 *               point data (climatic stations, drill holes etc.) given in a
 *               3D vector point input. Output grid3 file is elev. 
 *               Regularized spline with tension is used for the
 *               interpolation.
 *
 * COPYRIGHT:    (C) 1989, 1993, 2000 L. Mitas,  H. Mitasova,
 *               I. Kosinovsky, D. Gerdes, J. Hofierka
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "dataoct.h"
#include "externs.h"
#include "user.h"


struct quadruple *point_new(double x, double y, double z, double w, double sm)
{
    struct quadruple *point;

    if (!(point = (struct quadruple *)G_malloc(sizeof(struct quadruple)))) {
	return NULL;
    }

    point->x = x;
    point->y = y;
    point->z = z;
    point->w = w;
    point->sm = sm;

    return point;
}




struct octdata *data_new(double x_orig, double y_orig, double z_orig,
			 int n_rows, int n_cols, int n_levs, int n_points)
{
    struct octdata *data;
    int i;

    if (!(data = (struct octdata *)G_malloc(sizeof(struct octdata)))) {
	return NULL;
    }

    data->x_orig = x_orig;
    data->y_orig = y_orig;
    data->z_orig = z_orig;
    data->n_rows = n_rows;
    data->n_cols = n_cols;
    data->n_levs = n_levs;
    data->n_points = n_points;
    data->points =
	(struct quadruple *)G_malloc(sizeof(struct quadruple) * (KMAX + 1));
    for (i = 0; i <= KMAX; i++) {
	data->points[i].x = 0;
	data->points[i].y = 0;
	data->points[i].z = 0;
	data->points[i].w = 0;
	data->points[i].sm = 0;
    }

    return data;
}




int oct_compare(struct quadruple *point, struct octdata *data)
/* returns the quadrant the point should be inserted in */
/* called by divide() */
{
    int cond1, cond2, cond3, cond4, cond5, cond6, rows, cols, levs;

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
    if (data->n_levs % 2 == 0) {
	levs = data->n_levs / 2;
    }
    else {
	levs = (int)(data->n_levs / 2) + 1;
    }
    cond1 = (point->x >= data->x_orig);
    cond2 = (point->x >= data->x_orig + cols * ew_res);
    cond3 = (point->y >= data->y_orig);
    cond4 = (point->y >= data->y_orig + rows * ns_res);
    cond5 = (point->z >= data->z_orig);
    cond6 = (point->z >= data->z_orig + levs * tb_res);
    if (cond1 && cond3 && cond5) {
	if (cond6) {
	    if (cond2 && cond4)
		return NET;
	    if (cond2)
		return SET;
	    if (cond4)
		return NWT;
	    return SWT;
	}
	else {
	    if (cond2 && cond4)
		return NEB;
	    if (cond2)
		return SEB;
	    if (cond4)
		return NWB;
	    return SWB;
	}
    }
    else
	return 0;
}


int oct_add_data(struct quadruple *point, struct octdata *data)
{
    int n, i, cond;
    double xx, yy, zz, r;

    cond = 1;
    for (i = 0; i < data->n_points; i++) {
	xx = data->points[i].x - point->x;
	yy = data->points[i].y - point->y;
	zz = data->points[i].z - point->z;
	r = xx * xx + yy * yy + zz * zz;
	if (r <= dmin) {
	    cond = 0;
	}
    }

    if (cond) {
	n = (data->n_points)++;
	data->points[n].x = point->x;
	data->points[n].y = point->y;
	data->points[n].z = point->z;
	data->points[n].w = point->w;
	data->points[n].sm = point->sm;
    }
    return cond;
}


int oct_division_check(struct octdata *data)
{
    if (data->points == NULL)
	return -1;
    if (data->n_points < KMAX)
	return 0;
    else
	return 1;
}


struct octdata **oct_divide_data(struct octdata *data)
{
    struct octdata **datas;
    int cols1, cols2, rows1, rows2, levs1, levs2;
    int comp, i;
    double dx, dy, dz, x_or, y_or, z_or;

    if ((data->n_cols <= 1) || (data->n_rows <= 1)) {
	clean();
	G_fatal_error(_("Points are too concentrated -- please increase DMIN"));
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
    if (data->n_levs % 2 == 0) {
	levs1 = data->n_levs / 2;
	levs2 = levs1;
    }
    else {
	levs2 = (int)(data->n_levs / 2);
	levs1 = levs2 + 1;
    }
    dx = cols1 * ew_res;
    dy = rows1 * ns_res;
    dz = levs1 * tb_res;
    x_or = data->x_orig;
    y_or = data->y_orig;
    z_or = data->z_orig;
    if (!(datas = (struct octdata **)G_malloc(sizeof(struct octdata *) * 9))) {
	return NULL;
    }
    datas[SWB] = data_new(x_or, y_or, z_or, rows1, cols1, levs1, 0);
    datas[SEB] = data_new(x_or + dx, y_or, z_or, rows1, cols2, levs1, 0);
    datas[NWB] = data_new(x_or, y_or + dy, z_or, rows2, cols1, levs1, 0);
    datas[NEB] = data_new(x_or + dx, y_or + dy, z_or, rows2, cols2, levs1, 0);
    datas[SWT] = data_new(x_or, y_or, z_or + dz, rows1, cols1, levs2, 0);
    datas[SET] = data_new(x_or + dx, y_or, z_or + dz, rows1, cols2, levs2, 0);
    datas[NWT] = data_new(x_or, y_or + dy, z_or + dz, rows2, cols1, levs2, 0);
    datas[NET] =
	data_new(x_or + dx, y_or + dy, z_or + dz, rows2, cols2, levs2, 0);
    for (i = 0; i < data->n_points; i++) {
	comp = oct_compare(data->points + i, data);
	if ((comp < 1) || (comp > NUMLEAFS)) {
	    clean();
	    G_fatal_error(_("Point out of range"));
	}
	oct_add_data(data->points + i, datas[comp]);
    }
    data->n_points = 0;
    data->points = NULL;
    return datas;
}






int oct_intersect(double xmin, double xmax, double ymin, double ymax,
		  double zmin, double zmax, struct octdata *data)
{
    int ix, ix0, iy, iy0, iz, iz0, izor, iyzoror;

    ix0 = ((data->x_orig >= xmin) && (data->x_orig <= xmax));
    iy0 = ((data->y_orig >= ymin) && (data->y_orig <= ymax));
    iz0 = ((data->z_orig >= zmin) && (data->z_orig <= zmax));
    iz = ((zmin >= data->z_orig) &&
	  (zmin <= data->z_orig + data->n_levs * tb_res));
    iy = ((ymin >= data->y_orig) &&
	  (ymin <= data->y_orig + data->n_rows * ns_res));
    ix = ((xmin >= data->x_orig) &&
	  (xmin <= data->x_orig + data->n_cols * ew_res));

    /*printf("\n%f %f %f",zmin,data->z_orig,data->z_orig+data->n_levs*tb_res); */
    izor = (iz || iz0);
    iyzoror = ((iy0 && izor) || (iy && izor));

    if ((ix0 && iyzoror) || (ix && iyzoror))
    {
	return 1;
    }
    else
	return 0;
}






int oct_get_points(struct quadruple *points, struct octdata *data,
		   double xmin, double xmax, double ymin, double ymax,
		   double zmin, double zmax, int MAX)
{
    int i;
    int n = 0;
    int l = 0;

    struct quadruple *point;

    for (i = 0; i < data->n_points; i++) {
	point = data->points + i;
	if (l >= MAX)
	    return MAX + 1;
	if ((point->x >= xmin) && (point->x <= xmax)
	    && (point->y >= ymin) && (point->y <= ymax)
	    && (point->z >= zmin) && (point->z <= zmax)) {
	    points[l].x = point->x;
	    points[l].y = point->y;
	    points[l].z = point->z;
	    points[l].w = point->w;
	    points[l].sm = point->sm;
	    l++;
	}
    }
    n = l;
    return n;
}
