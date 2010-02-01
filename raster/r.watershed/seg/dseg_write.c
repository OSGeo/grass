#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/segment.h>
#include "cseg.h"

int dseg_write_cellfile(DSEG * dseg, char *map_name)
{
    int map_fd;
    int row, nrows, ncols;
    double *dbuffer;

    map_fd = Rast_open_new(map_name, DCELL_TYPE);
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    dbuffer = Rast_allocate_d_buf();
    segment_flush(&(dseg->seg));
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 1);
	segment_get_row(&(dseg->seg), (DCELL *) dbuffer, row);
	Rast_put_row(map_fd, dbuffer, DCELL_TYPE);
    }
    G_percent(row, nrows, 1);    /* finish it */
    G_free(dbuffer);
    Rast_close(map_fd);
    return 0;
}
