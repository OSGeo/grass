#include <stdio.h>
#include <grass/gis.h>
#include "cmd_line.h"
#include "costHa.h"
#include "local_proto.h"

#define DATA(map, r, c)		(map)[(r) * ncols + (c)]

void ram2out(void)
{
    extern CELL *cell, *x_cell, *y_cell;
    extern CELL *map_x_out, *map_y_out;
    extern float *map_out;
    extern int cum_fd, x_fd, y_fd;
    extern int nrows, ncols;
    extern struct Cell_head window;
    double north, west;
    double G_row_to_northing(), G_col_to_easting();
    int row, col;

    north = G_row_to_northing(0.5, &window);
    west = G_col_to_easting(0.5, &window);
    /*  Copy maps in ram to output maps, casting into integers */
    G_message("Writing output: %s, x_output: %s, y_output: %s ... ",
	      out_layer, x_out_layer, y_out_layer);
    for (row = 0; row < nrows; row++) {
	for (col = 0; col < ncols; col++) {
	    G_percent(row, nrows, 2);
	    *(cell + col) = (int)DATA(map_out, row, col);
	    if (x_out) {
		if (DATA(map_x_out, row, col) == 0)
		    *(x_cell + col) = 0;
		else
		    *(x_cell + col) =
			(int)(west +
			      window.ew_res * DATA(map_x_out, row, col));
	    }
	    if (y_out) {
		if (DATA(map_y_out, row, col) == 0)
		    *(y_cell + col) = 0;
		else
		    *(y_cell + col) =
			(int)(north -
			      window.ns_res * DATA(map_y_out, row, col));
	    }
	}
	G_put_raster_row(cum_fd, cell, CELL_TYPE);
	if (x_out)
	    G_put_raster_row(x_fd, x_cell, CELL_TYPE);
	if (y_out)
	    G_put_raster_row(y_fd, y_cell, CELL_TYPE);
    }
    G_percent(row, nrows, 2);
}
