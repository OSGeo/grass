#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/display.h>
#include "local_proto.h"

int make_window_box(struct Cell_head *window, double magnify, int full,
		    int hand)
{
    int screen_x, screen_y;
    double px, py, ux1, uy1, ux2, uy2;
    double ns, ew;
    int button;
    int cur_screen_x, cur_screen_y;
    int mode;			/* 1, 2 */
    int resetwin;
    struct Cell_head defwin;
    int printmenu = 1;

    G_get_default_window(&defwin);

    mode = 1;
    while (1) {
	resetwin = 0;
	if (!hand) {
	    if (printmenu) {
		fprintf(stderr, "\n\nButtons:\n");
		fprintf(stderr, "Left:   1. corner\n");
		fprintf(stderr, "Middle: Unzoom\n");
		if (full)
		    fprintf(stderr, "Right:  Main menu\n\n");
		else
		    fprintf(stderr, "Right:  Quit\n\n");

		printmenu = 0;
	    }
	}
	else {
	    if (mode == 1)
		fprintf(stderr, "\r1. corner");
	    else
		fprintf(stderr, "\r2. corner");
	}
	if (mode == 1) {
	    if (!hand) {
		R_get_location_with_pointer(&screen_x, &screen_y, &button);
	    }
	    else {
		R_get_location_with_box(0, 0, &screen_x, &screen_y, &button);
	    }
	    cur_screen_x = screen_x;
	    cur_screen_y = screen_y;
	}
	else {
	    R_get_location_with_box(cur_screen_x, cur_screen_y, &screen_x,
				    &screen_y, &button);
	}

	/* For print only */
	px = D_d_to_u_col((double)screen_x);
	py = D_d_to_u_row((double)screen_y);
	if (!hand)
	    print_coor(window, py, px);

	if (button == 1) {
	    if (!hand) {
		if (mode == 1) {
		    fprintf(stderr, "\n\nButtons:\n");
		    fprintf(stderr, "Left:   1. corner (reset)\n");
		    fprintf(stderr, "Middle: 2. corner\n");
		    if (full)
			fprintf(stderr, "Right:   Main menu\n\n");
		    else
			fprintf(stderr, "Right:   Quit\n\n");
		    mode = 2;
		}
		if (mode == 2) {
		    cur_screen_x = screen_x;
		    cur_screen_y = screen_y;
		}
	    }
	    else {		/* hand */
		if (mode == 1) {
		    mode = 2;
		}
		else {
		    ux1 = D_d_to_u_col((double)cur_screen_x);
		    uy1 = D_d_to_u_row((double)cur_screen_y);
		    ux2 = D_d_to_u_col((double)screen_x);
		    uy2 = D_d_to_u_row((double)screen_y);
		    resetwin = 1;
		    mode = 1;
		}
	    }
	}
	else if (button == 2) {
	    if (mode == 1) {	/* unzoom */
		ux2 = D_d_to_u_col((double)screen_x);
		uy2 = D_d_to_u_row((double)screen_y);
		ew = window->east - window->west;
		ns = window->north - window->south;

		if (ns <= window->ns_res)
		    ns = 2 * window->ns_res;
		else
		    ew /= magnify;

		if (ew <= window->ew_res)
		    ew = 2 * window->ew_res;
		else
		    ns /= magnify;

		ux1 = window->east + ew / 2;
		ux2 = window->west - ew / 2;
		uy1 = window->north + ns / 2;
		uy2 = window->south - ns / 2;
	    }
	    else {
		ux1 = D_d_to_u_col((double)cur_screen_x);
		uy1 = D_d_to_u_row((double)cur_screen_y);
		ux2 = D_d_to_u_col((double)screen_x);
		uy2 = D_d_to_u_row((double)screen_y);
		printmenu = 1;
		mode = 1;
	    }
	    fprintf(stderr, "\n");
	    resetwin = 1;
	}
	else {
	    fprintf(stderr, "\n");
	    return 1;
	}
	if (resetwin) {
	    set_win(window, ux1, uy1, ux2, uy2, hand);
	}
    }

    fprintf(stderr, "\n");
    return 1;			/* not reached */
}
