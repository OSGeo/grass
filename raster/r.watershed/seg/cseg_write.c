#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/segment.h>
#include "Gwater.h"

int cseg_write_cellfile(CSEG * cseg, char *map_name)
{
    int map_fd;
    GW_LARGE_INT row, rows;
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
    G_percent(row, rows, 1);	/* finish it */
    G_free(buffer);
    Rast_close(map_fd);
    return 0;
}
