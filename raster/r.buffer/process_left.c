
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


int process_left(int from_row, int to_row, int start_col, int first_zone)
{
    register int i, col, cur_zone;
    register MAPTYPE *to_ptr, *from_ptr;
    register int ncols, incr, farthest;


    /* find cells to the left
     * stop at left edge, or when ncols is bigger than the last zone,
     * or when we see a 1 in the map
     */

    col = start_col;
    from_ptr = map + MAPINDEX(from_row, col);
    to_ptr = map + MAPINDEX(to_row, col);
    farthest = distances[ndist - 1].ncols;


    /* planimetric grids will look for ncols^2
     * and can use fact that (n+1)^2 = n^2 + 2n + 1
     */

    if (window.proj != PROJECTION_LL)
	incr = 1;
    else
	incr = 0;

    ncols = 0;
    while (1) {
	if (col == 0) {		/* global wrap-around */
	    if (!wrap_ncols)
		break;		/* only can happen with lat-lon */
	    col = window.cols;
	    ncols += wrap_ncols - 1;
	    from_ptr = map + MAPINDEX(from_row, col);
	    to_ptr = map + MAPINDEX(to_row, col);
	}
	col--;

	if (incr) {
	    ncols += incr;
	    incr += 2;
	}
	else
	    ncols++;
	if (ncols > farthest)
	    break;

	if (*--from_ptr == 1)
	    break;

	/* convert 1,2,3,4 to -1,0,1,2 etc. 0 becomes ndist */

	if ((cur_zone = *--to_ptr))
	    cur_zone -= ZONE_INCR;
	else
	    cur_zone = ndist;

	/* find the first zone that is closer than the current value */

	for (i = first_zone; i < cur_zone; i++) {
	    if (distances[i].ncols >= ncols) {
		*to_ptr = (first_zone = i) + ZONE_INCR;
		break;
	    }
	}
    }

    return 0;
}
