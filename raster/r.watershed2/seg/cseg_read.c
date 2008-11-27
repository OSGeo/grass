#include <grass/gis.h>
#include <grass/segment.h>
#include "cseg.h"

static char *me = "cseg_read_cell";

int cseg_read_cell(CSEG * cseg, char *map_name, char *mapset)
{
    int row, nrows;
    int map_fd;
    char msg[100];
    CELL *buffer;

    cseg->name = NULL;
    cseg->mapset = NULL;

    map_fd = G_open_cell_old(map_name, mapset);
    if (map_fd < 0) {
	sprintf(msg, "%s(): unable to open file [%s] in [%s], %d",
		me, map_name, mapset, map_fd);
	G_warning(msg);
	return -3;
    }
    nrows = G_window_rows();
    buffer = G_allocate_cell_buf();
    for (row = 0; row < nrows; row++) {
	if (G_get_c_raster_row(map_fd, buffer, row) < 0) {
	    G_free(buffer);
	    G_close_cell(map_fd);
	    sprintf(msg, "%s(): unable to read file [%s] in [%s], %d %d",
		    me, map_name, mapset, row, nrows);
	    G_warning(msg);
	    return -2;
	}
	if (segment_put_row(&(cseg->seg), buffer, row) < 0) {
	    G_free(buffer);
	    G_close_cell(map_fd);
	    sprintf(msg, "%s(): unable to segment put row for [%s] in [%s]",
		    me, map_name, mapset);
	    G_warning(msg);
	    return (-1);
	}
    }

    G_close_cell(map_fd);
    G_free(buffer);

    cseg->name = G_store(map_name);
    cseg->mapset = G_store(mapset);

    return 0;
}
