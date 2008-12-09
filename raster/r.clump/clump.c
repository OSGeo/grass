
/****************************************************************************
 *
 * MODULE:       r.clump
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Recategorizes data in a raster map layer by grouping cells
 *               that form physically discrete areas into unique categories.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <time.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

#define INCR 1024


CELL clump(int in_fd, int out_fd)
{
    register int col;
    register CELL *prev_clump, *cur_clump;
    register CELL *index;
    register int n;
    CELL *prev_in, *cur_in;
    CELL *temp_cell, *temp_clump, *out_cell;
    CELL X, UP, LEFT, NEW, OLD;
    CELL label;
    int nrows, ncols;
    int row;
    int len;
    int pass;
    int nalloc;
    long cur_time;
    int column;


    nrows = G_window_rows();
    ncols = G_window_cols();

    /* allocate reclump index */
    nalloc = INCR;
    index = (CELL *) G_malloc(nalloc * sizeof(CELL));
    index[0] = 0;

    /* allocate CELL buffers one column larger than current window */
    len = (ncols + 1) * sizeof(CELL);
    prev_in = (CELL *) G_malloc(len);
    cur_in = (CELL *) G_malloc(len);
    prev_clump = (CELL *) G_malloc(len);
    cur_clump = (CELL *) G_malloc(len);
    out_cell = (CELL *) G_malloc(len);

/******************************** PASS 1 ************************************
 * first pass thru the input simulates the clump to determine
 * the reclumping index.
 * second pass does the clumping for real
 */
    time(&cur_time);
    for (pass = 1; pass <= 2; pass++) {
	/* second pass must generate a renumbering scheme */
	if (pass == 2) {
	    CELL cat, *renumber;

	    renumber = (CELL *) G_malloc((label + 1) * sizeof(CELL));
	    cat = 1;
	    for (n = 1; n <= label; n++)
		renumber[n] = 0;
	    for (n = 1; n <= label; n++)
		renumber[index[n]] = 1;
	    for (n = 1; n <= label; n++)
		if (renumber[n])
		    renumber[n] = cat++;
	    for (n = 1; n <= label; n++)
		index[n] = renumber[index[n]];
	    G_free(renumber);
	}

	/* fake a previous row which is all zero */
	G_zero(prev_in, len);
	G_zero(prev_clump, len);

	/* create a left edge of zero */
	cur_in[0] = 0;
	cur_clump[0] = 0;

	/* initialize clump labels */
	label = 0;

	G_message(_("Pass %d..."), pass);
	for (row = 0; row < nrows; row++) {
	    if (G_get_map_row(in_fd, cur_in + 1, row) < 0)
		G_fatal_error(_("Unable to read raster map row %d "),
			      row);
	    
	    G_percent(row+1, nrows, 2);
	    X = 0;
	    for (col = 1; col <= ncols; col++) {
		LEFT = X;
		X = cur_in[col];
		if (X == 0) {	/* don't clump zero data */
		    cur_clump[col] = 0;
		    continue;
		}

		UP = prev_in[col];

		/*
		 * if the cell value is different above and to the left
		 * then we must start a new clump
		 *
		 * this new clump may eventually collide with another
		 * clump and have to be merged
		 */
		if (X != LEFT && X != UP) {	/* start a new clump */
		    label++;
		    cur_clump[col] = label;
		    if (pass == 1) {
			if (label >= nalloc) {
			    nalloc += INCR;
			    index =
				(CELL *) G_realloc(index,
						   nalloc * sizeof(CELL));
			}
			index[label] = label;
		    }
		    continue;
		}
		if (X == LEFT && X != UP) {	/* same clump as to the left */
		    cur_clump[col] = cur_clump[col - 1];
		    continue;
		}
		if (X == UP && X != LEFT) {	/* same clump as above */
		    cur_clump[col] = prev_clump[col];
		    continue;
		}

		/*
		 * at this point the cell value X is the same as LEFT and UP
		 * so it should go into the same clump. It is possible for
		 * the clump value on the left to differ from the clump value
		 * above. If this happens we have a conflict and one of the
		 * LEFT or UP needs to be reclumped
		 */
		if (cur_clump[col - 1] == prev_clump[col]) {	/* ok */
		    cur_clump[col] = prev_clump[col];
		    continue;
		}

		/* conflict! preserve the clump from above and change the left.
		 * Must also go back to the left in the current row and to the right
		 * in the previous row to change all the clump values as well.
		 *
		 */

		NEW = prev_clump[col];
		OLD = cur_clump[col - 1];
		cur_clump[col] = NEW;

		/* to left
		   for (n = 1; n < col; n++)
		   if (cur_clump[n] == OLD)
		   cur_clump[n] = NEW;
		 */

		temp_cell = cur_clump++;
		n = col - 1;
		while (n-- > 0)
		    if (*cur_clump == OLD)
			*cur_clump++ = NEW;
		    else
			cur_clump++;
		cur_clump = temp_cell;

		/* to right
		   for (n = col+1; n <= ncols; n++)
		   if (prev_clump[n] == OLD)
		   prev_clump[n] = NEW;
		 */

		temp_cell = prev_clump;
		prev_clump += col + 1;
		n = ncols - col;
		while (n-- > 0)
		    if (*prev_clump == OLD)
			*prev_clump++ = NEW;
		    else
			prev_clump++;
		prev_clump = temp_cell;

		/* modify the indexes
		   if (pass == 1)
		   for (n = 1; n <= label; n++)
		   if (index[n] == OLD)
		   index[n] = NEW;
		 */

		if (pass == 1) {
		    temp_cell = index++;
		    n = label;
		    while (n-- > 0)
			if (*index == OLD)
			    *index++ = NEW;
			else
			    index++;
		    index = temp_cell;
		}
	    }

	    if (pass == 2) {
		/*
		   for (col = 1; col <= ncols; col++)
		   out_cell[col] = index[cur_clump[col]];

		   if (G_put_raster_row (out_fd, out_cell+1, CELL_TYPE) < 0)
		   G_fatal_error (_("Unable to properly write output raster map"));
		 */
		col = ncols;
		temp_clump = cur_clump + 1;	/* skip left edge */
		temp_cell = out_cell;

		while (col-- > 0)
		    *temp_cell++ = index[*temp_clump++];

		for (column = 0; column < ncols; column++) {
		    if (out_cell[column] == 0)
			G_set_null_value(&out_cell[column], 1, CELL_TYPE);
		}
		if (G_put_raster_row(out_fd, out_cell, CELL_TYPE) < 0)
		    G_fatal_error(_("Failed writing raster map row %d"),
				  row);
	    }

	    /* switch the buffers so that the current buffer becomes the previous */
	    temp_cell = cur_in;
	    cur_in = prev_in;
	    prev_in = temp_cell;

	    temp_clump = cur_clump;
	    cur_clump = prev_clump;
	    prev_clump = temp_clump;
	}

	print_time(&cur_time);
    }
    return 0;
}

int print_time(long *start)
{
    int hours, minutes, seconds;
    long done;

    time(&done);

    seconds = done - *start;
    *start = done;

    hours = seconds / 3600;
    minutes = (seconds - hours * 3600) / 60;
    seconds = seconds % 60;

    if (hours)
	G_verbose_message("%2d:%02d:%02d", hours, minutes, seconds);
    else if (minutes)
	G_verbose_message("%d:%02d", minutes, seconds);
    else
	G_verbose_message("%d seconds", seconds);

    return 0;
}
