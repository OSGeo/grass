
/****************************************************************************
 *
 * MODULE:       r.clump
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *               Markus Metz
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

#define INCR 1024

CELL clump(int in_fd, int out_fd, int diag)
{
    register int col;
    register CELL *prev_clump, *cur_clump;
    register CELL *index, *renumber;
    register int n;
    CELL *prev_in, *cur_in;
    CELL *temp_cell, *temp_clump, *out_cell;
    CELL X, UP, LEFT, UL, UR, NEW, OLD;
    CELL label;
    int nrows, ncols;
    int row;
    int len;
    int nalloc;
    long cur_time;
    char *cname;
    int cfd, csize;
    CELL cat;

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* allocate clump index */
    nalloc = INCR;
    index = (CELL *) G_malloc(nalloc * sizeof(CELL));
    index[0] = 0;
    renumber = NULL;

    /* allocate CELL buffers two columns larger than current window */
    len = (ncols + 2) * sizeof(CELL);
    prev_in = (CELL *) G_malloc(len);
    cur_in = (CELL *) G_malloc(len);
    prev_clump = (CELL *) G_malloc(len);
    cur_clump = (CELL *) G_malloc(len);
    out_cell = (CELL *) G_malloc(len);

    /* temp file for initial clump IDs */
    cname = G_tempfile();
    if ((cfd = open(cname, O_RDWR | O_CREAT | O_EXCL, 0600)) < 0)
	G_fatal_error(_("Unable to open temp file"));
    csize = ncols * sizeof(CELL);

    time(&cur_time);

    /* fake a previous row which is all zero */
    Rast_set_c_null_value(prev_in, ncols + 2);
    G_zero(prev_clump, len);

    /* set left and right edge to NULL */
    Rast_set_c_null_value(&cur_in[0], 1);
    Rast_set_c_null_value(&cur_in[ncols + 1], 1);

    /* set above left and right to NULL for diag == 0 */
    Rast_set_c_null_value(&UL, 1);
    Rast_set_c_null_value(&UR, 1);

    /* create a left and right edge of zero */
    G_zero(cur_clump, len);

    /* initialize clump labels */
    label = 0;

    /****************************************************
     *                      PASS 1                      *
     * pass thru the input, create initial clump labels *
     ****************************************************/

    G_message(_("Pass 1..."));
    for (row = 0; row < nrows; row++) {
	Rast_get_c_row(in_fd, cur_in + 1, row);

	G_percent(row, nrows, 4);
	Rast_set_c_null_value(&X, 1);
	for (col = 1; col <= ncols; col++) {
	    LEFT = X;
	    X = cur_in[col];
	    if (Rast_is_c_null_value(&X)) {	/* don't clump NULL data */
		cur_clump[col] = 0;
		continue;
	    }

	    /*
	     * if the cell value is different to the left and above
	     * (diagonal: and above left and above right)
	     * then we must start a new clump
	     *
	     * this new clump may eventually collide with another
	     * clump and will have to be merged
	     */

	    /* only one "if (diag)" for speed */
	    if (diag) {
		temp_cell = prev_in + col - 1;
		UL = *temp_cell++;
		UP = *temp_cell++;
		UR = *temp_cell;

		/* start a new clump */
		if (X != LEFT && X != UP && X != UL && X != UR) {
		    label++;
		    cur_clump[col] = label;
		    if (label >= nalloc) {
			nalloc += INCR;
			index =
			    (CELL *) G_realloc(index,
					       nalloc * sizeof(CELL));
		    }
		    index[label] = label;
		    continue;
		}

		OLD = NEW = 0;
		/* same clump as to the left */
		if (X == LEFT) {
		    OLD = cur_clump[col - 1];
		    cur_clump[col] = OLD;
		}
		/* check UL, UP, UR */
		n = 2;
		temp_clump = prev_clump + col + 1;
		temp_cell = prev_in + col + 1;
		do {
		    if (X == *temp_cell) {
			if (OLD == 0) {
			    OLD = *temp_clump;
			    cur_clump[col] = OLD;
			}
			else {
			    NEW = *temp_clump;
			    cur_clump[col] = NEW;
			}
		    }
		    if (NEW != 0)
			break;
		    temp_cell--;
		    temp_clump--;
		} while (n-- > 0);
	    }
	    else {
		UP = prev_in[col];

		/* start a new clump */
		if (X != LEFT && X != UP) {
		    label++;
		    cur_clump[col] = label;
		    if (label >= nalloc) {
			nalloc += INCR;
			index =
			    (CELL *) G_realloc(index,
					       nalloc * sizeof(CELL));
		    }
		    index[label] = label;
		    continue;
		}

		OLD = NEW = 0;
		/* same clump as to the left */
		if (X == LEFT) {
		    OLD = cur_clump[col - 1];
		    cur_clump[col] = OLD;
		}
		/* same clump as above */
		if (X == UP) {
		    if (OLD == 0) {
			OLD = prev_clump[col];
			cur_clump[col] = OLD;
		    }
		    else {
			NEW = prev_clump[col];
			cur_clump[col] = NEW;
		    }
		}
	    }

	    if (NEW == 0 || OLD == NEW) {	/* ok */
		continue;
	    }

	    /* conflict! preserve NEW clump ID and change OLD clump ID.
	     * Must go back to the left in the current row and to the right
	     * in the previous row to change all the clump values as well.
	     */

	    /* left of the current row from 1 to col - 1 */
	    temp_clump = cur_clump;
	    n = col - 1;
	    while (n-- > 0) {
		temp_clump++;	/* skip left edge */
		if (*temp_clump == OLD)
		    *temp_clump = NEW;
	    }

	    /* right of previous row from col + 1 to ncols */
	    temp_clump = prev_clump;
	    temp_clump += col;
	    n = ncols - col;
	    while (n-- > 0) {
		temp_clump++;	/* skip col */
		if (*temp_clump == OLD)
		    *temp_clump = NEW;
	    }

	    /* modify the OLD index */
	    index[OLD] = NEW;
	}

	/* write initial clump IDs */
	/* this works also with writing out cur_clump, 
	 * but only prev_clump is complete */
	if (row > 0) {
	    if (write(cfd, prev_clump + 1, csize) != csize)
		G_fatal_error(_("Unable to write to temp file"));
	}

	/* switch the buffers so that the current buffer becomes the previous */
	temp_cell = cur_in;
	cur_in = prev_in;
	prev_in = temp_cell;

	temp_clump = cur_clump;
	cur_clump = prev_clump;
	prev_clump = temp_clump;
    }
    /* write last row with initial clump IDs */
    if (write(cfd, prev_clump + 1, csize) != csize)
	G_fatal_error(_("Unable to write to temp file"));
    G_percent(1, 1, 1);

    /* generate a renumbering scheme */
    G_message(_("Generating renumbering scheme..."));
    G_debug(0, "%d initial labels", label);
    /* allocate final clump ID */
    renumber = (CELL *) G_malloc((label + 1) * sizeof(CELL));
    renumber[0] = 0;
    cat = 1;
    G_percent(0, label, 4);
    for (n = 1; n <= label; n++) {
	G_percent(n, label, 4);
	OLD = n;
	NEW = index[n];
	if (OLD != NEW) {
	    renumber[n] = 0;
	    /* find valid clump ID */
	    while (OLD != NEW) {
		OLD = NEW;
		NEW = index[OLD];
	    }
	    index[n] = NEW;
	}
	else
	    /* set final clump id */
	    renumber[n] = cat++;
    }
    
    /* rewind temp file */
    lseek(cfd, 0, SEEK_SET);

    /****************************************************
     *                      PASS 2                      *
     *             apply renumbering scheme             *
     ****************************************************/

    /* the input raster is no longer needed, 
     * instead we use the temp file with the initial clump labels */
    G_message(_("Pass 2..."));
    for (row = 0; row < nrows; row++) {

	G_percent(row, nrows, 4);
	
	if (read(cfd, cur_clump, csize) != csize)
	    G_fatal_error(_("Unable to read from temp file"));

	temp_clump = cur_clump;
	temp_cell = out_cell;

	for (col = 0; col < ncols; col++) {
	    *temp_cell = renumber[index[*temp_clump]];
	    if (*temp_cell == 0)
		Rast_set_null_value(temp_cell, 1, CELL_TYPE);
	    temp_clump++;
	    temp_cell++;
	}
	Rast_put_row(out_fd, out_cell, CELL_TYPE);
    }
    G_percent(1, 1, 1);
    close(cfd);
    unlink(cname);

    print_time(&cur_time);

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
