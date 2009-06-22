#include <grass/display.h>
#include <grass/glocale.h>
#include "globals.h"
#include "local_proto.h"

#define NP Region.npoints
#define PT Region.point


int draw_region(void)
{
    int x = 0, y = 0, button = 0;

    /* If this is new polygon get init and get first point */
    if (!Region.area.define) {
	Region.view = NULL;
	Region.npoints = 0;

	/* get first point */
	Menu_msg("Mouse: (Left=Select Point) (Right=Quit)");
	while (Region.view == NULL) {
	    Mouse_pointer(&x, &y, &button);

	    if (In_view(VIEW_MAP1, x, y))
		Region.view = VIEW_MAP1;
	    else if (In_view(VIEW_MAP1_ZOOM, x, y))
		Region.view = VIEW_MAP1_ZOOM;

	    if (button == RIGHT_BUTTON)
		return (0);
	}
	Region.area.define = 1;
	add_point(x, y);
	R_stabilize();
    }
    /* if the area is completed have to delete before adding */
    else if (Region.area.completed) {
	Menu_msg("Mouse: (Middle=Backup Point) (Right=Quit)");
	while (button != MIDDLE_BUTTON) {
	    Mouse_pointer(&x, &y, &button);
	    if (button == RIGHT_BUTTON)
		return (0);
	    if (button == MIDDLE_BUTTON) {
		del_point();
		R_stabilize();
		if (NP <= 0)
		    return (0);
	    }
	}
    }

    /* now get as many points as they want */
    Menu_msg("Mouse: (Left=Select Pt) (Middle=Backup Pt) (Right=Quit)");
    while (!signalflag.interrupt) {
	R_standard_color(WHITE);
	Mouse_line_anchored(PT[NP - 1].x, PT[NP - 1].y, &x, &y, &button);
	switch (button) {
	case LEFT_BUTTON:
	    if (!In_view(Region.view, x, y))
		continue;
	    add_point(x, y);
	    R_stabilize();
	    break;
	case MIDDLE_BUTTON:
	    if (!In_view(Region.view, x, y))
		continue;
	    del_point();
	    R_stabilize();
	    if (NP <= 0)
		return (0);
	    break;
	case RIGHT_BUTTON:
	    return (0);
	}
    }

    return 0;
}


int line_in_map1(int x1, int y1, int x2, int y2, int color)
{
    int c1, r1, c2, r2;

    c1 = view_to_col(VIEW_MAP1_ZOOM, x1);
    r1 = view_to_row(VIEW_MAP1_ZOOM, y1);
    c2 = view_to_col(VIEW_MAP1_ZOOM, x2);
    r2 = view_to_row(VIEW_MAP1_ZOOM, y2);
    G_message(_("\nOrig:(x1 y1), (x2 y2) = (%d %d), (%d %d)"),
	      x1, y1, x2, y2);

    x1 = col_to_view(VIEW_MAP1, c1);
    y1 = row_to_view(VIEW_MAP1, r1);
    x2 = col_to_view(VIEW_MAP1, c2);
    y2 = row_to_view(VIEW_MAP1, r2);

    G_message(_("\nNew:(x1 y1), (x2 y2) = (%d %d), (%d %d)"), x1, y1, x2, y2);
    R_standard_color(color);
    R_move_abs(x1, y1);
    R_cont_abs(x2, y2);

    return 0;
}
