#include <unistd.h>
#include <grass/rowio.h>
#include "glob.h"
#include "filter.h"

int execute_filter(ROWIO * r, int out, FILTER * filter, CELL * cell)
{
    int i;
    int count;
    int size;
    int row, rcount;
    int col, ccount;
    int startx, starty;
    int dx, dy;
    int mid;
    CELL **bufs, **box, *cp;
    CELL apply_filter();

    size = filter->size;
    mid = size / 2;
    bufs = (CELL **) G_malloc(size * sizeof(CELL *));
    box = (CELL **) G_malloc(size * sizeof(CELL *));

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
    lseek(out, 0L, 0);

    /* copy border rows to output */
    row = starty;
    for (i = 0; i < mid; i++) {
	cp = (CELL *) rowio_get(r, row);
	write(out, cp, buflen);
	row += dy;
    }

    /* for each row */
    for (count = 0; count < rcount; count++) {
	G_percent(count, rcount, 2);
	row = starty;
	starty += dy;
	/* get "size" rows */
	for (i = 0; i < size; i++) {
	    bufs[i] = (CELL *) rowio_get(r, row);
	    box[i] = bufs[i] + startx;
	    row += dy;
	}
	if (filter->type == SEQUENTIAL)
	    cell = bufs[mid];
	/* copy border */
	cp = cell;
	for (i = 0; i < mid; i++)
	    *cp++ = bufs[mid][i];

	/* filter row */
	col = ccount;
	while (col--) {
	    if (zero_only) {
		if (!box[mid][mid])
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
	write(out, cell, buflen);
    }
    G_percent(count, rcount, 2);

    /* copy border rows to output */
    row = starty + mid * dy;
    for (i = 0; i < mid; i++) {
	cp = (CELL *) rowio_get(r, row);
	write(out, cp, buflen);
	row += dy;
    }

    return 0;
}
