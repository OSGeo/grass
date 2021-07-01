#include <unistd.h>
#include <grass/rowio.h>
#include <grass/raster.h>
#include "glob.h"
#include "filter.h"

int execute_filter(ROWIO *r, int *out, FILTER *filter, DCELL **cell)
{
    int i;
    int count;
    int size;
    int row, rcount;
    int col, ccount;
    int startx, starty;
    int dx, dy;
    int mid;
    DCELL **bufs, **box, *cp;

    size = filter->size;
    mid = size / 2;
    bufs = (DCELL **) G_malloc(size * sizeof(DCELL *));
    box = (DCELL **) G_malloc(size * sizeof(DCELL *));

    switch (filter->start) {
    case UR:
	startx = ncols - size;
	starty = 0;
	dx = -1;
	dy = 1;
	break;
    case LL:
	startx = 0;
	starty = nrows - size;
	dx = 1;
	dy = -1;
	break;
    case LR:
	startx = ncols - size;
	starty = nrows - size;
	dx = -1;
	dy = -1;
	break;
    case UL:
    default:
	startx = 0;
	starty = 0;
	dx = 1;
	dy = 1;
	break;
    }
    direction = dy;

    G_debug(3, "direction %d, dx=%d, dy=%d", direction, dx, dy);

    rcount = nrows - (size - 1);
    ccount = ncols - (size - 1);

    /* rewind output */
    lseek(out[MASTER], 0L, 0);

    /* copy border rows to output */
    row = starty;
    for (i = 0; i < mid; i++) {
	cp = (DCELL *) Rowio_get(&r[MASTER], row);
	write(out[MASTER], cp, buflen);
	row += dy;
    }

    /* for each row */
    int id = MASTER;
    int work = 0;
    for (count = 0; count < rcount; count++) {
	G_percent(work, rcount, 2);
	row = starty;
	starty += dy;
	/* get "size" rows */
	for (i = 0; i < size; i++) {
	    bufs[i] = (DCELL *) Rowio_get(&r[id], row);
	    box[i] = bufs[i] + startx;
	    row += dy;
	}
	if (filter->type == SEQUENTIAL)
	    cell[id] = bufs[mid];
	/* copy border */
	cp = cell[id];
	for (i = 0; i < mid; i++)
	    *cp++ = bufs[mid][i];

	/* filter row */
	col = ccount;
	while (col--) {
	    if (null_only) {
		if (Rast_is_d_null_value(&box[mid][mid]))
		    *cp++ = apply_filter(filter, box);
		else
		    *cp++ = box[mid][mid];
	    }
	    else {
		*cp++ = apply_filter(filter, box);
	    }
	    for (i = 0; i < size; i++)
		box[i] += dx;
	}

	/* copy border */
	for (i = ncols - mid; i < ncols; i++)
	    *cp++ = bufs[mid][i];

	/* write row */
	write(out[id], cell[id], buflen);
    work++;
    }
    G_percent(work, rcount, 2);

    /* copy border rows to output */
    row = starty + mid * dy;
    for (i = 0; i < mid; i++) {
	cp = (DCELL *) Rowio_get(r, row);
	write(out[id], cp, buflen);
	row += dy;
    }

    return 0;
}
