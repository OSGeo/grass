#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/display.h>
#include "local_proto.h"

void plot(double lon1, double lat1, double lon2, double lat2,
	  int line_color, int text_color)
{
    int nsteps = 1000;
    int i;

    D_setup(0);

    D_use_color(line_color);

    if (lon1 == lon2) {
	D_line_abs(lon1, lat1, lon2, lat2);
	return;
    }

    if (lon1 > lon2) {
	double tmp = lon1;
	lon1 = lon2;
	lon2 = tmp;
    }

    G_shortest_way(&lon1, &lon2);

    G_begin_rhumbline_equation(lon1, lat1, lon2, lat2);

    D_begin();

    for (i = 0; i <= nsteps; i++) {
	double lon = lon1 + (lon2 - lon1) * i / nsteps;
	double lat = G_rhumbline_lat_from_lon(lon);
	if (i == 0)
	    D_move_abs(lon, lat);
	else
	    D_cont_abs(lon, lat);
    }

    D_end();
    D_stroke();
}
