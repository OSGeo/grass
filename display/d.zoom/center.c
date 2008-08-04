#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include "local_proto.h"

int make_window_center(struct Cell_head *window, double magnify, double east,
		       double north)
{
    char buffer[64];
    double east_west, north_south;
    int len_n, len_e;

    len_n = len_e = 0;

    if (east < 0.0 && north < 0.0) {
	east = (window->east + window->west) / 2.;
	north = (window->north + window->south) / 2.;
    }

    east_west = (window->east - window->west) / magnify;
    window->east = east + east_west / 2;
    window->west = east - east_west / 2;
    if (window->proj == PROJECTION_LL) {
	if (east_west > 360) {
	    window->east = east + 180;
	    window->west = east - 180;
	}
	window->east = G_adjust_easting(window->east, window);
    }

    north_south = (window->north - window->south) / magnify;
    window->north = north + north_south / 2;
    window->south = north - north_south / 2;
    G_limit_south(&window->south, window->proj);
    G_limit_north(&window->north, window->proj);

    G_format_easting(window->east, buffer, window->proj);
    G_format_easting(window->west, buffer, window->proj);
    G_format_northing(window->north, buffer, window->proj);
    G_format_northing(window->south, buffer, window->proj);

    return 0;
}
