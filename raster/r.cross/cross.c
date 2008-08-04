
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


CELL cross(int fd[], int non_zero, int primary, int outfd)
{
    CELL *cell[NFILES];
    CELL *result_cell, *rp;
    CELL cat[NFILES], cat0, cat1;
    register int i;
    int zero;
    int row, col;
    register int p, q;
    int dir;
    NODE *pnode, *new_node;
    CELL result;

    /* allocate i/o buffers for each raster map */

    for (i = 0; i < nfiles; i++)
	cell[i] = G_allocate_cell_buf();
    result_cell = cell[0];

    /* initialize the reclass table */
    result = 0;
    for (i = 0; i < nfiles; i++)
	cat[i] = 0;
    store_reclass(result, primary, cat);

    /* here we go */
    plant_tree();
    G_message(_("%s: STEP 1 ... "), G_program_name());
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 5);

	/* read the primary file first, even if not first in the list */
	if (G_get_map_row(fd[primary], cell[0], row) < 0)
	    exit(1);

	/* read the others */
	col = 1;
	for (i = 0; i < nfiles; i++)
	    if (i != primary && G_get_map_row(fd[i], cell[col++], row) < 0)
		exit(1);
	for (col = 0; col < ncols; col++) {
	    zero = 1;
	    for (i = 0; i < nfiles; i++) {
		if (cat[i] = cell[i][col])
		    zero = 0;
		else if (non_zero) {
		    zero = 1;
		    break;
		}
	    }
	    if (zero) {
		result_cell[col] = 0;
		continue;
	    }

	    /* search for this value in the tree */
	    cat0 = cat[0];
	    cat1 = cat0 - NCATS;
	    q = 1;
	    while (q > 0) {
		register CELL *t, *c;
		register CELL diff;

		pnode = &tree[p = q];
		t = pnode->cat;

		/* search the tree */
		dir = FOUND;
		if (*t > cat0)
		    dir = LEFT;
		else if (*t++ <= cat1)
		    dir = RIGHT;
		else {
		    c = cat + 1;
		    for (i = 1; i < nfiles; i++)
			if ((diff = (*t++ - *c++)) > 0) {
			    dir = LEFT;
			    break;
			}
			else if (diff < 0) {
			    dir = RIGHT;
			    break;
			}
		}
		switch (dir) {
		case FOUND:
		    rp = &pnode->result[cat0 - pnode->cat[0]];
		    if (*rp == 0) {
			*rp = ++result;
			store_reclass(result, primary, cat);
		    }
		    result_cell[col] = *rp;
		    q = 0;
		    break;
		case LEFT:
		    q = pnode->left;
		    break;
		case RIGHT:
		    q = pnode->right;
		    break;
		}
	    }
	    if (dir == FOUND)
		continue;

	    /* add a new node to the tree and put this value into it */
	    N++;
	    /* grow the tree? */
	    if (N >= tlen) {
		tree =
		    (NODE *) G_realloc(tree, sizeof(NODE) * (tlen += INCR));
		pnode = &tree[p];	/* realloc moves tree, must reassign pnode */
	    }

	    new_node = &tree[N];
	    new_node->cat = (CELL *) G_calloc(nfiles, sizeof(CELL));
	    new_node->result = (CELL *) G_calloc(NCATS, sizeof(CELL));

	    if (cat0 < 0)
		cat0 = -((-cat0) >> SHIFT) - 1;
	    else
		cat0 = cat0 >> SHIFT;

	    if (cat0 < 0)
		cat0 = -((-cat0) << SHIFT) + 1;
	    else
		cat0 = cat0 << SHIFT;
	    new_node->cat[0] = cat0;

	    for (i = 1; i < nfiles; i++)
		new_node->cat[i] = cat[i];

	    for (i = 0; i < NCATS; i++)
		new_node->result[i] = 0;
	    result_cell[col] = new_node->result[cat[0] - cat0] = ++result;
	    store_reclass(result, primary, cat);

	    new_node->left = 0;

	    if (dir == LEFT) {
		new_node->right = -p;	/* create thread */
		pnode->left = N;	/* insert left */
	    }
	    else {
		new_node->right = pnode->right;	/* copy right link/thread */
		pnode->right = N;	/* add right */
	    }
	}
	G_put_raster_row(outfd, result_cell, CELL_TYPE);
    }
    G_percent(nrows, nrows, 5);

    /* free some memory */
    uproot_tree();
    for (i = 0; i < nfiles; i++)
	G_free(cell[i]);
    return result;
}
