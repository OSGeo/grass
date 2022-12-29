#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/segment.h>
#include "Gwater.h"

int dseg_write_cellfile(DSEG * dseg, char *map_name)
{
    int map_fd;
    GW_LARGE_INT row, rows;
    double *dbuffer;

    map_fd = Rast_open_new(map_name, DCELL_TYPE);
    rows = Rast_window_rows();
    dbuffer = Rast_allocate_d_buf();
    Segment_flush(&(dseg->seg));
    for (row = 0; row < rows; row++) {
	G_percent(row, rows, 1);
	Segment_get_row(&(dseg->seg), (DCELL *) dbuffer, row);
	Rast_put_row(map_fd, dbuffer, DCELL_TYPE);
    }
    G_percent(row, rows, 1);	/* finish it */
    G_free(dbuffer);
    Rast_close(map_fd);
    return 0;
}
