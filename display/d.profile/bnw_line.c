#include <stdlib.h>
#include <grass/raster.h>
#include <grass/display.h>

int black_and_white_line(int screen_x, int screen_y,
			 int cur_screen_x, int cur_screen_y)
{
    R_standard_color(D_translate_color(DEFAULT_FG_COLOR));
    R_move_abs(cur_screen_x, cur_screen_y);
    R_cont_abs(screen_x, screen_y);
    R_standard_color(D_translate_color(DEFAULT_BG_COLOR));
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
