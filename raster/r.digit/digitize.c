
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
#include <grass/glocale.h>
#include "local_proto.h"


int digitize(FILE * fd)
{
    int any;
    struct Categories labels;

    Rast_init_cats((CELL) 0, "", &labels);
    any = 0;
    for (;;) {
	switch (get_type()) {
	case 'A':		/* area */
	    if (get_area(fd, &labels))
		any = 1;
	    break;
	case 'C':		/* circle */
	    if (get_circle(fd, &labels))
		any = 1;
	    break;
	case 'L':		/* line */
	    if (get_line(fd, &labels))
		any = 1;
	    break;
	case 'X':		/* done */
	    return any;
	case 'Q':		/* exit without saving */
	    if (G_yes(_("Quit without creating a map?? "), 0))
		return 0;
	}
    }
}
