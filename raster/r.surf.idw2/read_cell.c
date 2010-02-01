#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

int read_cell(char *name)
{
    int fd;
    CELL *cell;
    struct Cell_head window, cellhd;
    int row, col;
    double z, north;

    G_get_window(&window);

    /* Set window to align with input raster map */
    Rast_get_cellhd(name, "", &cellhd);
    Rast_align_window(&window, &cellhd);
    Rast_set_window(&window);

    cell = Rast_allocate_c_buf();

    fd = Rast_open_old(name, "");

    G_message(_("Reading raster map <%s>..."), name);

    north = window.north - window.ns_res / 2.0;
    for (row = 0; row < window.rows; row++) {
	G_percent(row, window.rows, 1);
	north += window.ns_res;
	Rast_get_c_row_nomask(fd, cell, row);
	for (col = 0; col < window.cols; col++)
	    if ((z = cell[col]))
		newpoint(z, window.west + (col + .5) * window.ew_res, north);
    }
    G_percent(row, window.rows, 1);

    Rast_close(fd);
    G_free(cell);

    /* reset the window */
    G_get_window(&window);
    Rast_set_window(&window);

    return 0;
}
