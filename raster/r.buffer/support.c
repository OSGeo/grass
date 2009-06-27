
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

#include <grass/raster.h>
#include "distance.h"


int make_support_files(char *output, char *units)
{
    struct Categories pcats;

    int i;
    CELL cat;
    char label[128];

    Rast_init_cats("Distance Zones", &pcats);

    cat = 1;
    Rast_set_c_cat(&cat, &cat, "distances calculated from these locations", &pcats);
    for (i = 0; i < ndist; i++) {
	if (i == 0)
	    sprintf(label, "0-%s %s", distances[i].label, units);
	else {
	    /* improved next, but it would be perfect to achieve (example):
	     * 0-100.55 meters
	     * 100.56-233.33 meters
	     *
	     *now we get:
	     * 0-100.55 meters
	     * 100.55-233.33 meters
	     *
	     *but it's better that the original code. MN 1/2002
	     */
	    sprintf(label, "%s-%s %s", distances[i - 1].label,
		    distances[i].label, units);
	}

	cat = i + ZONE_INCR;
	Rast_set_c_cat(&cat, &cat, label, &pcats);
    }

    Rast_write_cats(output, &pcats);
    Rast_free_cats(&pcats);

    return 0;
}
