#include <stdlib.h>
#include <grass/raster.h>

int
black_and_white_line(int c1, int c2, int screen_x, int screen_y,
		     int cur_screen_x, int cur_screen_y)
{
    if (abs(screen_y - cur_screen_y) <= abs(screen_x - cur_screen_x)) {
	if (screen_x > cur_screen_x)
	    R_standard_color(c1);
	else
	    R_standard_color(c2);
	R_move_abs(cur_screen_x, cur_screen_y);
	R_cont_abs(screen_x, screen_y);

	if (screen_x > cur_screen_x)
	    R_standard_color(c2);
	else
	    R_standard_color(c1);
	R_move_abs(cur_screen_x, cur_screen_y - 1);
	R_cont_abs(screen_x, screen_y - 1);
    }
    else {
	if (screen_y > cur_screen_y)
	    R_standard_color(c1);
	else
	    R_standard_color(c2);
	R_move_abs(cur_screen_x, cur_screen_y);
	R_cont_abs(screen_x, screen_y);
	if (screen_y > cur_screen_y)
	    R_standard_color(c2);
	else
	    R_standard_color(c1);
	R_move_abs(cur_screen_x + 1, cur_screen_y);
	R_cont_abs(screen_x + 1, screen_y);
    }

    R_stabilize();

    return 0;
}
