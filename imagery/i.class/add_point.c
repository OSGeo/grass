
/*************************************************************
add_point: add point to region point list

returns: TRUE (1) point added ok, FALSE (0) could not add, list full
*************************************************************/

#include "globals.h"
#include <grass/raster.h>
#include <grass/glocale.h>


int add_point(int x, int y)
{
    int last;

    if ((last = Region.npoints - 1) >= 0
	&& x == Region.point[last].x && y == Region.point[last].y)
	return (1);

    if (Region.npoints >= MAX_VERTEX) {
	G_warning(_("Can't mark another point.  Only %d points allowed.  Sorry."),
		  MAX_VERTEX);
	return (0);
    }

    last++;
    Region.point[last].x = x;
    Region.point[last].y = y;
    Region.npoints++;

    /* draw the added line in yellow */
    if (Region.npoints > 1) {
	R_standard_color(RED);
	R_move_abs(Region.point[last - 1].x, Region.point[last - 1].y);
	R_cont_abs(Region.point[last].x, Region.point[last].y);
	/*    if (Region.view == VIEW_MAP1_ZOOM)
	   line_in_map1(Region.point[last-1].x, Region.point[last-1].y,
	   Region.point[last].x, Region.point[last].y, RED); */
    }

    return (1);
}
