#include <string.h>
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
    buf = Rast_allocate_c_buf();
    dbuf = Rast_allocate_d_buf();

    screen_x = ((int)D_get_d_west() + (int)D_get_d_east()) / 2;
    screen_y = ((int)D_get_d_north() + (int)D_get_d_south()) / 2;

    for (i = 0; i < nrasts; i++)
	map_type[i] = Rast_get_map_type(fd[i]);

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
	Rast_set_c_null_value(&null_cell, 1);
	Rast_set_d_null_value(&null_dcell, 1);
	for (i = 0; i < nrasts; i++) {
	    if (row < 0 || row >= nrows || col < 0 || col >= ncols) {
		G_message(_("You are clicking outside the map"));
		continue;
	    }
	    Rast_get_c_row(fd[i], buf, row);
	    if (map_type[i] == CELL_TYPE) {
		show_cat(width, mwidth, name[i], mapset[i], buf[col],
			 Rast_get_c_cat(&buf[col], &cats[i]), terse, fs,
			 map_type[i]);
		continue;
	    }
	    else {		/* fp map */
		show_cat(width, mwidth, name[i], mapset[i], buf[col],
			 "", terse, fs, map_type[i]);
	    }

	    if (map_type[i] == CELL_TYPE)
		continue;

	    Rast_get_d_row(fd[i], dbuf, row);

	    show_dval(width, mwidth, name[i], mapset[i], dbuf[col],
		      Rast_get_d_cat(&dbuf[col], &cats[i]), terse,
		      fs, map_type[i]);
	}
    }
    while (!once);

    return 0;
}
