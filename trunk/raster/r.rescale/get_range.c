#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

int get_range(const char *name, long *min, long *max)
{
    struct Range range;
    int nrows, ncols, row, col;
    CELL *cell;
    int fd;
    CELL cmin, cmax;
    struct Cell_head cellhd;

    if (Rast_read_range(name, "", &range) < 0) {
	Rast_init_range(&range);	/* read the file to get the range */
	Rast_get_cellhd(name, "", &cellhd);
	Rast_set_window(&cellhd);
	cell = Rast_allocate_c_buf();
	fd = Rast_open_old(name, "");
	nrows = Rast_window_rows();
	ncols = Rast_window_cols();
	G_message(_("Reading %s ..."), name);
	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);
	    Rast_get_c_row_nomask(fd, cell, row);
	    for (col = 0; col < ncols; col++)
		Rast_update_range(cell[col], &range);
	}
	G_percent(row, nrows, 2);
	Rast_close(fd);
	G_free(cell);
    }

    Rast_get_range_min_max(&range, &cmin, &cmax);
    *min = cmin;
    *max = cmax;

    return 0;
}
