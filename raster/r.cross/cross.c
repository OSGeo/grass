
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
#include <grass/glocale.h>
#include <grass/raster.h>
#include <grass/btree.h>
#include "glob.h"
#include "local_proto.h"

static int compare(const void *aa, const void *bb)
{
    const CELL *a = aa;
    const CELL *b = bb;
    int i;

    for (i = 0; i < nfiles; i++) {
	if (a[i] > b[i])
	    return 1;
	if (a[i] < b[i])
	    return -1;
    }

    return 0;
}

CELL cross(int fd[], int non_zero, int primary, int outfd)
{
    BTREE btree;
    CELL *cell[NFILES];
    CELL *result_cell;
    CELL cat[NFILES];
    int row, col, i;
    int zero;
    CELL result;
    int keysize = nfiles * sizeof(CELL);
    void *ptr;

    /* allocate i/o buffers for each raster map */

    for (i = 0; i < nfiles; i++)
	cell[i] = Rast_allocate_c_buf();
    result_cell = cell[0];

    /* initialize the reclass table */
    result = 0;

    /* here we go */
    btree_create(&btree, compare, 1);
    G_message(_("%s: STEP 1 ... "), G_program_name());
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 5);

	/* read the primary file first, even if not first in the list */
	Rast_get_c_row(fd[primary], cell[0], row);

	/* read the others */
	col = 1;
	for (i = 0; i < nfiles; i++)
	    if (i != primary)
		Rast_get_c_row(fd[i], cell[col++], row);
	for (col = 0; col < ncols; col++) {
	    zero = 1;
	    for (i = 0; i < nfiles; i++) {
		cat[i] = cell[i][col];
		if (!Rast_is_c_null_value(&cat[i]))
		    zero = 0;
		else if (non_zero) {
		    zero = 1;
		    break;
		}
	    }
	    if (zero) {
		Rast_set_c_null_value(&result_cell[col], 1);
		continue;
	    }

	    /* search for this value in the tree */
	    if (btree_find(&btree, cat, &ptr))
		result_cell[col] = *(CELL*)ptr;
	    else {
		btree_update(&btree, cat, keysize, &result, sizeof(CELL));
		store_reclass(result, primary, cat);
		result_cell[col] = result;
		result++;
	    }
	}
	Rast_put_row(outfd, result_cell, CELL_TYPE);
    }
    G_percent(nrows, nrows, 5);

    /* free some memory */
    btree_free(&btree);
    for (i = 0; i < nfiles; i++)
	G_free(cell[i]);
    return result - 1;
}
