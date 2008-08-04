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


#include <stdio.h>
#include <grass/gis.h>
#include "trans.h"
#include <grass/glocale.h>

int print_transform_resids(int n_points)
{
    int i;

    fprintf(stdout, "\n");
    fprintf(stdout, "                CHECK MAP RESIDUALS\n");
    fprintf(stdout, "                Current Map                  New Map\n");
    fprintf(stdout,
	    " POINT      X coord    Y coord  |        X coord   Y coord    |      residuals\n");
    fprintf(stdout, "\n");

    for (i = 0; i < n_points; i++) {

	if (use[i])
	    fprintf(stdout,
		    " %2d.  %12.2f %12.2f | %12.2f   %12.2f | %12.2f\n",
		    i + 1, ax[i], ay[i], bx[i], by[i], residuals[i]);

    }

    fprintf(stdout, "\n\n  Number of points: %d\n", n_points);
    fprintf(stdout, "  Residual mean average: %f\n", rms);
    fprintf(stdout, "\n");

    return (0);

}				/*  print_transform_resid()  */
