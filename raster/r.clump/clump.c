
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
 * COPYRIGHT:    (C) 2006-2016 by the GRASS Development Team
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

int print_time(time_t *);

static CELL do_renumber(int *in_fd, DCELL *rng, int nin,
                        int diag, int minsize, 
			int cfd, CELL label, CELL *index, int out_fd)
{
    int row, col, nrows, ncols;
    int n;
    CELL OLD, NEW;
    CELL *temp_cell, *temp_clump;
    CELL *cur_clump, *out_cell;
    CELL *clumpid;
    CELL cat;
    int csize;

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    csize = ncols * sizeof(CELL);

    /* generate a renumbering scheme */
    G_message(_("Generating renumbering scheme..."));
    G_debug(1, "%d initial labels", label);
    /* allocate final clump ID */
    clumpid = (CELL *) G_malloc((label + 1) * sizeof(CELL));
    clumpid[0] = 0;
    cat = 0;
    G_percent(0, label, 1);
    for (n = 1; n <= label; n++) {
	G_percent(n, label, 1);
	OLD = n;
	NEW = index[n];
	if (OLD != NEW) {
	    clumpid[n] = 0;
	    /* find valid clump ID */
	    while (OLD != NEW) {
		OLD = NEW;
		NEW = index[OLD];
	    }
	    index[n] = NEW;
	}
	else
	    /* set final clump id */
	    clumpid[n] = ++cat;
    }

    /****************************************************
     *                      PASS 2                      *
     * apply renumbering scheme to initial clump labels *
     ****************************************************/

    G_message(_("Pass 2 of 2..."));

    if (minsize > 1) {
	int do_write;
	off_t coffset;
	CELL new_clump;

	cur_clump = Rast_allocate_c_buf();

	for (row = 0; row < nrows; row++) {

	    G_percent(row, nrows, 2);

	    coffset = (off_t)row * csize;
	    lseek(cfd, coffset, SEEK_SET);
	    if (read(cfd, cur_clump, csize) != csize)
		G_fatal_error(_("Unable to read from temp file"));

	    temp_clump = cur_clump;

	    do_write = 0;
	    for (col = 0; col < ncols; col++) {
		new_clump = clumpid[index[*temp_clump]];
		if (*temp_clump != new_clump) {
		    *temp_clump = new_clump;
		    do_write = 1;
		}
		temp_clump++;
	    }
	    if (do_write) {
		lseek(cfd, coffset, SEEK_SET);
		if (write(cfd, cur_clump, csize) != csize)
		    G_fatal_error(_("Unable to write to temp file"));
	    }
	}
	G_percent(1, 1, 1);

	G_free(cur_clump);
	G_free(index);
	G_free(clumpid);

	G_message(_("%d initial clumps"), cat);

	return merge_small_clumps(in_fd, nin, rng,
                        diag, minsize, &cat, 
			cfd, out_fd);
    }

    if (out_fd < 0) {
	fprintf(stdout, "clumps=%d\n", cat);
	
	return cat;
    }

    /* the input raster is no longer needed, 
     * using instead the temp file with initial clump labels */

    /* rewind temp file */
    lseek(cfd, 0, SEEK_SET);

    cur_clump = Rast_allocate_c_buf();
    out_cell = Rast_allocate_c_buf();

    for (row = 0; row < nrows; row++) {

	G_percent(row, nrows, 2);

	if (read(cfd, cur_clump, csize) != csize)
	    G_fatal_error(_("Unable to read from temp file"));

	temp_clump = cur_clump;
	temp_cell = out_cell;

	for (col = 0; col < ncols; col++) {
	    *temp_cell = clumpid[index[*temp_clump]];
	    if (*temp_cell == 0) {
		Rast_set_c_null_value(temp_cell, 1);
	    }
	    temp_clump++;
	    temp_cell++;
	}
	Rast_put_row(out_fd, out_cell, CELL_TYPE);
    }
    G_percent(1, 1, 1);

    G_free(cur_clump);
    G_free(out_cell);
    G_free(index);
    G_free(clumpid);

    return cat;
}

