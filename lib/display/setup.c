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
#include <string.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>


/*!
 * \brief initialize/create a frame
 *
 * This routine
 * performs a series of initialization steps for the current frame. It also
 * creates a full screen frame if there is no current frame. The <b>clear</b>
 * flag, if set to 1, tells this routine to clear any information associated with
 * the frame: graphics as well as region information.
 * This routine relieves the programmer of having to perform the following
 * idiomatic function call sequence
 *
 *  \param clear
 *  \return int
 */


/*!
 * \brief graphics frame setup
 *
 * Performs a full setup
 * for the current graphics frame: 1) Makes sure there is a current graphics
 * frame (will create a full-screen one, if not); 2) Sets the region coordinates
 * so that the graphics frame and the active module region agree (may change
 * active module region to do this); and 3) performs graphic frame/region
 * coordinate conversion initialization.
 * If <b>clear</b> is true, the frame is cleared (same as running
 * <i>d.erase.</i>) Otherwise, it is not cleared.
 *
 *  \param clear
 *  \return int
 */

int D_setup(int clear)
{
    struct Cell_head region;
    char name[128];
    int t, b, l, r;

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
    if (D_get_screen_window(&t, &b, &l, &r))
	G_fatal_error("Getting graphics coordinates");

    /* clear the frame, if requested to do so */
    if (clear) {
	D_clear_window();
	R_standard_color(D_translate_color(DEFAULT_BG_COLOR));
	R_box_abs(l, t, r, b);
    }

    /* Set the map region associated with graphics frame */
    G_get_set_window(&region);
    if (D_check_map_window(&region))
	G_fatal_error("Setting graphics coordinates");
    if (G_set_window(&region) < 0)
	G_fatal_error("Invalid graphics coordinates");

    /* Determine conversion factors */
    if (D_do_conversions(&region, t, b, l, r))
	G_fatal_error("Error calculating graphics-region conversions");

    /* set text clipping, for good measure */
    R_set_window(t, b, l, r);
    R_move_abs(0, 0);
    D_move_abs(0, 0);
    return 0;
}
