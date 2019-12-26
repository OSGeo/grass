#include <grass/gis.h>
#include <grass/raster.h>
#include "Gwater.h"

int bseg_write_cellfile(BSEG * bseg, char *map_name)
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
    G_percent(row, rows, 1);	/* finish it */
    G_free(buffer);
    Rast_close(map_fd);
    return 0;
}
