#if defined(_OPENMP)
    #include <omp.h>
#endif

#include <unistd.h>
#include <grass/rowio.h>
#include <grass/raster.h>
#include "glob.h"
#include "filter.h"

int execute_filter(ROWIO *r, int *out, FILTER *filter, DCELL **cell)
{
    int i;
    int t;
    int count;
    int size;
    int row, rcount;
    int col, ccount;
    int startx, starty;
    int dx, dy;
    int mid;
    DCELL ***bufs, ***box, *cp;

    size = filter->size;
    mid = size / 2;
    bufs = (DCELL ***) G_malloc(nprocs * sizeof(DCELL **));
    box = (DCELL ***) G_malloc(nprocs * sizeof(DCELL **));

    for (t = 0; t < nprocs; t++) {
        bufs[t] = (DCELL **) G_malloc(size * sizeof(DCELL *));
        box[t] = (DCELL **) G_malloc(size * sizeof(DCELL *));
    }

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
    lseek(out[MASTER], 0L, SEEK_SET);

    /* copy border rows to output */
    row = starty;
    for (i = 0; i < mid; i++) {
	cp = (DCELL *) Rowio_get(&r[MASTER], row);
	write(out[MASTER], cp, buflen);
	row += dy;
    }

    /* for each row */
    int id = MASTER;
    int start =  0;
    int end = rcount;
    int work = 0;

    #pragma omp parallel firstprivate(starty, id, start, end) private(i, count, row, col, cp) if(nprocs > 1)
    {
    #if defined(_OPENMP)
    if (nprocs > 1) {
        id = omp_get_thread_num();
        start = rcount * id/nprocs;
        end = rcount * (id+1)/nprocs;
        starty += start * dy;
        lseek(out[id], (off_t) ((mid + start) * buflen), SEEK_SET);
    }
    #endif

    for (count = start; count < end; count++) {
	G_percent(work, rcount, 2);
	row = starty;
	starty += dy;
	/* get "size" rows */
	for (i = 0; i < size; i++) {
	    bufs[id][i] = (DCELL *) Rowio_get(&r[id], row);
        /* printf("bufs - %p, id - %d\n", bufs[id][i], id); */
	    box[id][i] = bufs[id][i] + startx;
	    row += dy;
	}
	if (filter->type == SEQUENTIAL)
	    cell[id] = bufs[id][mid];
	/* copy border */
	cp = cell[id];
	for (i = 0; i < mid; i++)
	    *cp++ = bufs[id][mid][i];

	/* filter row */
	col = ccount;
	while (col--) {
	    if (null_only && !Rast_is_d_null_value(&box[id][mid][mid])) {
		    *cp++ = box[id][mid][mid];
	    } else {
            *cp++ = apply_filter(filter, box[id]);
	    }
	    for (i = 0; i < size; i++)
		box[id][i] += dx;
	}

	/* copy border */
	for (i = ncols - mid; i < ncols; i++)
	    *cp++ = bufs[id][mid][i];

	/* write row */
	write(out[id], cell[id], buflen);
    #pragma omp atomic update
    work++;
    }
    }
    G_percent(work, rcount, 2);
    starty += rcount * dy;
    lseek(out[MASTER], 0L, SEEK_END);

    /* copy border rows to output */
    row = starty + mid * dy;
    for (i = 0; i < mid; i++) {
	cp = (DCELL *) Rowio_get(&r[MASTER], row);
	write(out[MASTER], cp, buflen);
	row += dy;
    }

    return 0;
}
