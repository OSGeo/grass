#include <grass/gis.h>
#include <grass/raster.h>
#include <unistd.h>
#include "Gwater.h"

int bseg_read_cell(BSEG * bseg, char *map_name, char *mapset)
{
    int row, nrows;
    int col, ncols;
    int map_fd;
    CELL *buffer;
    char cbuf;

    bseg->name = NULL;
    bseg->mapset = NULL;

    map_fd = Rast_open_old(map_name, mapset);
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    buffer = Rast_allocate_c_buf();
    for (row = 0; row < nrows; row++) {
	Rast_get_c_row(map_fd, buffer, row);
	for (col = ncols; col >= 0; col--) {
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
