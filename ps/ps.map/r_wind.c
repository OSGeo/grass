/* parse the "region" instruction */

#include <string.h>
#include <grass/colors.h>
#include <grass/raster.h>
#include "local_proto.h"

#define KEY(x) (strcmp(key,x)==0)

static char *help[] = {
    "color  color",
    "width  #",
    ""
};

int read_wind(char *name, char *mapset)
{
    char fullname[100];
    char buf[1024];
    char *key, *data;
    double width;
    int r, g, b;
    int color_R, color_G, color_B;
    int ret;
    int i;
    double east, west, incr;
    struct Cell_head window;

    sprintf(fullname, "%s in %s", name, mapset);

    G_get_element_window(&window, "windows", name, mapset);

    width = 1.;
    color_R = color_G = color_B = 0;

    while (input(2, buf, help)) {
	if (!key_data(buf, &key, &data))
	    continue;

	if (KEY("width")) {
	    width = -1.;
	    *mapset = 0;
	    if (sscanf(data, "%lf%s", &width, mapset) < 1 || width < 0.) {
		width = 1.;
		error(key, data, "illegal width (wind)");
	    }
	    if (mapset[0] == 'i')
		width = width / 72.0;
	    continue;
	}


	if (KEY("color")) {
	    ret = G_str_to_color(data, &r, &g, &b);
	    if (ret == 1) {
		color_R = r;
		color_G = g;
		color_B = b;
	    }
	    else if (ret == 2)	/* i.e. "none" */
		color_R = color_G = color_B = -1;
	    else
		error(key, data, "illegal color request (wind)");

	    continue;
	}
	error(key, "", "illegal request (wind)");
    }

    /* draw horizontal lines in 3 pieces - lat-lon lines must not
     * extend more than half the globe
     */
    west = window.west;
    incr = (window.east - window.west) / 3;
    for (i = 0; i < 3; i++) {
	east = west + incr;
	sprintf(buf, "L 0 %f %f %f %f %d %d %d %.8f",
		west, window.north, east, window.north,
		color_R, color_G, color_B, width);
	add_to_plfile(buf);

	sprintf(buf, "L 0 %f %f %f %f %d %d %d %.8f",
		west, window.south, east, window.south,
		color_R, color_G, color_B, width);
	add_to_plfile(buf);

	west = east;
    }

    sprintf(buf, "L 0 %f %f %f %f %d %d %d %.8f",
	    window.east, window.north, window.east, window.south,
	    color_R, color_G, color_B, width);
    add_to_plfile(buf);

    sprintf(buf, "L 0 %f %f %f %f %d %d %d %.8f",
	    window.west, window.north, window.west, window.south,
	    color_R, color_G, color_B, width);
    add_to_plfile(buf);

    return 1;
}
