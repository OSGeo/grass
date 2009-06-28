#include <grass/gis.h>
#include <grass/raster.h>
#include "cseg.h"

static char *me = "bseg_write_cell";

int bseg_write_cellfile(BSEG * bseg, char *map_name)
{
    int map_fd;
    int row, nrows;
    int col, ncols;
    CELL *buffer;
    CELL value;

    map_fd = Rast_open_cell_new(map_name);
    if (map_fd < 0) {
	G_warning("%s(): unable to open new map layer [%s]", me, map_name);
	return -1;
    }
    nrows = G_window_rows();
    ncols = G_window_cols();
    buffer = Rast_allocate_c_buf();
    for (row = 0; row < nrows; row++) {
	for (col = 0; col < ncols; col++) {
	    bseg_get(bseg, &value, row, col);
	    buffer[col] = value;
	}
	if (Rast_put_raster_row(map_fd, buffer, CELL_TYPE) < 0) {
	    G_free(buffer);
	    Rast_unopen(map_fd);
	    G_warning("%s(): unable to write new map layer [%s], row %d",
		      me, map_name, row);
	    return -2;
	}
    }
    G_free(buffer);
    Rast_close(map_fd);
    return 0;
}
