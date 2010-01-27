#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

int zoom(struct Cell_head *window, const char *name, const char *mapset)
{
    int fd;
    void *raster, *rast_ptr;
    RASTER_MAP_TYPE map_type;
    int row, col;
    int nrows, ncols;
    int top, bottom, left, right, mark;
    double north, south, east, west;

    G_adjust_Cell_head3(window, 0, 0, 0);
    Rast_set_window(window);
    nrows = window->rows;
    ncols = window->cols;

    fd = Rast_open_old(name, mapset);
    map_type = Rast_get_map_type(fd);
    raster = Rast_allocate_buf(map_type);

    /* find first non-null row */
    top = nrows;
    bottom = -1;
    left = ncols;
    right = -1;
    for (row = 0; row < nrows; row++) {
	Rast_get_row(fd, rast_ptr = raster, row, map_type);
	for (col = 0; col < ncols; col++) {
	    if (!Rast_is_null_value(rast_ptr, map_type))
		break;
	    rast_ptr = G_incr_void_ptr(rast_ptr, Rast_cell_size(map_type));
	}
	if (col == ncols)
	    continue;
	if (row < top)
	    top = row;
	if (row > bottom)
	    bottom = row;
	if (col < left)
	    left = col;
	for (mark = col; col < ncols; col++) {
	    if (!Rast_is_null_value(rast_ptr, map_type))
		mark = col;
	    rast_ptr = G_incr_void_ptr(rast_ptr, Rast_cell_size(map_type));
	}
	if (mark > right)
	    right = mark;
    }
    Rast_close(fd);
    G_free(raster);

    /* no data everywhere? */
    if (bottom < 0)
	return 0;

    north = window->north - top * window->ns_res;
    south = window->north - (bottom + 1) * window->ns_res;
    west = window->west + left * window->ew_res;
    east = window->west + (right + 1) * window->ew_res;

    window->north = north;
    window->south = south;
    window->east = east;
    window->west = west;

    return 1;
}
