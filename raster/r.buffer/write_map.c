
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
#include <grass/raster.h>
#include <grass/glocale.h>


    /* write out result */

int write_output_map(char *output, int offset)
{
    int fd_in = 0, fd_out;
    int row;
    register int col;
    register CELL *cell;
    register MAPTYPE *ptr;
    int k;

    fd_out = Rast_open_c_new(output);

    if (offset)
	fd_in = Rast_open_old(output, G_mapset());

    cell = Rast_allocate_c_buf();
    G_message(_("Writing output raster map <%s>..."), output);

    ptr = map;

    for (row = 0; row < window.rows; row++) {
	G_percent(row, window.rows, 2);
	col = window.cols;
	if (!offset) {
	    while (col-- > 0)
		*cell++ = (CELL) * ptr++;
	}
	else {
	    Rast_get_c_row_nomask(fd_in, cell, row);

	    while (col-- > 0) {
		if (*cell == 0 && *ptr != 0)
		    *cell = (CELL) * ptr + offset;
		cell++;
		ptr++;
	    }
	}
	cell -= window.cols;
	/* set 0 to NULL */
	for (k = 0; k < window.cols; k++)
	    if (cell[k] == 0)
		Rast_set_null_value(&cell[k], 1, CELL_TYPE);

	Rast_put_row(fd_out, cell, CELL_TYPE);
    }

    G_percent(row, window.rows, 2);
    G_free(cell);

    if (offset)
	Rast_close(fd_in);

    Rast_close(fd_out);

    return 0;
}
