
#include "globals.h"
#include <grass/display.h>



/*************************************************************
del_point: delete last point from Region point list

returns: TRUE point deleted ok, FALSE list empty
*************************************************************/

int del_point(void)
{
    int last;

    if (Region.npoints <= 0) {
	Region.npoints = 0;	/* for safety */
	return (0);
    }

    last = --Region.npoints;
    if (Region.npoints == 0) {
	Region.area.define = 0;
	Region.view = NULL;
    }
    Region.area.completed = 0;

    /* draw the removed line in grey */
    if (Region.npoints > 0) {
	R_standard_color(GREY);
	R_move_abs(Region.point[last].x, Region.point[last].y);
	R_cont_abs(Region.point[last - 1].x, Region.point[last - 1].y);
	/*    if (Region.view == VIEW_MAP1_ZOOM)
	   line_in_map1(Region.point[last-1].x, Region.point[last-1].y,
	   Region.point[last].x, Region.point[last].y, GREY); */
    }

    return (1);
}
