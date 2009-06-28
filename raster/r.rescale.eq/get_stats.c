#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

int get_stats(const char *name, struct Cell_stats *statf)
{
    int fd;
    CELL *cell;
    int row, nrows, ncols;

    fd = Rast_open_cell_old(name, "");
    if (fd < 0)
	exit(1);
    nrows = G_window_rows();
    ncols = G_window_cols();
    cell = Rast_allocate_c_buf();

    Rast_init_cell_stats(statf);
    G_message(_("Reading %s ..."), name);
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	if (Rast_get_map_row(fd, cell, row) < 0)
	    break;
	Rast_update_cell_stats(cell, ncols, statf);
    }
    if (row < nrows)
	exit(1);
    Rast_close(fd);
    G_free(cell);
    G_percent(row, nrows, 2);

    return 0;
}
