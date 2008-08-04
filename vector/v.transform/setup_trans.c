/*
 ****************************************************************************
 *
 * MODULE:       v.transform
 * AUTHOR(S):    See other files as well...
 *               Eric G. Miller <egm2@jps.net>
 * PURPOSE:      To transform a vector layer's coordinates via a set of tie
 *               points.
 * COPYRIGHT:    (C) 2002 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/

/*  
 *  Functions in this file:
 *     setup_transform ( n_points)
 *         returns:  ALL_OK, POINTS_NOT_SPREAD, NEED_MORE_POINTS
 *     init_transform_arrays()
 *     print_transform_error(stat)
 *
 *  Written by the GRASS Team, 02/16/90, -mh .
 */

#include	<stdlib.h>
#include	<stdio.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include	"trans.h"

int setup_transform(int n_points)
{

    int status;


    /*  compute_transformation_coef() returns:
     *   -2,  Not enough points
     *   1,   everything is okay
     *   -1,  points weren't spread out enough
     */

    /*  if there are enough points registered compute residuals  */
    if (n_points >= MIN_COOR)
	status = compute_transformation_coef(ax, ay, bx, by, use, MAX_COOR);
    else
	return (NEED_MORE_POINTS);

    if (status != ALL_OK)
	return (POINTS_NOT_SPREAD);

    residuals_a_predicts_b(ax, ay, bx, by, use, MAX_COOR, residuals, &rms);

    return (ALL_OK);


}				/*  setup_transform()   */

int init_transform_arrays(void)
{
    int i;

    /*  initiliaze use[],  no points valid  */
    for (i = 0; i < MAX_COOR; ++i) {
	*(use + i) = 0;
	*(bx + i) = 0;
	*(by + i) = 0;
	*(residuals + i) = 0;
    }

    reg_cnt = 0;

    return 0;
}


int print_transform_error(int stat)
{
    switch (stat) {
    case POINTS_NOT_SPREAD:
	G_fatal_error(_("The points weren't spread out enough."));
	break;
    case NEED_MORE_POINTS:
	G_fatal_error(_("You need to enter at least %d points."), MIN_COOR);
	break;
    default:
	G_fatal_error("Your calling print_transform_error() with no error.");
	break;
    }

    return 0;
}
