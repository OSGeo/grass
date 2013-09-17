#include <math.h>
#include <grass/gis.h>
#include <grass/display.h>
#include "local_proto.h"

int plot_border(double grid_size, double east, double north, int direction)
{
    double x, y;
    struct Cell_head window;
    double i, steps, loop, longmark, middlemark, shortmark;
    double row_dist, colm_dist;

    G_get_set_window(&window);

    /* FIXME: pull right and bottom edges back one pixel; display lib bug? */
    row_dist = D_d_to_u_row(0.) - D_d_to_u_row(1.);
    colm_dist = D_d_to_u_col(1.) - D_d_to_u_col(0.);
/*     window.south += row_dist;
       window.east -= colm_dist;
 */

    steps = grid_size / 10.;	/* tick marks number */
    shortmark = 180.;		/* tick marks length */
    middlemark = 90.;
    longmark = 45.;


    /* plot boundary lines: */
    /* horizontal : */
    D_line_abs(window.west, window.south, window.east + colm_dist, window.south); /* display lib bug? */
    D_line_abs(window.west, window.north, window.east, window.north);

    /* vertical : */
    D_line_abs(window.west, window.south, window.west, window.north);
    D_line_abs(window.east, window.south, window.east, window.north);


    /* Draw ticks along top and bottom borders */
    if (window.west < east)
	x = floor((window.west - east) / grid_size) * grid_size + east;
    else
	x = east - ceil((east - window.west) / grid_size) * grid_size;

    if (direction != DIRN_LAT) {
	while (x <= window.east) {
	    loop = 0;

	    for (i = 0; i <= grid_size; i = i + steps) {
		if (x + i < window.west || x + i > window.east) {
		    loop++;
		    continue;
		}

		if (loop == 0) {
		    D_line_abs(x + i,
			      window.south + (window.north - window.south) / longmark, x + i,
			      window.south);
		    D_line_abs(x + i, window.north, x + i,
			       window.north - row_dist - (window.north - window.south) / longmark);
		}
		if (loop == 5) {
		    D_line_abs(x + i,
			       window.south + (window.north - window.south) / middlemark, x + i,
			       window.south);
		    D_line_abs(x + i, window.north, 
			       x + i, window.north - row_dist - (window.north - window.south) / middlemark);
		}
		else {
		    D_line_abs(x + i,
			       window.south + (window.north - window.south) / shortmark, x + i,
			       window.south);
		    D_line_abs(x + i, window.north,
			       x + i, window.north - row_dist - (window.north - window.south) / shortmark);
		}
		loop++;
	    }
	    x += grid_size;
	}
    }


    /* Draw ticks along left & right borders */
    if (window.south > north)
	y = floor((window.south - north) / grid_size) * grid_size + north;
    else
	y = north - ceil((north - window.south) / grid_size) * grid_size;

    if (direction != DIRN_LON) {
	while (y <= window.north) {
	    loop = 0;

	    for (i = 0; i <= grid_size; i = i + steps) {
		if (y + i < window.south || y + i > window.north) {
		    loop++;
		    continue;
		}

		if (loop == 0) {
		    D_line_abs(window.west, y + i,
			       window.west + (window.east - window.west) / longmark, y + i);
		    D_line_abs(window.east - (window.east - window.west) / longmark, y + i,
			       window.east, y + i);
		}
		if (loop == 5) {
		    D_line_abs(window.west, y + i,
			       window.west + (window.east - window.west) / middlemark, y + i);
		    D_line_abs(window.east - (window.east - window.west) / middlemark, y + i,
			       window.east, y + i);
		}
		else {
		    D_line_abs(window.west, y + i,
			       window.west + (window.east - window.west) / shortmark, y + i);
		    D_line_abs(window.east - (window.east - window.west) / shortmark, y + i,
			       window.east, y + i);
		}
		loop++;
	    }
	    y += grid_size;
	}
    }

    return 0;
}