CELL clump(int *in_fd, int out_fd, int diag, int minsize)
{
    register int col;
    register int n;
    CELL NEW, OLD;
    CELL *temp_cell, *temp_clump;
    CELL *prev_in, *cur_in;
    CELL *prev_clump, *cur_clump;
    CELL X, LEFT;
    CELL *index;
    CELL label;
    int nrows, ncols;
    int row;
    int len;
    int nalloc;
    time_t cur_time;
    char *cname;
    int cfd, csize;

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* allocate clump index */
    nalloc = INCR;
    index = (CELL *) G_malloc(nalloc * sizeof(CELL));
    index[0] = 0;

    /* allocate CELL buffers two columns larger than current window */
    len = (ncols + 2) * sizeof(CELL);
    prev_in = (CELL *) G_malloc(len);
    cur_in = (CELL *) G_malloc(len);
    prev_clump = (CELL *) G_malloc(len);
    cur_clump = (CELL *) G_malloc(len);

    /* temp file for initial clump IDs */
    cname = G_tempfile();
    if ((cfd = open(cname, O_RDWR | O_CREAT | O_EXCL, 0600)) < 0)
	G_fatal_error(_("Unable to open temp file"));
    csize = ncols * sizeof(CELL);

    time(&cur_time);

    /* fake a previous row which is all NULL */
    Rast_set_c_null_value(prev_in, ncols + 2);

    /* set left and right edge to NULL */
    Rast_set_c_null_value(&cur_in[0], 1);
    Rast_set_c_null_value(&cur_in[ncols + 1], 1);

    /* initialize clump labels */
    G_zero(cur_clump, len);
    G_zero(prev_clump, len);
    label = 0;

    /****************************************************
     *                      PASS 1                      *
     * pass thru the input, create initial clump labels *
     ****************************************************/

    G_message(_("Pass 1 of 2..."));
    for (row = 0; row < nrows; row++) {
	Rast_get_c_row(*in_fd, cur_in + 1, row);

	G_percent(row, nrows, 2);
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

	    /* try to connect the current cell to an existing clump */
	    OLD = NEW = 0;
	    /* same clump as to the left */
	    if (X == LEFT) {
		OLD = cur_clump[col] = cur_clump[col - 1];
	    }

	    if (diag) {
		/* check above right, center, left, in that order */
		n = 2;
		temp_clump = prev_clump + col + 1;
		temp_cell = prev_in + col + 1;
		do {
		    if (X == *temp_cell) {
			cur_clump[col] = *temp_clump;
			if (OLD == 0) {
			    OLD = *temp_clump;
			}
			else {
			    NEW = *temp_clump;
			    break;
			}
		    }
		    temp_cell--;
		    temp_clump--;
		} while (n-- > 0);
	    }
	    else {
		/* check above */
		if (X == prev_in[col]) {
		    temp_clump = prev_clump + col;
		    cur_clump[col] = *temp_clump;
		    if (OLD == 0) {
			OLD = *temp_clump;
			}
		    else {
			NEW = *temp_clump;
		    }
		}
	    }

	    if (NEW == 0 || OLD == NEW) {	/* ok */
		if (OLD == 0) {
		    /* start a new clump */
		    label++;
		    cur_clump[col] = label;
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
	    temp_clump = prev_clump + col;
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
	/* this works also with writing out cur_clump, but only 
	 * prev_clump is complete and will not change any more */
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

    /* free */
    G_free(prev_clump);
    G_free(cur_clump);

    G_free(prev_in);
    G_free(cur_in);

    do_renumber(in_fd, NULL, 1, diag, minsize, cfd, label, index, out_fd);

    close(cfd);
    unlink(cname);

    print_time(&cur_time);

    return 0;
}

static double get_diff2(DCELL **a, int acol, DCELL **b, int bcol, DCELL *rng, int n)
{
    int i;
    double diff, diff2;

    diff2 = 0;
    for (i = 0; i < n; i++) {
	if (Rast_is_d_null_value(&b[i][bcol]))
	    return 2;
	diff = a[i][acol] - b[i][bcol];
	/* normalize with the band's range */
	if (rng[i])
	    diff /= rng[i];
	diff2 += diff * diff;
    }
    /* normalize difference to the range [0, 1] */
    diff2 /= n;
    
    return diff2;
}

CELL clump_n(int *in_fd, char **inname, int nin, double threshold,
             int out_fd, int diag, int minsize)
{
    register int col;
    register int i, n;
    /* input */
    DCELL **prev_in, **cur_in, **temp_in;
    int bcol;
    DCELL *rng, maxdiff;
    double thresh2;
    /* output */
    CELL OLD, NEW;
    CELL *temp_clump;
    CELL *prev_clump, *cur_clump;
    CELL *index;
    CELL label;
    int nrows, ncols;
    int row;
    int isnull;
    int len;
    int nalloc;
    time_t cur_time;
    char *cname;
    int cfd, csize;

    G_message(_("%d-band clumping with threshold %g"), nin, threshold);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    thresh2 = threshold * threshold;

    /* allocate clump index */
    nalloc = INCR;
    index = (CELL *) G_malloc(nalloc * sizeof(CELL));
    index[0] = 0;

    /* allocate DCELL buffers two columns larger than current window */
    len = (ncols + 2) * sizeof(DCELL);
    prev_in = (DCELL **) G_malloc(sizeof(DCELL *) * nin);
    cur_in = (DCELL **) G_malloc(sizeof(DCELL *) * nin);
    rng = G_malloc(sizeof(DCELL) * nin);

    maxdiff = 0;
    for (i = 0; i < nin; i++) {
	struct FPRange fp_range;	/* min/max values of each input raster */
	DCELL min, max;

	if (Rast_read_fp_range(inname[i], "", &fp_range) != 1)
	    G_fatal_error(_("No min/max found in raster map <%s>"),
			  inname[i]);
	Rast_get_fp_range_min_max(&fp_range, &min, &max);
	rng[i] = max - min;
	maxdiff += rng[i] * rng[i];

	prev_in[i] = (DCELL *) G_malloc(len);
	cur_in[i] = (DCELL *) G_malloc(len);

	/* fake a previous row which is all NULL */
	Rast_set_d_null_value(prev_in[i], ncols + 2);

	/* set left and right edge to NULL */
	Rast_set_d_null_value(&cur_in[i][0], 1);
	Rast_set_d_null_value(&cur_in[i][ncols + 1], 1);
    }
    G_debug(1, "maximum possible difference: %g", maxdiff);

    /* allocate CELL buffers two columns larger than current window */
    len = (ncols + 2) * sizeof(CELL);
    prev_clump = (CELL *) G_malloc(len);
    cur_clump = (CELL *) G_malloc(len);

    /* temp file for initial clump IDs */
    cname = G_tempfile();
    if ((cfd = open(cname, O_RDWR | O_CREAT | O_EXCL, 0600)) < 0)
	G_fatal_error(_("Unable to open temp file"));
    csize = ncols * sizeof(CELL);

    time(&cur_time);

    /* initialize clump labels */
    G_zero(cur_clump, len);
    G_zero(prev_clump, len);
    label = 0;

    /****************************************************
     *                      PASS 1                      *
     * pass thru the input, create initial clump labels *
     ****************************************************/

    G_message(_("Pass 1 of 2..."));
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	for (i = 0; i < nin; i++) {
	    Rast_get_d_row(in_fd[i], cur_in[i] + 1, row);
	}

	for (col = 1; col <= ncols; col++) {
	    isnull = 0;
	    for (i = 0; i < nin; i++) {
		if (Rast_is_d_null_value(&cur_in[i][col])) {	/* don't clump NULL data */
		    cur_clump[col] = 0;
		    isnull = 1;
		    break;
		}
	    }
	    if (isnull)
		continue;

	    /*
	     * if the cell values are different to the left and above
	     * (diagonal: and above left and above right)
	     * then we must start a new clump
	     *
	     * this new clump may eventually collide with another
	     * clump and will have to be merged
	     */

	    /* try to connect the current cell to an existing clump */
	    OLD = NEW = 0;
	    /* same clump as to the left */
	    if (get_diff2(cur_in, col, cur_in, col - 1, rng, nin) <= thresh2) {
		OLD = cur_clump[col] = cur_clump[col - 1];
	    }

	    if (diag) {
		/* check above right, center, left, in that order */
		temp_clump = prev_clump + col + 1;
		bcol = col + 1;
		do {
		    if (get_diff2(cur_in, col, prev_in, bcol, rng, nin) <= thresh2) {
			cur_clump[col] = *temp_clump;
			if (OLD == 0) {
			    OLD = *temp_clump;
			}
			else {
			    NEW = *temp_clump;

			    /* threshold > 0 and diagonal requires a bit of extra work
			     * because of bridge cells:
			     * A similar to B, B similar to C, but A not similar to C
			     * -> B is bridge cell */
			    if (NEW != OLD) {
				CELL *temp_clump2;

				/* conflict! preserve NEW clump ID and change OLD clump ID.
				 * Must go back to the left in the current row and to the right
				 * in the previous row to change all the clump values as well.
				 */

				/* left of the current row from 1 to col - 1 */
				temp_clump2 = cur_clump;
				n = col - 1;
				while (n-- > 0) {
				    temp_clump2++;	/* skip left edge */
				    if (*temp_clump2 == OLD)
					*temp_clump2 = NEW;
				}

				/* right of previous row from col - 1 to ncols */
				temp_clump2 = prev_clump + col - 1;
				n = ncols - col + 2;
				while (n-- > 0) {
				    if (*temp_clump2 == OLD)
					*temp_clump2 = NEW;
				    temp_clump2++;
				}

				/* modify the OLD index */
				index[OLD] = NEW;

				OLD = NEW;
				NEW = 0;
			    }
			}
		    }
		    temp_clump--;
		} while (bcol-- > col - 1);
	    }
	    else {
		/* check above */
		if (get_diff2(cur_in, col, prev_in, col, rng, nin) <= thresh2) {
		    temp_clump = prev_clump + col;
		    cur_clump[col] = *temp_clump;
		    if (OLD == 0) {
			OLD = *temp_clump;
			}
		    else {
			NEW = *temp_clump;
			if (NEW != OLD) {

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
			    temp_clump = prev_clump + col;
			    n = ncols - col;
			    while (n-- > 0) {
				temp_clump++;	/* skip col */
				if (*temp_clump == OLD)
				    *temp_clump = NEW;
			    }

			    /* modify the OLD index */
			    index[OLD] = NEW;

			    OLD = NEW;
			    NEW = 0;
			}
		    }
		}
	    }

	    if (NEW == 0 || OLD == NEW) {	/* ok */
		if (OLD == 0) {
		    /* start a new clump */
		    label++;
		    cur_clump[col] = label;
		    if (label >= nalloc) {
			nalloc += INCR;
			index =
			    (CELL *) G_realloc(index,
					       nalloc * sizeof(CELL));
		    }
		    index[label] = label;
		}
	    }
	    /* else the relabelling above failed */
	}

	/* write initial clump IDs */
	/* this works also with writing out cur_clump, but only 
	 * prev_clump is complete and will not change any more */
	if (row > 0) {
	    if (write(cfd, prev_clump + 1, csize) != csize)
		G_fatal_error(_("Unable to write to temp file"));
	}

	/* switch the buffers so that the current buffer becomes the previous */
	temp_in = cur_in;
	cur_in = prev_in;
	prev_in = temp_in;

	temp_clump = cur_clump;
	cur_clump = prev_clump;
	prev_clump = temp_clump;
    }
    /* write last row with initial clump IDs */
    if (write(cfd, prev_clump + 1, csize) != csize)
	G_fatal_error(_("Unable to write to temp file"));
    G_percent(1, 1, 1);

    /* free */
    G_free(prev_clump);
    G_free(cur_clump);

    for (i = 0; i < nin; i++) {
	G_free(prev_in[i]);
	G_free(cur_in[i]);
    }
    G_free(prev_in);
    G_free(cur_in);

    do_renumber(in_fd, rng, nin, diag, minsize, cfd, label, index, out_fd);

    close(cfd);
    unlink(cname);

    print_time(&cur_time);

    return 0;
}

int print_time(time_t *start)
{
    int hours, minutes, seconds;
    time_t done;

    time(&done);

    seconds = (int)(done - *start);
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
