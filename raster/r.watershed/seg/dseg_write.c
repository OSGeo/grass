#include <grass/gis.h>
#include <grass/segment.h>
#include "cseg.h"

static char *me = "dseg_write_cell";

int dseg_write_cellfile(DSEG * dseg, char *map_name)
{
    int map_fd;
    int row, nrows, ncols;
    double *dbuffer;

    map_fd = G_open_raster_new(map_name, DCELL_TYPE);
    if (map_fd < 0) {
	G_warning("%s(): unable to open new map layer [%s]", me, map_name);
	return -1;
    }
    nrows = G_window_rows();
    ncols = G_window_cols();
    dbuffer = G_allocate_d_raster_buf();
    segment_flush(&(dseg->seg));
    for (row = 0; row < nrows; row++) {
	segment_get_row(&(dseg->seg), (DCELL *) dbuffer, row);
	if (G_put_raster_row(map_fd, dbuffer, DCELL_TYPE) < 0) {
	    G_free(dbuffer);
	    G_unopen_cell(map_fd);
	    G_warning("%s(): unable to write new map layer [%s], row %d",
		      me, map_name, row);
	    return -2;
	}
    }
    G_free(dbuffer);
    G_close_cell(map_fd);
    return 0;
}
