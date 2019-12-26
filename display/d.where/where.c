#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/gprojects.h>
#include <grass/display.h>
#include "local_proto.h"

extern struct pj_info iproj, oproj, tproj;

int where_am_i(char **coords, FILE *fp, int have_spheroid, int decimal,
               int dcoord)
{
    char buf1[50], buf2[50];
    int screen_x, screen_y;
    double east, north;
    int projection;

    projection = G_projection();

    for (;;) {
	if (coords) {
	    const char *x, *y;
	    x = *coords++;
	    if (!x)
		return 0;
	    y = *coords++;
	    if (!y)
		return 0;
	    if (sscanf(x, "%d", &screen_x) != 1 || sscanf(y, "%d", &screen_y) != 1)
		G_fatal_error(_("Invalid coordinates <%s,%s>"), x, y);
	}
	else
	    if (fscanf(fp, "%d %d", &screen_x, &screen_y) != 2)
		return 0;

	east = D_d_to_u_col((double)screen_x);
	north = D_d_to_u_row((double)screen_y);

	if (decimal) {
	    G_format_easting(east, buf1, 0);
	    G_format_northing(north, buf2, 0);
	}
	else {
	    G_format_easting(east, buf1, projection);
	    G_format_northing(north, buf2, projection);
	}
	fprintf(stdout, " %s %s", buf1, buf2);

	if (dcoord) {
	    fprintf(stdout, " %.1f %.1f",
		    100 * (east - D_get_u_west()) / (D_get_u_east() -
						     D_get_u_west()),
		    100 * (north - D_get_u_south()) / (D_get_u_north() -
						       D_get_u_south()));
	}

	if (have_spheroid) {
	    double lat = north;
	    double lon = east;

	    if (GPJ_transform(&iproj, &oproj, &tproj, PJ_FWD,
			      &lon, &lat, NULL) < 0)
		G_fatal_error(_("Error in GPJ_transform()"));

	    if (decimal) {
		G_format_easting(lon, buf1, 0);
		G_format_northing(lat, buf2, 0);
	    }
	    else {
		G_lon_format(lon, buf1);
		G_lat_format(lat, buf2);
	    }
	    fprintf(stdout, " %s %s", buf1, buf2);
	}

	fprintf(stdout, "\n");
    }

    return 0;
}
