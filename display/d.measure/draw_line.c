#include <stdlib.h>
#include <grass/raster.h>
#include "local_proto.h"

int
draw_line(int screen_x, int screen_y, int cur_screen_x, int cur_screen_y,
	  int color1, int color2)
{
    R_standard_color(color1);
    R_move_abs(cur_screen_x, cur_screen_y);
    R_cont_abs(screen_x, screen_y);
    R_standard_color(color2);
    if (abs(screen_y - cur_screen_y) <= abs(screen_x - cur_screen_x)) {
	R_move_abs(cur_screen_x, cur_screen_y - 1);
	R_cont_abs(screen_x, screen_y - 1);
    }
    else {
	R_move_abs(cur_screen_x + 1, cur_screen_y);
	R_cont_abs(screen_x + 1, screen_y);
    }

    R_stabilize();

    return 0;
}
