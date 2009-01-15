#include <grass/gis.h>
#include <grass/segment.h>
#include "cseg.h"

static char *me = "dseg_read_cell";

int dseg_read_cell(DSEG * dseg, char *map_name, char *mapset)
{
    int row, nrows, ncols;
    int map_fd;
    char msg[100];
    double *dbuffer;

    dseg->name = NULL;
    dseg->mapset = NULL;

    map_fd = G_open_cell_old(map_name, mapset);
    if (map_fd < 0) {
	sprintf(msg, "%s(): unable to open file [%s] in [%s], %d",
		me, map_name, mapset, map_fd);
	G_warning(msg);
	return -3;
    }
    nrows = G_window_rows();
    ncols = G_window_cols();
    dbuffer = G_allocate_d_raster_buf();
    for (row = 0; row < nrows; row++) {
	if (G_get_d_raster_row(map_fd, dbuffer, row) < 0) {
	    G_free(dbuffer);
	    G_close_cell(map_fd);
	    sprintf(msg, "%s(): unable to read file [%s] in [%s], %d %d",
		    me, map_name, mapset, row, nrows);
	    G_warning(msg);
	    return -2;
	}
	if (segment_put_row(&(dseg->seg), (DCELL *) dbuffer, row) < 0) {
	    G_free(dbuffer);
	    G_close_cell(map_fd);
	    sprintf(msg, "%s(): unable to segment put row for [%s] in [%s]",
		    me, map_name, mapset);
	    G_warning(msg);
	    return (-1);
	}
    }

    G_close_cell(map_fd);
    G_free(dbuffer);

    dseg->name = G_store(map_name);
    dseg->mapset = G_store(mapset);

    return 0;
}
