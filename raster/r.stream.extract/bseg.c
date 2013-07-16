#include <unistd.h>
#include <fcntl.h>
#include <grass/glocale.h>
#include "seg.h"

int bseg_open(BSEG *bseg, int srows, int scols, int nsegs_in_memory)
{
    char *filename;
    int errflag;
    int fd;

    bseg->filename = NULL;
    bseg->fd = -1;
    bseg->name = NULL;
    bseg->mapset = NULL;

    filename = G_tempfile();
    if (-1 == (fd = creat(filename, 0666))) {
	G_warning(_("bseg_open(): unable to create segment file"));
	return -2;
    }
    if (0 > (errflag = segment_format(fd, Rast_window_rows(),
				      Rast_window_cols(), srows, scols,
				      sizeof(char)))) {
	close(fd);
	unlink(filename);
	if (errflag == -1) {
	    G_warning(_("bseg_open(): could not write segment file"));
	    return -1;
	}
	else {
	    G_warning(_("bseg_open(): illegal configuration parameter(s)"));
	    return -3;
	}
    }
    close(fd);
    if (-1 == (fd = open(filename, 2))) {
	unlink(filename);
	G_warning(_("bseg_open(): unable to re-open segment file"));
	return -4;
    }
    if (0 > (errflag = segment_init(&(bseg->seg), fd, nsegs_in_memory))) {
	close(fd);
	unlink(filename);
	if (errflag == -1) {
	    G_warning(_("bseg_open(): could not read segment file"));
	    return -5;
	}
	else {
	    G_warning(_("bseg_open(): out of memory"));
	    return -6;
	}
    }
    bseg->filename = filename;
    bseg->fd = fd;
    return 0;
}

int bseg_close(BSEG *bseg)
{
    segment_release(&(bseg->seg));
    close(bseg->fd);
    unlink(bseg->filename);
    if (bseg->name) {
	G_free(bseg->name);
	bseg->name = NULL;
    }
    if (bseg->mapset) {
	G_free(bseg->mapset);
	bseg->mapset = NULL;
    }
    return 0;
}

int bseg_put(BSEG *bseg, char *value, int row, int col)
{
    if (segment_put(&(bseg->seg), value, row, col) < 0) {
	G_warning(_("bseg_put(): could not write segment file"));
	return -1;
    }
    return 0;
}

int bseg_put_row(BSEG *bseg, char *value, int row)
{
    if (segment_put_row(&(bseg->seg), value, row) < 0) {
	G_warning(_("bseg_put_row(): could not write segment file"));
	return -1;
    }
    return 0;
}

int bseg_get(BSEG *bseg, char *value, int row, int col)
{
    if (segment_get(&(bseg->seg), value, row, col) < 0) {
	G_warning(_("bseg_get(): could not read segment file"));
	return -1;
    }
    return 0;
}


int bseg_read_raster(BSEG *bseg, char *map_name, char *mapset)
{
    int row, nrows;
    int col, ncols;
    int map_fd;
    CELL *buffer;
    char cbuf;

    bseg->name = NULL;
    bseg->mapset = NULL;

    map_fd = Rast_open_old(map_name, mapset);
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    buffer = Rast_allocate_c_buf();
    for (row = 0; row < nrows; row++) {
	Rast_get_c_row(map_fd, buffer, row);
	for (col = ncols; col >= 0; col--) {
	    cbuf = (char) buffer[col];
	    bseg_put(bseg, &cbuf, row, col);
	}
    }

    Rast_close(map_fd);
    G_free(buffer);

    bseg->name = G_store(map_name);
    bseg->mapset = G_store(mapset);

    return 0;
}

int bseg_write_raster(BSEG *bseg, char *map_name)
{
    int map_fd;
    int row, nrows;
    int col, ncols;
    CELL *buffer;
    char value;

    map_fd = Rast_open_c_new(map_name);
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    buffer = Rast_allocate_c_buf();
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 1);
	for (col = 0; col < ncols; col++) {
	    bseg_get(bseg, &value, row, col);
	    buffer[col] = value;
	}
	Rast_put_row(map_fd, buffer, CELL_TYPE);
    }
    G_percent(row, nrows, 1);    /* finish it */
    G_free(buffer);
    Rast_close(map_fd);
    return 0;
}
