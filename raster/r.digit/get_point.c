
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

#include <stdio.h>
#include <grass/display.h>
#include "local_proto.h"


/* button 1 is whereami: no return, keep looping
 *        2 is mark point, return 1 (ok)
 *        3 is done, return 0 (done)
 */

int get_point(int *x, int *y, char *east, char *north)
{
    int button;
    int curx, cury;

    curx = *x;
    cury = *y;
    do {
	if (curx >= 0 && cury >= 0)
	    R_get_location_with_line(curx, cury, &curx, &cury, &button);
	else
	    R_get_location_with_pointer(&curx, &cury, &button);

	if (button == 3)
	    return (0);

	get_east_north(curx, cury, east, north);
	fprintf(stdout, "EAST:  %-20s", east);
	fprintf(stdout, "NORTH: %s\n", north);
	instructions(1);

    } while (button == 1);

    *x = curx;
    *y = cury;
    return (1);
}
