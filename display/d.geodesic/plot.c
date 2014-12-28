#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/display.h>
#include "local_proto.h"

void plot(double lon1, double lat1, double lon2, double lat2,
	  int line_color, int text_color, double factor, const char *unit)
{
    double distance;
    double text_x, text_y;
    double a, e2;
    int nsteps = 1000;
    int i;

    /* establish the current graphics window */
    D_setup(0);

    G_get_ellipsoid_parameters(&a, &e2);
    G_begin_geodesic_distance(a, e2);

    D_use_color(line_color);

    G_shortest_way(&lon1, &lon2);

    if (lon1 != lon2) {
	G_begin_geodesic_equation(lon1, lat1, lon2, lat2);

	D_begin();

	for (i = 0; i <= nsteps; i++) {
	    double lon = lon1 + (lon2 - lon1) * i / nsteps;
	    double lat = G_geodesic_lat_from_lon(lon);

	    if (i == 0)
		D_move_abs(lon, lat);
	    else
		D_cont_abs(lon, lat);
	}

	D_end();
	D_stroke();

	text_x = (lon1 + lon2) / 2;
	text_y = G_geodesic_lat_from_lon(text_x);
    }
    else {
	D_line_abs(lon1, lat1, lon2, lat2);
	text_x = (lon1 + lon2) / 2;
	text_y = (lat1 + lat2) / 2;
    }

    if (text_color != -1) {
	double t, b, l, r;
	char buf[100];

	D_text_size(10, 10);

	distance = G_geodesic_distance(lon1, lat1, lon2, lat2);
	sprintf(buf, "%.0f %s", distance / factor, unit);

	D_pos_abs(text_x, text_y);
	D_get_text_box(buf, &t, &b, &l, &r);

	if (t - D_get_u_north() > 0)
	    text_y -= t - D_get_u_north();
	if (b - D_get_u_south() < 0)
	    text_y -= b - D_get_u_south();
	if (r - D_get_u_east() > 0)
	    text_x -= r - D_get_u_east();
	if (l - D_get_u_west() < 0)
	    text_x -= l - D_get_u_west();

	D_use_color(text_color);

	D_pos_abs(text_x, text_y);
	D_text(buf);
    }
}
