
/****************************************************************************
 *
 * MODULE:       r.digit
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Interactive tool used to draw and save vector features
 *               on a graphics monitor using a pointing device (mouse)
 *               and save to a raster map.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <grass/gis.h>
#include <grass/display.h>
#include "local_proto.h"


int get_line(FILE * fd, struct Categories *labels)
{
    int x, y;
    int px = 0, py = 0;
    int any;
    char east[256], north[256];

    instructions(0);
    x = y = -9999;
    any = 0;
    while (get_point(&x, &y, east, north)) {
	if (!any) {
	    fprintf(fd, "LINE\n");
	    any = 1;
	}
	else {
	    black_and_white_line(px, py, x, y);
	    R_flush();
	}
	px = x;
	py = y;
	fprintf(fd, " %s %s\n", east, north);
    }
    get_category(fd, "line", labels);
    return any;
}
