#include <unistd.h>
#include <fcntl.h>
#include <grass/glocale.h>
#include "seg.h"

int cseg_open(CSEG *cseg, int srows, int scols, int nsegs_in_memory)
{
    char *filename;
    int errflag;
    int fd;

    cseg->filename = NULL;
    cseg->fd = -1;
    cseg->name = NULL;
    cseg->mapset = NULL;

    filename = G_tempfile();
    if (-1 == (fd = creat(filename, 0666))) {
	G_warning(_("cseg_open(): unable to create segment file"));
	return -2;
    }
    if (0 >
	(errflag =
	 segment_format(fd, Rast_window_rows(), Rast_window_cols(), srows, scols,
			sizeof(CELL)))) {
	close(fd);
	unlink(filename);
	if (errflag == -1) {
	    G_warning(_("cseg_open(): could not write segment file"));
	    return -1;
	}
	else {
	    G_warning(_("cseg_open(): illegal configuration parameter(s)"));
	    return -3;
	}
    }
    close(fd);
    if (-1 == (fd = open(filename, 2))) {
	unlink(filename);
	G_warning(_("cseg_open(): unable to re-open segment file"));
	return -4;
    }
    if (0 > (errflag = segment_init(&(cseg->seg), fd, nsegs_in_memory))) {
	close(fd);
	unlink(filename);
	if (errflag == -1) {
	    G_warning(_("cseg_open(): could not read segment file"));
	    return -5;
	}
	else {
	    G_warning(_("cseg_open(): out of memory"));
	    return -6;
	}
    }
    cseg->filename = filename;
    cseg->fd = fd;
    return 0;
}

int cseg_close(CSEG *cseg)
{
    segment_release(&(cseg->seg));
    close(cseg->fd);
    unlink(cseg->filename);
    if (cseg->name) {
	G_free(cseg->name);
	cseg->name = NULL;
    }
    if (cseg->mapset) {
	G_free(cseg->mapset);
	cseg->mapset = NULL;
    }
    return 0;
}

int cseg_put(CSEG *cseg, CELL *value, int row, int col)
{
    if (segment_put(&(cseg->seg), value, row, col) < 0) {
	G_warning(_("cseg_put(): could not write segment file"));
	return -1;
    }
    return 0;
}

int cseg_put_row(CSEG *cseg, CELL *value, int row)
{
    if (segment_put_row(&(cseg->seg), value, row) < 0) {
	G_warning(_("cseg_put_row(): could not write segment file"));
	return -1;
    }
    return 0;
}

int cseg_get(CSEG *cseg, CELL *value, int row, int col)
{
    if (segment_get(&(cseg->seg), value, row, col) < 0) {
	G_warning(_("cseg_get(): could not read segment file"));
	return -1;
    }
    return 0;
}

int cseg_read_raster(CSEG *cseg, char *map_name, char *mapset)
{
    int row, nrows;
    int map_fd;
    CELL *buffer;

    cseg->name = NULL;
    cseg->mapset = NULL;

    map_fd = Rast_open_old(map_name, mapset);
    nrows = Rast_window_rows();
    buffer = Rast_allocate_c_buf();
    for (row = 0; row < nrows; row++) {
	Rast_get_c_row(map_fd, buffer, row);
	if (segment_put_row(&(cseg->seg), buffer, row) < 0) {
	    G_free(buffer);
	    Rast_close(map_fd);
	    G_warning(_("cseg_read_cell(): unable to segment put row for <%s> in <%s>"),
		    map_name, mapset);
	    return (-1);
	}
    }

    Rast_close(map_fd);
    G_free(buffer);

    cseg->name = G_store(map_name);
    cseg->mapset = G_store(mapset);

    return 0;
}

int cseg_write_raster(CSEG *cseg, char *map_name)
{
    int map_fd;
    int row, nrows;
    CELL *buffer;

    map_fd = Rast_open_c_new(map_name);
    nrows = Rast_window_rows();
    buffer = Rast_allocate_c_buf();
    segment_flush(&(cseg->seg));
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 1);
	segment_get_row(&(cseg->seg), buffer, row);
	Rast_put_row(map_fd, buffer, CELL_TYPE);
    }
    G_percent(row, nrows, 1);    /* finish it */
    G_free(buffer);
    Rast_close(map_fd);
    return 0;
}
