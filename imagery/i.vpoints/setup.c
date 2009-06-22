#include <string.h>
#include <grass/gis.h>
#include <grass/display.h>
#include "globals.h"

/* D_setup (clear)
 *
 * This is a high level D call.
 * It does a full setup for the current graphics frame.
 *
 *   1. Makes sure there is a current graphics frame
 *      (will create a full-screen one, if not
 *   2. Sets the region coordinates so that the graphics frame
 *      and the active program region agree
 *      (may change active program region to do this).
 *   3. Performs graphic frame/region coordinate conversion intialization
 *
 * Returns: 0 if ok. Exits with error message if failure.
 *
 * Note: Connection to driver must already be made.
 *
 * clear values:
 *   1: clear frame (visually and coordinates)
 *   0: do not clear frame
 */

int dsp_setup(int blank, struct Cell_head *cellhead)
{
    char name[128];
    double t, b, l, r;

    if (D_get_cur_wind(name)) {
	t = R_screen_top();
	b = R_screen_bot();
	l = R_screen_left();
	r = R_screen_rite();
	strcpy(name, "full_screen");
	D_new_window(name, t, b, l, r);
    }

    if (D_set_cur_wind(name))
	G_fatal_error("Current graphics frame not available");

    D_get_screen_window(&t, &b, &l, &r);

    /* clear the window, if requested to do so */
    if (blank) {
	D_clear_window();
	if (!cellmap_present) {
	    R_standard_color(blank);
	    R_box_abs(l, t, r, b);
	}
    }

    D_check_map_window(cellhead);

    if (Rast_set_window(cellhead) < 0)
	G_fatal_error("Invalid graphics window coordinates");

    /* Determine conversion factors */
    D_do_conversions(cellhead, t, b, l, r);

    D_set_clip(t, b, l, r);

    /* set text clipping, for good measure */
    R_set_window(t, b, l, r);
    R_move_abs(0, 0);
    D_move_abs(0, 0);
    return 0;
}
