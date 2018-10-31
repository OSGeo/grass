#include <unistd.h>
#include <fcntl.h>
#include <grass/glocale.h>
#include "seg.h"

int cseg_open(CSEG *cseg, int srows, int scols, int nsegs_in_memory)
{
    char *filename;
    int errflag;

    cseg->filename = NULL;
    cseg->fd = -1;
    cseg->name = NULL;
    cseg->mapset = NULL;

    filename = G_tempfile();
    if (0 > (errflag = Segment_open(&(cseg->seg), filename, Rast_window_rows(),
				    Rast_window_cols(), srows, scols,
				    sizeof(CELL), nsegs_in_memory))) {
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

    cseg->filename = filename;

    return 0;
}

int cseg_close(CSEG *cseg)
{
    Segment_close(&(cseg->seg));
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

int cseg_put(CSEG *cseg, CELL *value, GW_LARGE_INT row, GW_LARGE_INT col)
{
    if (Segment_put(&(cseg->seg), value, row, col) < 0) {
	G_warning(_("Unable to write segment file"));
	return -1;
    }
    return 0;
}

int cseg_put_row(CSEG *cseg, CELL *value, GW_LARGE_INT row)
{
    if (Segment_put_row(&(cseg->seg), value, row) < 0) {
	G_warning(_("Unable to write segment file"));
	return -1;
    }
    return 0;
}

int cseg_get(CSEG *cseg, CELL *value, GW_LARGE_INT row, GW_LARGE_INT col)
{
    if (Segment_get(&(cseg->seg), value, row, col) < 0) {
	G_warning(_("Unable to read segment file"));
	return -1;
    }
    return 0;
}

int cseg_read_raster(CSEG *cseg, char *map_name, char *mapset)
{
    int row, rows;
    int map_fd;
    CELL *buffer;

    cseg->name = NULL;
    cseg->mapset = NULL;

    map_fd = Rast_open_old(map_name, mapset);
    rows = Rast_window_rows();
    buffer = Rast_allocate_c_buf();
    for (row = 0; row < rows; row++) {
	Rast_get_c_row(map_fd, buffer, row);
	if (Segment_put_row(&(cseg->seg), buffer, row) < 0) {
	    G_free(buffer);
	    Rast_close(map_fd);
	    G_warning(_("Unable to segment put row %d for raster map <%s>"),
                      row, map_name);
	    return -1;
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
    int row, rows;
    CELL *buffer;

    map_fd = Rast_open_c_new(map_name);
    rows = Rast_window_rows();
    buffer = Rast_allocate_c_buf();
    Segment_flush(&(cseg->seg));
    for (row = 0; row < rows; row++) {
	G_percent(row, rows, 1);
	Segment_get_row(&(cseg->seg), buffer, row);
	Rast_put_row(map_fd, buffer, CELL_TYPE);
    }
    G_percent(row, rows, 1);    /* finish it */
    G_free(buffer);
    Rast_close(map_fd);
    return 0;
}
