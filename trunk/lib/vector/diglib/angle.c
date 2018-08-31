/*
 ****************************************************************************
 *
 * MODULE:       Vector library 
 *              
 * AUTHOR(S):    Original author CERL, probably Dave Gerdes.
 *               Update to GRASS 5.7 Radim Blazek.
 *
 * PURPOSE:      Lower level functions for reading/writing/manipulating vectors.
 *
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/
/*
 *    functions - calc_begin_angle(), and calc_end_angle()  
 *    used to calculate the angle of a line to a node.
 *    returns -  (float)angle (-PI ... +PI)
 *    returns -  (float)(-9)  if only 1 point or more points but identical
 */

#include <stdio.h>
#include <math.h>
#include <grass/vector.h>

static double d_atan2(double, double);

float dig_calc_begin_angle(const struct line_pnts *points, double thresh)
{
    double last_x;
    double last_y;
    const double *xptr;
    const double *yptr;
    int short_line;
    int i;
    int n_points;
    const double *xarray;
    const double *yarray;

    /* temporary way to set up use of struct line_pnts */
    n_points = points->n_points;
    xarray = points->x;
    yarray = points->y;

    last_x = *xarray;
    last_y = *yarray;
    xptr = xarray + 1;
    yptr = yarray + 1;

    /* check degenerate line */
    if (dig_line_degenerate(points) > 0)
	return ((float)-9.);

    short_line = 1;
    if (n_points != 2) {
	/* Search for next different coord. Note that in >= g5.7, threshold
	 * is not used for build process. */
	/* 4.1 but do not use opposite node if there are other points */
	for (i = 1; i < n_points - 1; i++) {
	    if ((thresh < fabs(*xptr - last_x)) ||
		(thresh < fabs(*yptr - last_y))) {
		short_line = 0;
		break;
	    }
	    xptr++;
	    yptr++;
	}
    }

    if (short_line) {
	/* for 4.1 change this to take 1st point after node  -dpg 12/92 */
	/* return ((float) d_atan2 (yarray[n_points - 1] - last_y, xarray[n_points - 1] - last_x)); */
	return ((float)d_atan2(yarray[1] - last_y, xarray[1] - last_x));
    }

    return ((float)d_atan2(*yptr - last_y, *xptr - last_x));
}				/*  calc_begin_angle()  */

float dig_calc_end_angle(const struct line_pnts *points, double thresh)
{
    double last_x;
    double last_y;
    const double *xptr;
    const double *yptr;
    int short_line;
    int i;
    int n_points;
    const double *xarray;
    const double *yarray;

    short_line = 1;

    xarray = points->x;
    yarray = points->y;
    n_points = points->n_points;

    /* check degenerate line */
    if (dig_line_degenerate(points) > 0)
	return ((float)-9.);

    last_x = *(xarray + n_points - 1);
    last_y = *(yarray + n_points - 1);
    xptr = xarray + n_points - 2;
    yptr = yarray + n_points - 2;

    if (n_points != 2) {
	/* Search for next different coord. Note that in >= g5.7, threshold
	 * is not used for build process. */
	/* 4.1 but do not use opposite node if there are other points */
	for (i = n_points - 2; i > 0; i--) {
	    if ((thresh < fabs(*xptr - last_x)) ||
		(thresh < fabs(*yptr - last_y))) {
		short_line = 0;
		break;
	    }
	    xptr--;
	    yptr--;
	}
    }

    if (short_line) {
	/* updated for 4.1 to take next point away from node  -dpg */
	/* return ((float) d_atan2 (yarray[0] - last_y, xarray[0] - last_x)); */
	return ((float)
		d_atan2(yarray[n_points - 2] - last_y,
			xarray[n_points - 2] - last_x));
    }

    return ((float)d_atan2(*yptr - last_y, *xptr - last_x));
}

int dig_is_line_degenerate(const struct line_pnts *points, double thresh)
{
    double last_x;
    double last_y;
    const double *xptr;
    const double *yptr;
    int short_line;
    int i;
    int n_points;
    const double *xarray;
    const double *yarray;

    /* temporary way to set up use of struct line_pnts */
    n_points = points->n_points;
    xarray = points->x;
    yarray = points->y;

    last_x = *xarray;
    last_y = *yarray;
    xptr = xarray + 1;
    yptr = yarray + 1;

    short_line = 1;
    for (i = 1; i < n_points; i++) {	/* Search for next different coord */
	if ((thresh < fabs(*xptr - last_x)) ||
	    (thresh < fabs(*yptr - last_y))) {
	    short_line = 0;
	    break;
	}
	xptr++;
	yptr++;
    }

    if (short_line)
	return (1);

    return (0);

}

/* Check if line is degenerate (one point or more identical points)
 *  Returns: 0 is not degenerate (but som points may be identical)
 *           1 one point
 *           2 more identical points
 */
int dig_line_degenerate(const struct line_pnts *points)
{
    int i, ident;
    int n_points;

    G_debug(5, "dig_line_degenerate()");
    /* temporary way to set up use of struct line_pnts */
    n_points = points->n_points;

    if (n_points == 1) {
	G_debug(5, "  Line is degenerate (one points)");
	return 1;
    }

    /* check identical points (= one point) */
    ident = 1;
    for (i = 1; i < n_points; i++) {
	if (points->x[i] != points->x[i - 1] ||
	    points->y[i] != points->y[i - 1]) {
	    ident = 0;
	    break;
	}
    }

    if (ident) {
	G_debug(5, "  Line is degenerate (more points)");
	return 2;
    }

    return 0;
}

static double d_atan2(double y, double x)
{
    if (y == 0.0 && x == 0.0)
	return (0.0);
    else
	return (atan2(y, x));
}
