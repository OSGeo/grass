
/****************************************************************************
 *
 * MODULE:       r.cross
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Creates a cross product of the category values from
 *               multiple raster map layers.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <stdlib.h>
#include "glob.h"
#include "local_proto.h"
#include <grass/glocale.h>


int renumber(int in, int out)
{
    CELL *cell, *c;
    int row, col;

    cell = G_allocate_cell_buf();

    G_message(_("%s: STEP 3 ... "), G_program_name());
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 5);
	if (G_get_map_row(in, c = cell, row) < 0)
	    exit(1);
	col = ncols;
	while (col-- > 0) {
	    *c = table[*c];
	    c++;
	}
	if (G_put_raster_row(out, cell, CELL_TYPE) < 0)
	    exit(1);
    }
    G_percent(row, nrows, 10);
    G_free(cell);

    return 0;
}
