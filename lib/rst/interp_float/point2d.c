
/*-
 *
 * Original program and various modifications:
 * Lubos Mitas 
 *
 * GRASS4.1 version of the program and GRASS4.2 modifications:
 * H. Mitasova
 * I. Kosinovsky, D. Gerdes
 * D. McCauley 
 *
 * Copyright 1993, 1995:
 * L. Mitas ,
 * H. Mitasova ,
 * I. Kosinovsky, ,
 * D.Gerdes 
 * D. McCauley
 *
 * modified by McCauley in August 1995
 * modified by Mitasova in August 1995, Nov. 1996
 *
 */


#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/site.h>
#include <grass/vector.h>
#include <grass/dbmi.h>

#define	POINT2D_C
#include <grass/interpf.h>

/* needed for AIX */
#ifdef hz
#undef hz
#endif

int IL_check_at_points_2d(struct interp_params *params, struct quaddata *data,	/* current region */
			  double *b,	/* solution of linear equations */
			  double *ertot,	/* total error */
			  double zmin,	/* min z-value */
			  double dnorm, struct triple skip_point)

/*
 * Checks if interpolating function interp() evaluates correct z-values at
 * given points. If smoothing is used calculate the maximum error caused
 * by smoothing.
 */
{
    int n_points = data->n_points;	/* number of points */
    struct triple *points = data->points;	/* points for interpolation */
    double east = data->xmax;
    double west = data->x_orig;
    double north = data->ymax;
    double south = data->y_orig;
    double rfsta2, errmax, h, xx, yy, r2, hz, zz, err, xmm, ymm, r;
    double skip_err;
    int n1, mm, m, cat;
    double fstar2;
    int inside;

    /*  Site *site; */
    char buf[1024];


    /*  if ((site = G_site_new_struct (-1, 2, 0, 1)) == NULL)
       G_fatal_error ("Memory error for site struct"); */

    fstar2 = params->fi * params->fi / 4.;
    errmax = .0;
    n1 = n_points + 1;
    for (mm = 1; mm <= n_points; mm++) {
	h = b[0];
	for (m = 1; m <= n_points; m++) {
	    xx = points[mm - 1].x - points[m - 1].x;
	    yy = points[mm - 1].y - points[m - 1].y;
	    r2 = yy * yy + xx * xx;
	    if (r2 != 0.) {
		rfsta2 = fstar2 * r2;
		r = r2;
		h = h + b[m] * params->interp(r, params->fi);
	    }
	}
	/* modified by helena january 1997 - normalization of z was
	   removed from segm2d.c and interp2d.c
	   hz = (h * dnorm) + zmin;
	   zz = (points[mm - 1].z * dnorm) + zmin;
	 */
	hz = h + zmin;
	zz = points[mm - 1].z + zmin;
	err = hz - zz;
	xmm = points[mm - 1].x * dnorm + params->x_orig + west;
	ymm = points[mm - 1].y * dnorm + params->y_orig + south;
	if ((xmm >= west + params->x_orig) && (xmm <= east + params->x_orig)
	    && (ymm >= south + params->y_orig) &&
	    (ymm <= north + params->y_orig))
	    inside = 1;
	else
	    inside = 0;

	if (params->fddevi != NULL) {

	    if (inside) {	/* if the point is inside the region */
		Vect_reset_line(Pnts);
		Vect_reset_cats(Cats2);

		Vect_append_point(Pnts, xmm, ymm, zz);
		cat = count;
		Vect_cat_set(Cats2, 1, cat);
		Vect_write_line(&Map2, GV_POINT, Pnts, Cats2);

		db_zero_string(&sql2);
		sprintf(buf, "insert into %s values ( %d ", ff->table, cat);
		db_append_string(&sql2, buf);

		sprintf(buf, ", %f", err);
		db_append_string(&sql2, buf);
		db_append_string(&sql2, ")");
		G_debug(3, "IL_check_at_points_2d: %s", db_get_string(&sql2));

		if (db_execute_immediate(driver2, &sql2) != DB_OK) {
		    db_close_database(driver2);
		    db_shutdown_driver(driver2);
		    G_fatal_error("Cannot insert new row: %s",
				  db_get_string(&sql2));
		}
		count++;

	    }
	}
	(*ertot) += err * err;
    }

    /* cv stuff */
    if (params->cv) {
	h = b[0];
	for (m = 1; m <= n_points - 1; m++) {
	    xx = points[m - 1].x - skip_point.x;
	    yy = points[m - 1].y - skip_point.y;
	    r2 = yy * yy + xx * xx;
	    if (r2 != 0.) {
		rfsta2 = fstar2 * r2;
		r = r2;
		h = h + b[m] * params->interp(r, params->fi);
	    }
	}
	hz = h + zmin;
	zz = skip_point.z + zmin;
	skip_err = hz - zz;
	xmm = skip_point.x * dnorm + params->x_orig + west;
	ymm = skip_point.y * dnorm + params->y_orig + south;

	if ((xmm >= west + params->x_orig) && (xmm <= east + params->x_orig)
	    && (ymm >= south + params->y_orig) &&
	    (ymm <= north + params->y_orig))
	    inside = 1;
	else
	    inside = 0;

	if (inside) {		/* if the point is inside the region */
	    Vect_reset_line(Pnts);
	    Vect_reset_cats(Cats2);

	    Vect_append_point(Pnts, xmm, ymm, zz);
	    cat = count;
	    Vect_cat_set(Cats2, 1, cat);
	    Vect_write_line(&Map2, GV_POINT, Pnts, Cats2);

	    db_zero_string(&sql2);
	    sprintf(buf, "insert into %s values ( %d ", ff->table, cat);
	    db_append_string(&sql2, buf);

	    sprintf(buf, ", %f", skip_err);
	    db_append_string(&sql2, buf);
	    db_append_string(&sql2, ")");
	    G_debug(3, "IL_check_at_points_2d: %s", db_get_string(&sql2));

	    if (db_execute_immediate(driver2, &sql2) != DB_OK) {
		db_close_database(driver2);
		db_shutdown_driver(driver2);
		G_fatal_error("Cannot insert new row: %s",
			      db_get_string(&sql2));
	    }
	    count++;
	}
    }				/* cv */


    return 1;
}
