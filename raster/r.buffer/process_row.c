
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
#include "local_proto.h"


int process_row(int row, int start_col)
{
    int r, first_zone, col;


    /* go north */

    begin_distance(row);
    for (r = row; r >= 0 && (first_zone = find_distances(r)) >= 0; r--) {
	col = start_col;
	while (col <= maxcol) {
	    process_left(row, r, col, first_zone);
	    col = process_at(row, r, col, first_zone);
	    col = process_right(row, r, col, first_zone);
	}
    }

    /* go south */

    reset_distances();
    for (r = row + 1;
	 r < window.rows && (first_zone = find_distances(r)) >= 0; r++) {
	col = start_col;
	while (col <= maxcol) {
	    process_left(row, r, col, first_zone);
	    col = process_at(row, r, col, first_zone);
	    col = process_right(row, r, col, first_zone);
	}
    }

    return 0;
}
