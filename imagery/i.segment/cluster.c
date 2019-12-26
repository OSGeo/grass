
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
#include <math.h>
#include <time.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "iseg.h"

#define INCR 1024

/* defeats the purpose of padding ... */
#define CISNULL(r, c) (((c) == 0 || (c) == ncols + 1 ? 1 : \
                       (FLAG_GET(globals->null_flag, (r), (c - 1)))))


CELL cluster_bands(struct globals *globals)
{
    register int col;
    register int i, n;
    /* input */
    DCELL **prev_in, **cur_in, **temp_in;
    struct ngbr_stats Ri, Rk, Rn;
    int nin;
    int diag;
    int bcol;
    /* output */
    CELL OLD, NEW;
    CELL *temp_clump, out_cell;
    CELL *prev_clump, *cur_clump;
    CELL *index, *renumber;
    CELL label, cellmax;
    int nrows, ncols;
    int row;
    int len;
    int nalloc;
    char *cname;
    int cfd, csize;
    CELL cat;
    int mwrow, mwrow1, mwrow2, mwnrows, mwcol, mwcol1, mwcol2, mwncols, radiusc;
    double diff, diff2, avgdiff, ka2, hspat, hspat2;
    double hspec, hspecad, hspec2, hspec2ad;
    LARGEINT count;

    G_message(_("%d-band clustering with threshold %g"), globals->nbands, globals->hr);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    hspec = globals->hr;
    hspec2 = globals->hr * globals->hr;
    nin = globals->nbands;
    diag = (globals->nn == 8);
    radiusc = globals->hs;
    mwnrows = mwncols = radiusc * 2 + 1;

    /* spatial bandwidth */
    hspat = globals->hs;
    if (hspat < 1)
	hspat = 1.5;
    hspat2 = hspat * hspat;

    cellmax = ((CELL)1 << (sizeof(CELL) * 8 - 2)) - 1;
    cellmax += ((CELL)1 << (sizeof(CELL) * 8 - 2));

    Ri.mean = NULL;
    Rk.mean = NULL;
    Rn.mean = G_malloc(globals->datasize);

    /* allocate clump index */
    /* TODO: support smallest label ID > 1 */
    nalloc = INCR;
    index = (CELL *) G_malloc(nalloc * sizeof(CELL));
    index[0] = 0;
    renumber = NULL;

    /* allocate DCELL buffers two columns larger than current window */
    prev_in = (DCELL **) G_malloc(sizeof(DCELL *) * (ncols + 2));
    cur_in = (DCELL **) G_malloc(sizeof(DCELL *) * (ncols + 2));
    
    prev_in[0] = (DCELL *) G_malloc(globals->datasize * (ncols + 2) * nin);
    cur_in[0] = (DCELL *) G_malloc(globals->datasize * (ncols + 2) * nin);

    Rast_set_d_null_value(cur_in[0], (ncols + 2) * nin);
    Rast_set_d_null_value(prev_in[0], (ncols + 2) * nin);

    for (i = 1; i < ncols + 2; i++) {
	prev_in[i] = prev_in[i - 1] + nin; 
	cur_in[i] = cur_in[i - 1] + nin; 
    }

    /* allocate CELL buffers two columns larger than current window */
    len = (ncols + 2) * sizeof(CELL);
    prev_clump = (CELL *) G_malloc(len);
    cur_clump = (CELL *) G_malloc(len);

    /* temp file for initial clump IDs */
    cname = G_tempfile();
    if ((cfd = open(cname, O_RDWR | O_CREAT | O_EXCL, 0600)) < 0)
	G_fatal_error(_("Unable to open temp file"));
    csize = ncols * sizeof(CELL);

    /* initialize clump labels */
    G_zero(cur_clump, len);
    G_zero(prev_clump, len);
    /* TODO: support smallest label ID > 1 */
    label = 0;

    /****************************************************
     *                      PASS 1                      *
     * pass thru the input, create initial clump labels *
     ****************************************************/

    G_message(_("Assigning initial region IDs..."));
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);

	for (col = 1; col <= ncols; col++) {

	    /* get band values */
	    Segment_get(globals->bands_out, (void *)cur_in[col], row, col - 1);
	    Ri.mean = cur_in[col];

	    if (CISNULL(row, col)) {
		cur_clump[col] = 0;
		continue;
	    }
	    
	    hspec2ad = hspec2;

	    if (globals->ms_adaptive) {
		/* adapt initial range bandwidth */

		mwrow1 = row - radiusc;
		mwrow2 = mwrow1 + mwnrows;
		if (mwrow1 < 0)
		    mwrow1 = 0;
		if (mwrow2 > nrows)
		    mwrow2 = nrows;

		mwcol1 = col - radiusc;
		mwcol2 = mwcol1 + mwncols;
		if (mwcol1 < 0)
		    mwcol1 = 0;
		if (mwcol2 > ncols)
		    mwcol2 = ncols;

		ka2 = hspec2; 	/* OTB: conductance parameter */
		
		avgdiff = 0;
		count = 0;
		for (mwrow = mwrow1; mwrow < mwrow2; mwrow++) {
		    for (mwcol = mwcol1; mwcol < mwcol2; mwcol++) {
			if ((FLAG_GET(globals->null_flag, mwrow, mwcol)))
			    continue;
			if (mwrow == row && mwcol == col)
			    continue;

			diff = mwrow - row;
			diff2 = diff * diff;
			diff = mwcol - col;
			diff2 += diff * diff;

			if (diff2 <= hspat2) {

			    Segment_get(globals->bands_out, (void *)Rn.mean,
					mwrow, mwcol);

			    /* get spectral distance */
			    diff2 = (globals->calculate_similarity)(&Ri, &Rn, globals);

			    avgdiff += sqrt(diff2);
			    count++;
			}
		    }
		}
		hspec2ad = 0;
		if (avgdiff > 0) {
		    avgdiff /= count;
		    hspecad = hspec;
		    /* OTB-like, contrast enhancing */
		    hspecad = exp(-avgdiff * avgdiff / (2 * ka2)) * avgdiff;
		    /* preference for large regions, from Perona Malik 1990 
		     * if the settings are right, it could be used to reduce noise */
		    /* hspecad = 1 / (1 + (avgdiff * avgdiff / (2 * hspec2))); */
		    hspec2ad = hspecad * hspecad;
		    G_debug(1, "avg spectral diff: %g", avgdiff);
		    G_debug(1, "initial hspec2: %g", hspec2);
		    G_debug(1, "adapted hspec2: %g", hspec2ad);
		}
	    }

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

	    Rk.mean = cur_in[col - 1];

	    /* same clump as to the left */
	    if (!CISNULL(row, col - 1) &&
	        globals->calculate_similarity(&Ri, &Rk, globals) <= hspec2ad) {
		OLD = cur_clump[col] = cur_clump[col - 1];
	    }

	    if (diag) {
		/* check above right, center, left, in that order */
		temp_clump = prev_clump + col + 1;
		bcol = col + 1;
		do {
		    Rk.mean = prev_in[bcol];
		    if (row > 0 && !CISNULL(row - 1, bcol) &&
		        globals->calculate_similarity(&Ri, &Rk, globals) <= hspec2ad) {
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
		Rk.mean = prev_in[col];

		if (row > 0 && !CISNULL(row - 1, col) &&
		    globals->calculate_similarity(&Ri, &Rk, globals) <= hspec2ad) {
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
		    if (label == cellmax)
			G_fatal_error(_("Too many objects: integer overflow"));

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

    G_free(prev_in[0]);
    G_free(cur_in[0]);
    G_free(prev_in);
    G_free(cur_in);

    /* generate a renumbering scheme */
    G_message(_("Generating renumbering scheme..."));
    G_debug(1, "%d initial labels", label);
    /* allocate final clump ID */
    renumber = (CELL *) G_malloc((label + 1) * sizeof(CELL));
    renumber[0] = 0;
    cat = 0;
    G_percent(0, label, 1);
    for (n = 1; n <= label; n++) {
	G_percent(n, label, 1);
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
	    renumber[n] = ++cat;
    }

    if (cat > cellmax - globals->max_rid)
	G_fatal_error(_("Too many objects: integer overflow"));

    /* rewind temp file */
    lseek(cfd, 0, SEEK_SET);

    /****************************************************
     *                      PASS 2                      *
     * apply renumbering scheme to initial clump labels *
     ****************************************************/

    /* the input raster is no longer needed, 
     * using instead the temp file with initial clump labels */

    G_message(_("Assigning final region IDs..."));
    for (row = 0; row < nrows; row++) {

	G_percent(row, nrows, 2);
    
	if (read(cfd, cur_clump, csize) != csize)
	    G_fatal_error(_("Unable to read from temp file"));

	temp_clump = cur_clump;

	for (col = 0; col < ncols; col++) {
	    if (!(FLAG_GET(globals->null_flag, row, col))) {
		out_cell = renumber[index[*temp_clump]] + globals->max_rid;

		Segment_put(&globals->rid_seg, (void *)&out_cell, row, col);
	    }
	    temp_clump++;
	}
    }
    G_percent(1, 1, 1);

    close(cfd);
    unlink(cname);

    /* free */
    G_free(prev_clump);
    G_free(cur_clump);
    G_free(index);
    G_free(renumber);

    G_message(_("Found %d clumps"), cat);
    globals->max_rid += cat;

    return globals->max_rid;
}
