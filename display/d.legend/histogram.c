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
		    int height, int color, int flip, int horiz)
{
    int i, nsteps;
    long cell_count;
    double max_width, width_mult, dx;
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
    get_stats(map_name, &dist_stats, nsteps);

    width_mult = max_width / dist_stats.maxstat;

    D_use_color(color);
    D_begin();

    ptr = dist_stats.ptr;
    for (i = dist_stats.mincat; i <= dist_stats.maxcat; i++) {
	if (!ptr)
	    break;

	if (ptr->cat == i) {       /* AH-HA!! found the stat */
            cell_count = ptr->stat;

            if (ptr->next != NULL)
                ptr = ptr->next;
        }
        else {                  /* we have to look for the stat */

            /* loop until we find it, or pass where it should be */
            while (ptr->cat < i && ptr->next != NULL)
                ptr = ptr->next;
            if (ptr->cat == i) {        /* AH-HA!! found the stat */
                cell_count = ptr->stat;

                if (ptr->next != NULL)
                    ptr = ptr->next;
            }
            else                /* stat cannot be found */
		G_debug(4, "No matching stat found, i=%d", i);
        }

	if (!cell_count)
	    continue;

	dx = cell_count * width_mult;

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

    D_close();
    D_end();
    D_stroke();
}
