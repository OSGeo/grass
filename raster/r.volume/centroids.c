/* Find 'centroids' of all cats.  Most useful with cats which are
 * like clumps (contiguous areas), but will work on any file.
 * Respects the current window and mask. 
 * Category zero and negative categories are not done!
 * Returned centroids are (row, col) pairs in n, e.
 * Returned value is 0 for most cases, > 0 if 'both' method
 * selected and some values fell outside 'clumps', and were
 * adjusted.
 * Two methods can be used: 'distance-weighted' and 'counting'.
 * 
 * Method 0 = 'counting' or 'clump'; centroid point guaranteed
 * to be at a cell of the given category.
 * 1 = Both 0 and 2 are run; if method 2 centroid is on
 * cell of proper cat, it is used, otherwise
 * method 1 value is substituted.
 * 2 = 'distance-weighted'; row = sigma(row)/n,
 * col = sigma(col)/n.
 */
#include <grass/gis.h>
#include <grass/raster.h>

int centroids(int fd,		/* File descriptor of map layer to process */
	      /* This file is assumed to be opened before calling */
	      /*   centroids. */
	      int *e, int *n,	/* Pointers to arrays at least max+1 long */
	      int method,	/* 0, 1, or 2; see above. */
	      int max)
{				/* Highest positive cat number in map layer */
    CELL *cell_buf, v;
    int i, adjusted, numb, left, right;
    long int *count;
    int row, col, rows, cols;

    adjusted = 0;

    cell_buf = Rast_allocate_c_buf();
    /* space to accumulate counts */
    count = (long int *)G_malloc((max + 1) * sizeof(long int));

    /* zero the count totals */
    for (i = 1; i <= max; i++) {
	count[i] = 0;
	e[i] = 0;
	n[i] = 0;
    }

    /* do rows and columns through window and mask */
    /*  to do counting */
    rows = Rast_window_rows();
    cols = Rast_window_cols();
    for (row = 0; row < rows; row++) {
	Rast_get_c_row(fd, cell_buf, row);	/* get a row */
	for (col = 0; col < cols; col++) {
	    v = cell_buf[col];	/* next cell value in row */
	    if (v < 1)
		continue;	/* can't handle 0 or - values */
	    count[v]++;
	    if (method > 0) {	/* acccumulate row, col weights */
		e[v] += col;
		n[v] += row;
	    }
	}
    }

    /* compute averages */
    if (method > 0)
	for (i = 0; i <= max; i++) {
	    if (count[i]) {
		numb = count[i];
		e[i] /= numb;
		n[i] /= numb;
	    }
	}

    /* substitute 'count' centroids for 'weighted' ones, if necessary */
    if (method == 1) {
	for (i = 1; i <= max; i++) {
	    if (count[i]) {
		row = n[i];
		col = e[i];
		/* get cell at row,col */
		Rast_get_c_row(fd, cell_buf, row);
		v = cell_buf[col];
		if (v > 0) {
		    if (v == i)
			count[i] = 0;	/* weighted is acceptable */
		    else
			adjusted++;
		}
	    }
	}
    }

    /* compute middle cell; zeros will remain zeros */
    for (i = 1; i <= max; i++)
	count[i] = (count[i] + 1) / 2;

    /* go through map again */
    for (row = 0; row < rows; row++) {
	Rast_get_c_row(fd, cell_buf, row);
	for (col = 0; col < cols; col++) {
	    v = cell_buf[col];
	    if (v < 1)
		continue;
	    if (count[v] == 0)
		continue;
	    if ((--count[v]) == 0) {	/* then this is middle cell */
		n[v] = row;
		/* find row-center in this clump */
		left = right = col;
		/* left edge first */
		while (left > 0)
		    if (cell_buf[--left] != v) {
			left++;
			break;
		    }
		/* then right edge */
		while (right < cols)
		    if (cell_buf[++right] != v) {
			right--;
			break;
		    }
		e[v] = (left + right) / 2;
	    }
	}
    }
    G_free(cell_buf);
    G_free(count);
    return (adjusted);
}				/* end of centroids() */
