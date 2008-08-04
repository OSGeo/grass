
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
#include <grass/glocale.h>


    /* read the input map. convert non-nulls to 1 */

int read_input_map(char *input, char *mapset, int ZEROFLAG)
{
    int fd;
    int row;
    int hit;
    register int col;
    register CELL *cell;
    register MAPTYPE *ptr;

    map = (MAPTYPE *) G_malloc(window.rows * window.cols * sizeof(MAPTYPE));

    fd = G_open_cell_old(input, mapset);
    if (fd < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), input);

    cell = G_allocate_cell_buf();

    ptr = map;

    minrow = -1;
    maxrow = -1;
    mincol = window.cols;
    maxcol = 0;

    G_message(_("Reading input raster map <%s>..."),
	      G_fully_qualified_name(input, mapset));

    count_rows_with_data = 0;

    for (row = 0; row < window.rows; row++) {
	hit = 0;
	G_percent(row, window.rows, 2);

	if (G_get_c_raster_row(fd, cell, row) < 0)
	    G_fatal_error(_("Unable to read raster map <%s> row %d"),
			  G_fully_qualified_name(input, mapset), row);

	for (col = 0; col < window.cols; col++) {
	    if (ZEROFLAG) {
		if ((*ptr++ = (*cell++ != 0))) {
		    if (minrow < 0)
			minrow = row;
		    maxrow = row;
		    if (col < mincol)
			mincol = col;
		    if (col > maxcol)
			maxcol = col;
		    if (!hit) {
			count_rows_with_data++;
			hit = 1;
		    }
		}
	    }
	    else {		/* use NULL */

		if ((*ptr++ = !G_is_c_null_value(cell++))) {
		    if (minrow < 0)
			minrow = row;
		    maxrow = row;
		    if (col < mincol)
			mincol = col;
		    if (col > maxcol)
			maxcol = col;
		    if (!hit) {
			count_rows_with_data++;
			hit = 1;
		    }
		}
	    }
	}
	cell -= window.cols;
    }
    G_percent(row, window.rows, 2);
    G_close_cell(fd);
    G_free(cell);

    return 0;
}
