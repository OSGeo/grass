#include <string.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "what.h"
#include "local_proto.h"

int what(int once, int terse, int colrow, char *fs, int width, int mwidth)
{
    int i;
    int row, col;
    int nrows, ncols;
    CELL *buf, null_cell;
    DCELL *dbuf, null_dcell;
    struct Cell_head window;
    int screen_x, screen_y;
    double east, north;
    int button;
    RASTER_MAP_TYPE *map_type;

    map_type = (RASTER_MAP_TYPE *) G_malloc(nrasts * sizeof(RASTER_MAP_TYPE));

    G_get_set_window(&window);
    nrows = window.rows;
    ncols = window.cols;
    buf = G_allocate_c_raster_buf();
    dbuf = G_allocate_d_raster_buf();

    screen_x = ((int)D_get_d_west() + (int)D_get_d_east()) / 2;
    screen_y = ((int)D_get_d_north() + (int)D_get_d_south()) / 2;

    for (i = 0; i < nrasts; i++)
	map_type[i] = G_get_raster_map_type(fd[i]);

    do {
	if (!terse)
	    show_buttons(once);
	R_get_location_with_pointer(&screen_x, &screen_y, &button);
	if (!once) {
	    if (button == 2)
		continue;
	    if (button == 3)
		break;
	}

	east = D_d_to_u_col(screen_x + 0.5);
	north = D_d_to_u_row(screen_y + 0.5);
	col = D_d_to_a_col(screen_x + 0.5);
	row = D_d_to_a_row(screen_y + 0.5);

	show_utm(name[0], mapset[0], north, east, &window, terse, colrow,
		 button, fs);
	G_set_c_null_value(&null_cell, 1);
	G_set_d_null_value(&null_dcell, 1);
	for (i = 0; i < nrasts; i++) {
	    if (row < 0 || row >= nrows || col < 0 || col >= ncols) {
		G_message(_("You are clicking outside the map"));
		continue;
	    }
	    if (G_get_c_raster_row(fd[i], buf, row) < 0)
		show_cat(width, mwidth, name[i], mapset[i], null_cell,
			 "ERROR reading raster map", terse, fs, map_type[i]);
	    else if (map_type[i] == CELL_TYPE) {
		show_cat(width, mwidth, name[i], mapset[i], buf[col],
			 G_get_c_raster_cat(&buf[col], &cats[i]), terse, fs,
			 map_type[i]);
		continue;
	    }
	    else {		/* fp map */

		show_cat(width, mwidth, name[i], mapset[i], buf[col],
			 "", terse, fs, map_type[i]);
	    }

	    if (map_type[i] == CELL_TYPE)
		continue;

	    if (G_get_d_raster_row(fd[i], dbuf, row) < 0)
		show_dval(width, mwidth, name[i], mapset[i], null_dcell,
			  "ERROR reading fcell file", terse, fs);
	    else
		show_dval(width, mwidth, name[i], mapset[i], dbuf[col],
			  G_get_d_raster_cat(&dbuf[col], &cats[i]), terse,
			  fs);
	}
    }
    while (!once);

    return 0;
}
