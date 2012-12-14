/*
 * readcell.c - reads an entire cell layer into a buffer
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "global.h"

struct cache *readcell(int fdi, int size, int target_env)
{
    DCELL *tmpbuf;
    struct cache *c;
    int nrows;
    int ncols;
    int row;
    char *filename;
    int nx, ny;
    int nblocks;
    int i;

    if (target_env)
	select_target_env();
    else
	select_current_env();

    nrows = G_window_rows();
    ncols = G_window_cols();

    /* Temporary file must be created in the same location/mapset 
     * where the module was called */
    if (target_env)
	select_current_env();

    ny = (nrows + BDIM - 1) / BDIM;
    nx = (ncols + BDIM - 1) / BDIM;

    if (size > 0)
	nblocks = size * ((1 << 20) / sizeof(block));
    else
	nblocks = (nx + ny) * 2;	/* guess */

    if (nblocks > nx * ny)
	nblocks = nx * ny;

    c = G_malloc(sizeof(struct cache));
    c->stride = nx;
    c->nblocks = nblocks;
    c->grid = (block **) G_calloc(nx * ny, sizeof(block *));
    c->blocks = (block *) G_malloc(nblocks * sizeof(block));
    c->refs = (int *)G_malloc(nblocks * sizeof(int));

    if (nblocks < nx * ny) {
	filename = G_tempfile();
	c->fd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0600);
	if (c->fd < 0)
	    G_fatal_error(_("Unable to open temporary file"));
	remove(filename);
    }
    else
	c->fd = -1;
	
    G_debug(1, "%d of %d blocks in memory", nblocks, nx * ny);

    G_important_message(_("Allocating memory and reading input map..."));
    G_percent(0, nrows, 5);

    for (i = 0; i < c->nblocks; i++)
	c->refs[i] = -1;

    tmpbuf = (DCELL *) G_malloc(nx * sizeof(block));

    if (target_env)
	select_target_env();
    for (row = 0; row < nrows; row += BDIM) {
	int x, y;

	for (y = 0; y < BDIM; y++) {
	    G_percent(row + y, nrows, 5);

	    if (row + y >= nrows)
		break;

	    G_get_d_raster_row(fdi, &tmpbuf[y * nx * BDIM], row + y);
	}

	for (x = 0; x < nx; x++)
	    for (y = 0; y < BDIM; y++)
		if (c->fd >= 0) {
		    if (write
			(c->fd, &tmpbuf[(y * nx + x) * BDIM],
			 BDIM * sizeof(DCELL)) < 0)
			G_fatal_error(_("Error writing segment file"));
		}
		else
		    memcpy(&c->blocks[BKIDX(c, HI(row), x)][LO(y)][0],
			   &tmpbuf[(y * nx + x) * BDIM],
			   BDIM * sizeof(DCELL));
    }

    G_free(tmpbuf);

    if (c->fd < 0)
	for (i = 0; i < c->nblocks; i++) {
	    c->grid[i] = &c->blocks[i];
	    c->refs[i] = i;
	}

    if (target_env)
	select_current_env();

    return c;
}

block *get_block(struct cache * c, int idx)
{
    int replace = rand() % c->nblocks;
    block *p = &c->blocks[replace];
    int ref = c->refs[replace];
    off_t offset = (off_t) idx * sizeof(DCELL) << L2BSIZE;

    if (c->fd < 0)
	G_fatal_error(_("Internal error: cache miss on fully-cached map"));

    if (ref >= 0)
	c->grid[ref] = NULL;

    c->grid[idx] = p;
    c->refs[replace] = idx;

    if (lseek(c->fd, offset, SEEK_SET) < 0)
	G_fatal_error(_("Error seeking on segment file"));

    if (read(c->fd, p, sizeof(block)) < 0)
	G_fatal_error(_("Error reading segment file"));

    return p;
}

void release_cache(struct cache *c)
{
    G_free(c->refs);
    G_free(c->blocks);
    G_free(c->grid);
    
    G_free(c);
    c = NULL;
}
