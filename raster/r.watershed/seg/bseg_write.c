#include <grass/gis.h>
#include <grass/raster.h>
#include "Gwater.h"

int bseg_write_cellfile(BSEG * bseg, char *map_name)
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
