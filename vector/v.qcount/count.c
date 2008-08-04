/*
 * Copyright (C) 1994-1995. James Darrell McCauley. (darrell@mccauley-usa.com)
 *                                http://mccauley-usa.com/
 *
 * This program is free software under the GPL (>=v2)
 * Read the file GPL.TXT coming with GRASS for details.
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include "quaddefs.h"

void count_sites(COOR * quads, int nquads, int *counts, double radius,
		 struct Map_info *Map)
/*
 * counts the number of sites in the Map that fall within nquads quads of a certain radius
 */
{

    int i, line, nlines;
    struct line_pnts *Points;

    Points = Vect_new_line_struct();
    nlines = Vect_get_num_lines(Map);

    for (line = 1; line <= nlines; line++) {
	int type;

	type = Vect_read_line(Map, Points, NULL, line);
	if (!(type & GV_POINT))
	    continue;

	for (i = 0; i < nquads; ++i) {
	    if (hypot(Points->x[0] - quads[i].x, Points->y[0] - quads[i].y) <=
		radius) {
		counts[i]++;
		break;		/* next point, quads don't overlap */
	    }
	}
    }
}
