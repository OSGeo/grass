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
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "r.proj.h"

struct cache *readcell(int fdi, const char *size)
{
    FCELL *tmpbuf;
    struct cache *c;
    int nrows;
    int ncols;
    int row;
    int nx, ny;
    int nblocks;
    int i;

    nrows = Rast_input_window_rows();
    ncols = Rast_input_window_cols();

    ny = (nrows + BDIM - 1) / BDIM;
    nx = (ncols + BDIM - 1) / BDIM;

    if (size)
	nblocks = atoi(size) * ((1 << 20) / sizeof(block));
    else
	nblocks = (nx + ny) * 2;	/* guess */

    if (nblocks > nx * ny)
	nblocks = nx * ny;

    c = G_malloc(sizeof(struct cache));
    c->stride = nx;
    c->nblocks = nblocks;
    c->grid = (block **) G_calloc(nx * ny, sizeof(block *));
    c->blocks = (block *) G_malloc(nblocks * sizeof(block));
    c->refs = (int *)G_calloc(nblocks, sizeof(int));

    if (nblocks < nx * ny) {
	/* Temporary file must be created in output location */
	G__switch_env();
	c->fname = G_tempfile();
	G__switch_env();
	c->fd = open(c->fname, O_RDWR | O_CREAT | O_EXCL, 0600);
	if (c->fd < 0)
	    G_fatal_error(_("Unable to open temporary file"));
    }
    else {
	c->fd = -1;
	c->fname = NULL;
    }
    G_message("%.2f percent are kept in memory", 100.0 * nblocks / (nx * ny));

    G_important_message(_("Allocating memory and reading input map..."));
    G_percent(0, nrows, 5);

    for (i = 0; i < c->nblocks; i++)
	c->refs[i] = -1;

    tmpbuf = (FCELL *) G_malloc(nx * sizeof(block));

    for (row = 0; row < nrows; row += BDIM) {
	int x, y;

	for (y = 0; y < BDIM; y++) {
	    G_percent(row + y, nrows, 5);

	    if (row + y >= nrows)
		break;

	    Rast_get_f_row(fdi, &tmpbuf[y * nx * BDIM], row + y);
	}

	for (x = 0; x < nx; x++)
	    for (y = 0; y < BDIM; y++)
		if (c->fd >= 0) {
		    if (write
			(c->fd, &tmpbuf[(y * nx + x) * BDIM],
			 BDIM * sizeof(FCELL)) < 0)
			G_fatal_error(_("Error writing segment file"));
		}
		else
		    memcpy(&c->blocks[BKIDX(c, HI(row), x)][LO(y)][0],
			   &tmpbuf[(y * nx + x) * BDIM],
			   BDIM * sizeof(FCELL));
    }

    G_free(tmpbuf);

    if (c->fd < 0) {
	for (i = 0; i < c->nblocks; i++) {
	    c->grid[i] = &c->blocks[i];
	    c->refs[i] = i;
	}
    }
    else {
	close(c->fd);
	c->fd = -1;
    }

    return c;
}

block *get_block(struct cache * c, int idx)
{
    int fd;
    int replace = rand() % c->nblocks;
    block *p = &c->blocks[replace];
    int ref = c->refs[replace];
    off_t offset = (off_t) idx * sizeof(FCELL) << L2BSIZE;

    if (c->fname == NULL)
	G_fatal_error(_("Internal error: cache miss on fully-cached map"));

    fd = open(c->fname, O_RDONLY, 0600);
    if (fd < 0)
	G_fatal_error(_("Unable to open temporary file"));

    if (ref >= 0)
	c->grid[ref] = NULL;

    c->grid[idx] = p;
    c->refs[replace] = idx;

    if (lseek(fd, offset, SEEK_SET) < 0)
	G_fatal_error(_("Error seeking on segment file"));

    if (read(fd, p, sizeof(block)) < 0)
	G_fatal_error(_("Error reading segment file"));
	
    close(fd);

    return p;
}

void release_cache(struct cache *c)
{
    G_free(c->grid);
    G_free(c->blocks);
    G_free(c->refs);
    remove(c->fname);
    G_free(c);
}
