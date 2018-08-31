
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

int process_at(int from_row, int to_row, int start_col, int first_zone)
{
    register MAPTYPE *to_ptr, *from_ptr;
    register int col, cur_zone;

    /* find all adjacent 1 cells 
     * stop at last 1 in the from_row
     * return position of last 1
     */

    col = start_col;
    from_ptr = map + MAPINDEX(from_row, col);
    to_ptr = map + MAPINDEX(to_row, col);
    while (col <= maxcol && *from_ptr == 1) {
	if ((cur_zone = *to_ptr))
	    cur_zone -= ZONE_INCR;
	else
	    cur_zone = ndist;

	if (first_zone < cur_zone)
	    *to_ptr = first_zone + ZONE_INCR;

	to_ptr++;
	col++;
	from_ptr++;
    }

    return col - 1;
}
