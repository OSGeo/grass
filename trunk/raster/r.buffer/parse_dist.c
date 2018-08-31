
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

#include <stdlib.h>
#include "distance.h"
#include <grass/glocale.h>

static int cmp(const void *, const void *);
static int scan_dist(char *, double *);


int parse_distances(char **zone_list, double to_meters)
{
    double dist;
    double ew2 = 0.0;
    int i;
    int count;

    for (count = 0; zone_list[count]; count++) ;
    if (count <= 0)
	return 0;

    /* create an array to hold the distances */

    distances = (struct Distance *)G_calloc(count, sizeof(struct Distance));

    /* scan the command line for the distances */

    if (window.proj != PROJECTION_LL) {
	ew2 = window.ew_res * window.ew_res;
	ns_to_ew_squared = window.ns_res * window.ns_res / ew2;
    }

    for (i = 0; i < count; i++) {
	if (!scan_dist(zone_list[i], &dist)) {
	    G_warning(_("%s: %s - illegal distance specification"),
		      pgm_name, zone_list[i]);
	    return 0;
	}
	else {
	    dist *= (to_meters * meters_to_grid);
	    if (window.proj != PROJECTION_LL)
		dist = dist * dist / ew2;
	    distances[i].dist = dist;
	}
	distances[i].label = zone_list[i];
    }

    /* sort the distances in increasing order */

    qsort(distances, count, sizeof(struct Distance), cmp);

    return count;
}

static int cmp(const void *aa, const void *bb)
{
    const struct Distance *a = aa, *b = bb;

    if (a->dist < b->dist)
	return -1;
    return a->dist > b->dist;
}

static int scan_dist(char *s, double *dist)
{
    char dummy[2];

    *dummy = 0;
    if (sscanf(s, "%lf%1s", dist, dummy) != 1)
	return 0;
    if (*dummy)
	return 0;
    if (*dist <= 0.0)
	return 0;
    return 1;
}
