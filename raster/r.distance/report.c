
/****************************************************************************
 *
 * MODULE:       r.distance
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Locates the closest points between objects in two
 *               raster maps.
 *
 * COPYRIGHT:    (C) 2003 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <grass/glocale.h>

#include "defs.h"

void report(struct Parms *parms)
{
    int i1, i2;
    struct Map *map1, *map2;
    char *fs;
    double distance, north1, east1, north2, east2;
    struct Cell_head region;
    struct CatEdgeList *list1, *list2;
    char temp[100];

    extern void find_minimum_distance();
    extern char *get_label();

    G_get_set_window(&region);
    G_begin_distance_calculations();

    map1 = &parms->map1;
    map2 = &parms->map2;
    fs = parms->fs;

    G_message(_("Processing..."));

    for (i1 = 0; i1 < map1->edges.ncats; i1++) {
	list1 = &map1->edges.catlist[i1];
	for (i2 = 0; i2 < map2->edges.ncats; i2++) {
	    list2 = &map2->edges.catlist[i2];
	    find_minimum_distance(list1, list2,
				  &east1, &north1, &east2, &north2, &distance,
				  &region, parms->overlap, map1->name,
				  map2->name);

	    /* print cat numbers */
	    fprintf(stdout, "%ld%s%ld", (long)list1->cat, fs,
		    (long)list2->cat);

	    /* print distance */
	    sprintf(temp, "%.10f", distance);
	    G_trim_decimal(temp);
	    fprintf(stdout, "%s%s", fs, temp);

	    /* print coordinates of the closest pair */
	    G_format_easting(east1, temp,
			     G_projection() == PROJECTION_LL ? -1 : 0);
	    fprintf(stdout, "%s%s", fs, temp);
	    G_format_northing(north1, temp,
			      G_projection() == PROJECTION_LL ? -1 : 0);
	    fprintf(stdout, "%s%s", fs, temp);
	    G_format_easting(east2, temp,
			     G_projection() == PROJECTION_LL ? -1 : 0);
	    fprintf(stdout, "%s%s", fs, temp);
	    G_format_northing(north2, temp,
			      G_projection() == PROJECTION_LL ? -1 : 0);
	    fprintf(stdout, "%s%s", fs, temp);

	    /* print category labels */
	    if (parms->labels) {
		fprintf(stdout, "%s%s", fs, get_label(map1, list1->cat));
		fprintf(stdout, "%s%s", fs, get_label(map2, list2->cat));
	    }
	    fprintf(stdout, "\n");
	}
    }
}
