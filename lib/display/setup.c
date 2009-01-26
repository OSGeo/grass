/* D_setup (clear)
 *
 * This is a high level D call.
 * It does a full setup for the current graphics frame.
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
 * \brief graphics frame setup
 *
 * D_setup() sets the source coordinate system to the current region, and
 * adjusts the destination coordinate system to preserve the aspect
 * ratio.
 *
 * Performs a full setup for the current graphics frame:
 * 1) Makes sure there is a current graphics frame (will create a full-screen
 *    one, if not);
 * 2) Sets the region coordinates so that the graphics frame and the active
 *    module region agree (may change active module region to do this); and  
 * 3) Performs graphic frame/region coordinate conversion initialization.
 *
 * If <b>clear</b> is true, the frame is cleared (same as running
 * <i>d.erase</i>.) Otherwise, it is not cleared.
 *
 *  \param clear
 *  \return none
 */
void D_setup(int clear)
{
    struct Cell_head region;
    double dt, db, dl, dr;

    R_get_window(&dt, &db, &dl, &dr);

    G_get_set_window(&region);
    if (G_set_window(&region) < 0)
	G_fatal_error("Invalid graphics coordinates");

    D_do_conversions(&region, dt, db, dl, dr);

    if (clear)
	D_erase(DEFAULT_BG_COLOR);
}


/*!
 * \brief 
 *
 * D_setup_unity() sets the source coordinate system to match the
 * destination coordinate system, so that D_* functions use the same
 * coordinate system as R_* functions.
 *
 * If <b>clear</b> is true, the frame is cleared (same as running
 * <i>d.erase</i>.) Otherwise, it is not cleared.
 *
 *  \param clear
 *  \return none
 */
void D_setup_unity(int clear)
{
    double dt, db, dl, dr;

    R_get_window(&dt, &db, &dl, &dr);

    D_set_src(dt, db, dl, dr);
    D_set_dst(dt, db, dl, dr);

    D_update_conversions();

    if (clear)
	D_erase(DEFAULT_BG_COLOR);
}


/*!
 * \brief 
 *
 * D_setup2() sets the source coordinate system to its arguments, and if
 * the <b>fit</b> argument is non-zero, adjusts the destination coordinate
 * system to preserve the aspect ratio.
 *
 * If <b>clear</b> is true, the frame is cleared (same as running
 * <i>d.erase</i>.) Otherwise, it is not cleared.
 *
 *  \param clear
 *  \param fit
 *  \param s_top
 *  \param s_bottom
 *  \param s_left
 *  \param s_right
 *  \return none
 */
void D_setup2(int clear, int fit, double st, double sb, double sl, double sr)
{
    double dt, db, dl, dr;

    R_get_window(&dt, &db, &dl, &dr);

    D_set_src(st, sb, sl, sr);
    D_set_dst(dt, db, dl, dr);

    if (fit)
	D_fit_d_to_u();

    D_update_conversions();

    if (clear)
	D_erase(DEFAULT_BG_COLOR);
}
