#include <unistd.h>
#include <fcntl.h>
#include <grass/glocale.h>
#include "seg.h"

int bseg_open(BSEG *bseg, int srows, int scols, int nsegs_in_memory)
{
    char *filename;
    int errflag;

    bseg->filename = NULL;
    bseg->fd = -1;
    bseg->name = NULL;
    bseg->mapset = NULL;

    filename = G_tempfile();
    if (0 > (errflag = Segment_open(&(bseg->seg), filename, Rast_window_rows(),
				    Rast_window_cols(), srows, scols,
				    sizeof(char), nsegs_in_memory))) {
	if (errflag == -1) {
	    G_warning(_("File name is invalid"));
	    return -1;
	}
	else if (errflag == -2) {
	    G_warning(_("File write error"));
	    return -2;
	}
	else if (errflag == -3) {
	    G_warning(_("Illegal parameters are passed"));
	    return -3;
	}
	else if (errflag == -4) {
	    G_warning(_("File could not be re-opened"));
	    return -4;
	}
	else if (errflag == -5) {
	    G_warning(_("Prepared file could not be read"));
	    return -5;
	}
	else if (errflag == -6) {
	    G_warning(_("Out of memory"));
	    return -6;
	}
    }


    bseg->filename = filename;

    return 0;
}

int bseg_close(BSEG *bseg)
{
    Segment_close(&(bseg->seg));
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

int bseg_put(BSEG *bseg, char *value, GW_LARGE_INT row, GW_LARGE_INT col)
{
    if (Segment_put(&(bseg->seg), value, row, col) < 0) {
	G_warning(_("Unable to write segment file"));
	return -1;
    }
    return 0;
}

int bseg_put_row(BSEG *bseg, char *value, GW_LARGE_INT row)
{
    if (Segment_put_row(&(bseg->seg), value, row) < 0) {
	G_warning(_("Unable to write segment file"));
	return -1;
    }
    return 0;
}

int bseg_get(BSEG *bseg, char *value, GW_LARGE_INT row, GW_LARGE_INT col)
{
    if (Segment_get(&(bseg->seg), value, row, col) < 0) {
	G_warning(_("Unable to read segment file"));
	return -1;
    }
    return 0;
}


int bseg_read_raster(BSEG *bseg, char *map_name, char *mapset)
{
    int row, rows;
    int col, cols;
    int map_fd;
    CELL *buffer;
    char cbuf;

    bseg->name = NULL;
    bseg->mapset = NULL;

    map_fd = Rast_open_old(map_name, mapset);
    rows = Rast_window_rows();
    cols = Rast_window_cols();
    buffer = Rast_allocate_c_buf();
    for (row = 0; row < rows; row++) {
	Rast_get_c_row(map_fd, buffer, row);
	for (col = cols; col >= 0; col--) {
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
    int row, rows;
    int col, cols;
    CELL *buffer;
    char value;

    map_fd = Rast_open_c_new(map_name);
    rows = Rast_window_rows();
    cols = Rast_window_cols();
    buffer = Rast_allocate_c_buf();
    for (row = 0; row < rows; row++) {
	G_percent(row, rows, 1);
	for (col = 0; col < cols; col++) {
	    bseg_get(bseg, &value, row, col);
	    buffer[col] = value;
	}
	Rast_put_row(map_fd, buffer, CELL_TYPE);
    }
    G_percent(row, rows, 1);    /* finish it */
    G_free(buffer);
    Rast_close(map_fd);
    return 0;
}
