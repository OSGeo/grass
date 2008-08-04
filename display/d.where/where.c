#include <unistd.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/gprojects.h>
#include <grass/display.h>
#include <grass/raster.h>
#include "local_proto.h"

static int nlines = 4;
static int header(int, int, int);
static int show(char *, int, int, int);

int where_am_i(int once, int have_spheroid, int decimal, int wgs84,
	       int dcoord)
{
    char buffer[200];
    char buf1[50], buf2[50];
    char temp[100];
    double lat, lon;
    int screen_x, screen_y;
    int cur_screen_x = 0, cur_screen_y = 0;
    double east, north;
    int button;
    int white, black;
    int projection;
    int draw_on;
    extern struct pj_info iproj, oproj;

    projection = G_projection();
    white = D_translate_color("white");
    black = D_translate_color("black");

    screen_x = ((int)D_get_d_west() + (int)D_get_d_east()) / 2;
    screen_y = ((int)D_get_d_north() + (int)D_get_d_south()) / 2;
    draw_on = 0;

    if (!dcoord)
	header(once, have_spheroid, wgs84);

    for (;;) {
	R_get_location_with_pointer(&screen_x, &screen_y, &button);
	if (button == 3 && !once)
	    return (0);

	east = D_d_to_u_col((double)screen_x);
	north = D_d_to_u_row((double)screen_y);

	if (dcoord) {
	    fprintf(stdout, "%.1f,%.1f\n",
		    100 * (east - D_get_u_west()) / (D_get_u_east() -
						     D_get_u_west()),
		    100 * (north - D_get_u_south()) / (D_get_u_north() -
						       D_get_u_south()));

	    if (once)
		break;
	    else
		continue;
	}

	if (decimal) {
	    G_format_easting(east, buf1, 0);
	    G_format_northing(north, buf2, 0);
	}
	else {
	    G_format_easting(east, buf1, projection);
	    G_format_northing(north, buf2, projection);
	}
	sprintf(buffer, "%18s %18s", buf1, buf2);
	if (have_spheroid) {
	    lat = north;
	    lon = east;

	    if (pj_do_proj(&lon, &lat, &iproj, &oproj) < 0)
		G_fatal_error("Error in pj_do_proj()");

	    if (decimal) {
		G_format_easting(lon, buf1, 0);
		G_format_northing(lat, buf2, 0);
	    }
	    else {
		G_lon_format(lon, buf1);
		G_lat_format(lat, buf2);
	    }
	    sprintf(temp, " %18s %18s", buf1, buf2);
	    strcat(buffer, temp);
	}
	if (once) {
	    sprintf(temp, " %d", button);
	    strcat(buffer, temp);
	}
	show(buffer, once, have_spheroid, wgs84);
	if (button != 2)
	    draw_on = 0;

	if (draw_on) {
	    black_and_white_line(black, white, screen_x, screen_y,
				 cur_screen_x, cur_screen_y);
	    cur_screen_x = screen_x;
	    cur_screen_y = screen_y;
	    R_move_abs(cur_screen_x, cur_screen_y);
	}
	else if (button == 2) {
	    R_move_abs(screen_x, screen_y);
	    cur_screen_x = screen_x;
	    cur_screen_y = screen_y;
	    draw_on = 1;
	}
	nlines++;
	if (once)
	    return 0;
    }

    return 0;
}

static int show(char *buf, int once, int have_spheroid, int wgs84)
{
    fprintf(stdout, "%s\n", buf);
    if (!isatty(1))
	fprintf(stderr, "%s\n", buf);

    if (nlines >= 21) {
	header(once, have_spheroid, wgs84);
	nlines = 4;
    }

    return 0;
}

static int header(int once, int have_spheroid, int wgs84)
{
    int projection = G_projection();

    if (!once) {
	fprintf(stderr, "\nButtons:\n");
	fprintf(stderr, "Left:   where am i\n");
	fprintf(stderr, "Middle: draw to/from here\n");
	fprintf(stderr, "Right:  quit this\n\n");
    }
    else
	fprintf(stderr, "\nClick mouse button on desired location\n\n");

    if (wgs84)
	fprintf(stderr, "%69s\n", "WGS84 Co-ordinates");
    if (projection == PROJECTION_LL)
	fprintf(stderr, "%18s %18s", "LON:", "LAT:");
    else
	fprintf(stderr, "%18s %18s", "EAST:", "NORTH:");
    if (have_spheroid)
	fprintf(stderr, " %18s %18s", "LON:", "LAT:");
    fprintf(stderr, "\n");

    return 0;
}
