
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
#include <grass/glocale.h>


int execute_distance(void)
{
    int row, col, nrows;
    MAPTYPE *ptr;

    /* find the first 1 in each row, and process that row */

    G_message(_("Finding buffer zones..."));

    nrows = 0;
    for (row = minrow; row <= maxrow; row++) {
	ptr = map + MAPINDEX(row, mincol);
	for (col = mincol; col <= maxcol; col++) {
	    if (*ptr++ == 1) {
		G_percent(nrows++, count_rows_with_data, 2);
		process_row(row, col);
		break;
	    }
	}
    }

    G_percent(nrows, count_rows_with_data, 2);

    return 0;
}
