/* histogram.c:
 *    Draws a histogram along the left side of a smooth gradient legend
 *    (stats fetching code adapted from d.histogram)
 *
 *    Copyright (C) 2014 by Hamish Bowman, and the GRASS Development Team* 
 *    This program is free software under the GPL (>=v2)
 *    Read the COPYING file that comes with GRASS for details.
 */

#include <grass/gis.h>
#include <grass/display.h>
#include "local_proto.h"

void draw_histogram(const char *map_name, int x0, int y0, int width,
		    int height, int color, int flip, int horiz,
		    int map_type, int is_fp)
{
    int i, nsteps;
    long cell_count = 0;
    double max_width, width_mult, dx;
    double dy, y0_adjust;	/* only needed for CELL maps */
    struct stat_list dist_stats;
    struct stat_node *ptr;

    if (horiz) {
	max_width = height * 1.75;
	nsteps = width - 3;
    }
    else {
	max_width = width * 1.75;
	nsteps = height - 3;
    }

    /* get the distribution statistics */
    get_stats(map_name, &dist_stats, nsteps, map_type);

    width_mult = max_width / dist_stats.maxstat;

    D_use_color(color);
    D_begin();

    ptr = dist_stats.ptr;

    if (!is_fp) {
	dy = (nsteps + 3.0) / (1 + dist_stats.maxcat - dist_stats.mincat);

	if (flip)
	    dy *= -1;

	if (dist_stats.mincat == 0)
	    y0_adjust = dy;
	else
	    y0_adjust = 0;

	if (!flip)  /* mmph */
	    y0_adjust += 0.5;
    }

    for (i = dist_stats.mincat; i <= dist_stats.maxcat; i++) {
	if (!ptr)
	    break;

	if (ptr->cat == i) {	/* AH-HA!! found the stat */
	    cell_count = ptr->stat;

	    if (ptr->next != NULL)
		ptr = ptr->next;
	}
	else {			/* we have to look for the stat */

	    /* loop until we find it, or pass where it should be */
	    while (ptr->cat < i && ptr->next != NULL)
		ptr = ptr->next;
	    if (ptr->cat == i) {	/* AH-HA!! found the stat */
		cell_count = ptr->stat;

		if (ptr->next != NULL)
		    ptr = ptr->next;
	    }
	    else		/* stat cannot be found */
		G_debug(4, "No matching stat found, i=%d", i);
	}

	if (!cell_count)
	    continue;

	dx = cell_count * width_mult;

	if (is_fp) {
	    if (horiz) {
		if (flip)
		    D_move_abs(x0 + width - i - 1, y0 - 1);
		else
		    D_move_abs(x0 + i + 1, y0 - 1);

		D_cont_rel(0, -dx);
	    }
	    else {  /* vertical */
		if (flip)
		    D_move_abs(x0 - 1, y0 - 1 + height - i);
		else
		    D_move_abs(x0 - 1, y0 + 1 + i);

		D_cont_rel(-dx, 0);
	    }
	}
	else {	/* categorical */

	    if (horiz) {
		if (flip)
		    D_box_abs(x0 + width + y0_adjust + ((i - 1) * dy),
			      y0 - 1,
			      x0 + width + y0_adjust + 1 + (i * dy),
			      y0 - 1 - dx);
		else
		    D_box_abs(x0 + y0_adjust + ((i - 1) * dy),
			      y0 - 1,
			      x0 - 1 + y0_adjust + (i * dy),
			      y0 - 1 - dx);
	    }
	    else {  /* vertical */

		if (flip)
		    /* GRASS_EPSILON fudge around D_box_abs() weirdness + PNG driver */
		    D_box_abs(x0 - 1 - GRASS_EPSILON * 10,
			      y0 + height + y0_adjust + ((i - 1) * dy),
			      x0 - 1 - dx,
			      y0 + height + y0_adjust + 1 + (i * dy));
		else
		    D_box_abs(x0 - 1 - GRASS_EPSILON * 10,
			      y0 + y0_adjust + ((i - 1) * dy),
			      x0 - 1 - dx,
			      y0 + y0_adjust - 1 + (i * dy));
	    }
	}
    }

    D_close();
    D_end();
    D_stroke();
}
