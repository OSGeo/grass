/* Function: read map border options
 *
 *  AUTHOR:      M. Hamish Bowman, New Zealand  February 2007
 *
 *  COPYRIGHT:   (c) 2007 Hamish Bowman, and the GRASS Development Team
 *             This program is free software under the GNU General Public
 *             License (>=v2). Read the file COPYING that comes with GRASS
 *             for details.
 */

#include <stdlib.h>
#include <string.h>
#include <grass/colors.h>
#include <grass/raster.h>
#include "border.h"
#include "local_proto.h"

#define KEY(x) (strcmp(key,x)==0)


int read_border(void)
{
    char buf[1024];
    char *key, *data;
    double width;
    char ch;
    int r, g, b;
    double color_r, color_g, color_b;
    int ret;

    static char *help[] = {
	"color    name",
	"width    #",
	""
    };

    G_debug(1, "Reading border settings ..");

    width = 1.;
    color_r = color_g = color_b = 0.;

    while (input(2, buf, help)) {
	if (!key_data(buf, &key, &data))
	    continue;

	if (KEY("color")) {
	    ret = G_str_to_color(data, &r, &g, &b);
	    if (ret == 1) {
		color_r = r / 255.;
		color_g = g / 255.;
		color_b = b / 255.;
	    }
	    else if (ret == 2)	/* i.e. "none" */
		color_r = color_g = color_b = -1.;
	    else
		error(key, data, "illegal border color request");

	    continue;
	}

	if (KEY("width")) {
	    ch = ' ';
	    if (sscanf(data, "%lf%c", &width, &ch) < 1 || width < 0.) {
		width = 1.;
		error(key, data, "illegal border width request");
	    }
	    /* if asked for in inches */
	    if (ch == 'i')
		width = width * 72.;

	    continue;
	}

	error(key, data, "illegal border sub-request");
    }

    brd.r = color_r;
    brd.g = color_g;
    brd.b = color_b;
    brd.width = width;

    return 0;
}
