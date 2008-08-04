#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

int get_range(char *name, char *mapset, long *min, long *max)
{
    struct Range range;
    int nrows, ncols, row, col;
    CELL *cell;
    int fd;
    CELL cmin, cmax;
    struct Cell_head cellhd;

    if (G_read_range(name, mapset, &range) < 0) {
	G_init_range(&range);	/* read the file to get the range */
	G_get_cellhd(name, mapset, &cellhd);
	G_set_window(&cellhd);
	cell = G_allocate_cell_buf();
	fd = G_open_cell_old(name, mapset);
	if (fd < 0)
	    exit(1);
	nrows = G_window_rows();
	ncols = G_window_cols();
	G_message(_("Reading %s ..."), name);
	for (row = 0; row < nrows; row++) {
	    G_percent(row, nrows, 2);
	    if (G_get_map_row_nomask(fd, cell, row) < 0)
		exit(1);
	    for (col = 0; col < ncols; col++)
		G_update_range(cell[col], &range);
	}
	G_percent(row, nrows, 2);
	G_close_cell(fd);
	G_free(cell);
    }

    G_get_range_min_max(&range, &cmin, &cmax);
    *min = cmin;
    *max = cmax;

    return 0;
}
