/*
 * Copyright (C) 1994-1995. James Darrell McCauley. (darrell@mccauley-usa.com)
 *                                http://mccauley-usa.com/
 *
 * This program is free software under the GPL (>=v2)
 * Read the file GPL.TXT coming with GRASS for details.
 */

#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include "quaddefs.h"

#define RANDOM(lo,hi) (G_drand48() * (hi-lo) + lo)

/*
 * returns Z struct filled with centers of n non-overlapping circles of
 * radius r contained completely within window
 */
COOR *find_quadrats(int n, double r, struct Cell_head window)
{
    int i = 1, j, overlapped;
    unsigned k;
    double east, north, e_max, e_min, n_max, n_min;
    COOR *quads = NULL;

    quads = (COOR *) G_malloc(n * sizeof(COOR));
    if (quads == NULL)
	G_fatal_error("cannot allocate memory for quadrats");

    /* FIXME - allow seed to be specified for repeatability */
    G_srand48_auto();

    e_max = window.east - r;
    e_min = window.west + r;
    n_max = window.north - r;
    n_min = window.south + r;

    quads[0].x = RANDOM(e_min, e_max);
    quads[0].y = RANDOM(n_min, n_max);

    while (i < n) {
	k = 0;
	G_percent(i, n, 1);
	overlapped = 1;
	while (overlapped) {
	    east = RANDOM(e_min, e_max);
	    north = RANDOM(n_min, n_max);
	    k++;
	    overlapped = 0;
	    for (j = i; j >= 0; --j) {
		if (hypot(quads[j].x - east, quads[j].y - north) < 2 * r) {
		    overlapped = 1;
		    j = -1;
		}
	    }
	    if (k == n * n)
		G_warning
		    ("Having difficulties fitting that many circles with that radius");
	    if (k == 2 * n * n)
		G_fatal_error
		    ("Maximum number of iterations exceeded\nTry smaller radius or smaller number of quads");
	}
	G_percent(i, n, 1);
	quads[i].x = east;
	quads[i].y = north;
	i++;
    }
    return quads;
}
