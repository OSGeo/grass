#include <grass/config.h>
#include <sys/types.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "tinf.h"
#include "local.h"

/* get the slope between two cells and return a slope direction */
void check(CELL newdir, CELL * dir, void *center, void *edge, double cnst,
	   double *oldslope)
{
    double newslope;

    /* never discharge to a null boundary */
    if (is_null(edge))
	return;

    newslope = slope(center, edge, cnst);
    if (newslope == *oldslope) {
	*dir += newdir;
    }
    else if (newslope > *oldslope) {
	*oldslope = newslope;
	*dir = newdir;
    }

    return;

}

/* determine the flow direction at each cell on one row */
void build_one_row(int i, int nl, int ns, struct band3 *bnd, CELL * dir,
		   struct metrics m)
{
    int j, inc;
    off_t offset;
    CELL sdir;
    double slope;
    char *center;
    char *edge;

    inc = bpe();

    for (j = 0; j < ns; j += 1) {
	offset = j * bpe();
	center = bnd->b[1] + offset;
	if (is_null(center)) {
	    Rast_set_c_null_value(dir + j, 1);
	    continue;
	}

	sdir = 0;
	/* slope=HUGE; */
	slope = HUGE_VAL;
	if (i == 0) {
	    sdir = 128;
	}
	else if (i == nl - 1) {
	    sdir = 8;
	}
	else if (j == 0) {
	    sdir = 32;
	}
	else if (j == ns - 1) {
	    sdir = 2;
	}
	else {
	    /* slope=-HUGE; */
	    slope = -HUGE_VAL;

	    /* check one row back */
	    edge = bnd->b[0] + offset;
	    check(64, &sdir, center, edge - inc, m.diag_res, &slope);
	    check(128, &sdir, center, edge, m.ns_res, &slope);
	    check(1, &sdir, center, edge + inc, m.diag_res, &slope);

	    /* check this row */
	    check(32, &sdir, center, center - inc, m.ew_res, &slope);
	    check(2, &sdir, center, center + inc, m.ew_res, &slope);

	    /* check one row forward */
	    edge = bnd->b[2] + offset;
	    check(16, &sdir, center, edge - inc, m.diag_res, &slope);
	    check(8, &sdir, center, edge, m.ns_res, &slope);
	    check(4, &sdir, center, edge + inc, m.diag_res, &slope);
	}

	if (slope == 0.)
	    sdir = -sdir;
	else if (slope < 0.)
	    sdir = -256;
	dir[j] = sdir;
    }
    return;
}

void filldir(int fe, int fd, int nl, struct band3 *bnd, struct metrics *m)
{
    int i, bufsz;  /* use off_t bufsz for large files ? MM */
    CELL *dir;

    /* determine the flow direction in each cell.  On outer rows and columns
     * the flow direction is always directly out of the map */

    dir = G_calloc(bnd->ns, sizeof(CELL));
    bufsz = bnd->ns * sizeof(CELL);

    lseek(fe, 0, SEEK_SET);
    lseek(fd, 0, SEEK_SET);
    advance_band3(fe, bnd);
    for (i = 0; i < nl; i++) {
        G_percent(i, nl, 5);
	advance_band3(fe, bnd);
	build_one_row(i, nl, bnd->ns, bnd, dir, m[i]);
	write(fd, dir, bufsz);
    }
    G_percent(1, 1, 1);
    advance_band3(fe, bnd);
    build_one_row(nl - 1, nl, bnd->ns, bnd, dir, m[nl - 1]);
    write(fd, dir, bufsz);

    G_free(dir);

    return;
}
