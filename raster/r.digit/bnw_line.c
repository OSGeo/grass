
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

#include <stdlib.h>
#include <grass/display.h>


int
black_and_white_line(int screen_x, int screen_y, int cur_screen_x,
		     int cur_screen_y)
{
    R_standard_color(D_translate_color("white"));
    R_move_abs(cur_screen_x, cur_screen_y);
    R_cont_abs(screen_x, screen_y);
    R_standard_color(D_translate_color("black"));
    if (abs(screen_y - cur_screen_y) <= abs(screen_x - cur_screen_x)) {
	R_move_abs(cur_screen_x, cur_screen_y - 1);
	R_cont_abs(screen_x, screen_y - 1);
    }
    else {
	R_move_abs(cur_screen_x + 1, cur_screen_y);
	R_cont_abs(screen_x + 1, screen_y);
    }

    return 0;
}
