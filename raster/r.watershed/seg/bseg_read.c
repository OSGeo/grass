#include <grass/gis.h>
#include <grass/raster.h>
#include <unistd.h>
#include "Gwater.h"

int bseg_read_cell(BSEG * bseg, char *map_name, char *mapset)
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
	    cbuf = (char)buffer[col];
	    bseg_put(bseg, &cbuf, row, col);
	}
    }

    Rast_close(map_fd);
    G_free(buffer);

    bseg->name = G_store(map_name);
    bseg->mapset = G_store(mapset);

    return 0;
}
