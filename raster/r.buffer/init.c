
/****************************************************************************
 *
 * MODULE:       r.buffer
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      This program creates distance zones from non-zero
 *               cells in a grid layer. Distances are specified in
 *               meters (on the command-line). Window does not have to
 *               have square cells. Works both for planimetric
 *               (UTM, State Plane) and lat-long.
 *
 * COPYRIGHT:    (C) 2005 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
****************************************************************************/

#include "distance.h"
#include <grass/gis.h>

int init_grass(void)
{
    double a, e2;
    double factor;

    G_get_set_window(&window);
    if (window.proj == PROJECTION_LL) {
	G_get_ellipsoid_parameters(&a, &e2);
	G_begin_geodesic_distance(a, e2);
	wrap_ncols =
	    (360.0 - (window.east - window.west)) / window.ew_res + 1.1;
	/* add 1.1 instead of 1 to insure that we round up, not down */
    }
    else {
	wrap_ncols = 0;
	factor = G_database_units_to_meters_factor();
	if (factor <= 0.0)
	    factor = 1.0;
	meters_to_grid = 1.0 / factor;
    }

    return 0;
}
