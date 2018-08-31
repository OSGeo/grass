
/****************************************************************************
 *
 * MODULE:       r.carve
 *
 * AUTHOR(S):    Original author Bill Brown, UIUC GIS Laboratory
 *               Brad Douglas <rez touchofmadness com>
 *
 * PURPOSE:      Takes vector stream data, converts it to 3D raster and
 *               subtracts a specified depth
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
****************************************************************************/

/* 
 * Routines to create a line of best fit when given a set of coord pairs.
 */

#include "enforce.h"
#include <grass/glocale.h>


/*
 * pg_init - initialize PointGrp variables
 */
void pg_init(PointGrp * pg)
{
    pg->sum_x = pg->sum_y = pg->sum_xy = pg->sum_x_sq = 0.0;
    pg->npts = 0;
}


/*
 * pg_y_from_x - determine y value for a given x value in a PointGrp
 */
double pg_y_from_x(PointGrp * pg, const double x)
{
    return ((pg->slope * x) + pg->yinter);
}


/*
 * pg_addpt - Add a point to a PointGrp
 */
void pg_addpt(PointGrp * pg, Point2 pt)
{
    if (pg->npts < MAX_PTS - 1) {
	double x = pt[0];
	double y = pt[1];

	/* add point to group */
	pg->pnts[pg->npts][0] = x;
	pg->pnts[pg->npts][1] = y;
	pg->sum_x += x;
	pg->sum_y += y;
	pg->sum_xy += (x * y);
	pg->sum_x_sq += SQR(x);
	++pg->npts;
    }

    if (pg->npts > 1) {
	double denom;

	/* solve for x and y using Cramer's Rule */

	/* check for divide by zero */
	if (0 ==
	    (denom = DET2_2(pg->sum_x_sq, pg->sum_x, pg->sum_x, pg->npts))) {
	    G_warning(_("trying to divide by zero...no unique solution for "
		       "system...skipping..."));
	    pg->slope = pg->yinter = 0.0;
	}
	else {
	    pg->slope = DET2_2(pg->sum_xy, pg->sum_x, pg->sum_y, pg->npts) /
		denom;
	    pg->yinter =
		DET2_2(pg->sum_x_sq, pg->sum_xy, pg->sum_x,
		       pg->sum_y) / denom;
	}
    }
}


/*
 * pg_getpoints - returns the Point2 structure from a PointGrp
 */
Point2 *pg_getpoints(PointGrp * pg)
{
    return pg->pnts;
}


/*
 * pg_getpoints_reversed - reverse points in PointGrp and returns Point2
 */
Point2 *pg_getpoints_reversed(PointGrp * pg)
{
    int i;
    int iter = pg->npts / 2;
    double x, y;

    for (i = 0; i < iter; i++) {
	/* swap points */
	x = pg->pnts[i][0];
	y = pg->pnts[i][1];
	pg->pnts[i][0] = pg->pnts[pg->npts - i - 1][0];
	pg->pnts[i][1] = pg->pnts[pg->npts - i - 1][1];
	pg->pnts[pg->npts - i - 1][0] = x;
	pg->pnts[pg->npts - i - 1][1] = y;
    }

    return pg->pnts;
}
