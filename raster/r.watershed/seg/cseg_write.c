#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/segment.h>
#include "Gwater.h"

int cseg_write_cellfile(CSEG * cseg, char *map_name)
{
    int map_fd;
    GW_LARGE_INT row, nrows;
    CELL *buffer;

    map_fd = Rast_open_c_new(map_name);
    nrows = Rast_window_rows();
    buffer = Rast_allocate_c_buf();
    segment_flush(&(cseg->seg));
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 1);
	segment_get_row(&(cseg->seg), buffer, row);
	Rast_put_row(map_fd, buffer, CELL_TYPE);
    }
    G_percent(row, nrows, 1);    /* finish it */
    G_free(buffer);
    Rast_close(map_fd);
    return 0;
}
