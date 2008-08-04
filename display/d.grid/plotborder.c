#include <math.h>
#include <grass/gis.h>
#include <grass/display.h>

int plot_border(double grid_size, double east, double north)
{
    double x, y;
    struct Cell_head window;
    double i, steps, loop, longmark, middlemark, shortmark;
    double row_dist, colm_dist;

    G_get_set_window(&window);

    /* pull right and bottom edges back one pixel; display lib bug? */
    row_dist = D_d_to_u_row(0.) - D_d_to_u_row(1.);
    colm_dist = D_d_to_u_col(1.) - D_d_to_u_col(0.);
    window.south = window.south + row_dist;
    window.east = window.east - colm_dist;

    G_setup_plot(D_get_d_north(), D_get_d_south(), D_get_d_west(),
		 D_get_d_east(), D_move_abs, D_cont_abs);

    steps = grid_size / 10.;	/* tick marks number */
    shortmark = 180.;		/* tick marks length */
    middlemark = 90.;
    longmark = 45.;

    /* plot boundary lines: */

    /* horizontal : */
    G_plot_line(window.west, window.south, window.east, window.south);
    G_plot_line(window.west, window.north, window.east, window.north);

    /* vertical : */
    G_plot_line(window.west, window.south, window.west, window.north);
    G_plot_line(window.east, window.south, window.east, window.north);

    /* Draw vertical border marks */
    if (window.west < east)
	x = floor((window.west - east) / grid_size) * grid_size + east;
    else
	x = east - ceil((east - window.west) / grid_size) * grid_size;

    while (x <= window.east) {
	loop = 0;
	for (i = 0; i <= grid_size; i = i + steps) {
	    if (loop == 0) {
		G_plot_line(x + i,
			    window.south + (window.north -
					    window.south) / longmark, x + i,
			    window.south);
		G_plot_line(x + i, window.north, x + i,
			    window.north - (window.north -
					    window.south) / longmark);
	    }
	    if (loop == 5) {
		G_plot_line(x + i,
			    window.south + (window.north -
					    window.south) / middlemark, x + i,
			    window.south);
		G_plot_line(x + i, window.north, x + i,
			    window.north - (window.north -
					    window.south) / middlemark);
	    }
	    else {
		G_plot_line(x + i,
			    window.south + (window.north -
					    window.south) / shortmark, x + i,
			    window.south);
		G_plot_line(x + i, window.north, x + i,
			    window.north - (window.north -
					    window.south) / shortmark);
	    }
	    loop++;
	}
	x += grid_size;
    }

    /* Draw horizontal border marks */

    if (window.south > north)
	y = floor((window.south - north) / grid_size) * grid_size + north;
    else
	y = north - ceil((north - window.south) / grid_size) * grid_size;

    while (y <= window.north) {
	loop = 0;
	for (i = 0; i <= grid_size; i = i + steps) {
	    if (loop == 0) {
		G_plot_line(window.west, y + i,
			    window.west + (window.east -
					   window.west) / longmark, y + i);
		G_plot_line(window.east -
			    (window.east - window.west) / longmark, y + i,
			    window.east, y + i);
	    }
	    if (loop == 5) {
		G_plot_line(window.west, y + i,
			    window.west + (window.east -
					   window.west) / middlemark, y + i);
		G_plot_line(window.east -
			    (window.east - window.west) / middlemark, y + i,
			    window.east, y + i);
	    }
	    else {
		G_plot_line(window.west, y + i,
			    window.west + (window.east -
					   window.west) / shortmark, y + i);
		G_plot_line(window.east -
			    (window.east - window.west) / shortmark, y + i,
			    window.east, y + i);
	    }
	    loop++;
	}
	y += grid_size;
    }

    return 0;
}
