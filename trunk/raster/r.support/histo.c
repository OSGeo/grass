#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

/* 
 * do_histogram() - Creates histogram for CELL
 *
 * RETURN: EXIT_SUCCESS / EXIT_FAILURE
 */
int do_histogram(const char *name)
{
    CELL *cell;
    struct Cell_head cellhd;
    struct Cell_stats statf;
    int nrows, ncols;
    int row;
    int fd;

    Rast_get_cellhd(name, "", &cellhd);

    Rast_set_window(&cellhd);
    fd = Rast_open_old(name, "");

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    cell = Rast_allocate_c_buf();

    Rast_init_cell_stats(&statf);
    for (row = 0; row < nrows; row++) {
	Rast_get_c_row_nomask(fd, cell, row);
	Rast_update_cell_stats(cell, ncols, &statf);
    }

    if (row == nrows)
	Rast_write_histogram_cs(name, &statf);

    Rast_free_cell_stats(&statf);
    Rast_close(fd);
    G_free(cell);

    if (row < nrows)
	return -1;

    return 0;
}
