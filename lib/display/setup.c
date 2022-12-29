/*!
  \file lib/display/setup.c

  \brief Display Driver - setup

  (C) 2006-2011 by the GRASS Development Team

  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.

  \author Glynn Clements <glynn gclements.plus.com> (original contributor)
  \author Huidae Cho <grass4u gmail.com>
*/

#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>

#include "driver.h"

/*!
  \brief Graphics frame setup

  This is a high level D call. It does a full setup for the current
  graphics frame.
 
  Note: Connection to driver must already be made.
  
  Sets the source coordinate system to the current region, and
  adjusts the destination coordinate system to preserve the aspect
  ratio.
  
  Performs a full setup for the current graphics frame:
  - Makes sure there is a current graphics frame (will create a full-screen
  one, if not);
  - Sets the region coordinates so that the graphics frame and the active
  module region agree (may change active module region to do this); and  
  - Performs graphic frame/region coordinate conversion initialization.
 
  If <b>clear</b> is true, the frame is cleared (same as running
  <i>d.erase</i>.) Otherwise, it is not cleared.

  \param clear 1 to clear frame (visually and coordinates)
*/
void D_setup(int clear)
{
    struct Cell_head region;
    double dt, db, dl, dr;

    D_get_frame(&dt, &db, &dl, &dr);

    G_get_set_window(&region);
    Rast_set_window(&region);

    D_do_conversions(&region, dt, db, dl, dr);

    D_set_clip_window_to_screen_window();

    if (clear)
	D_erase(DEFAULT_BG_COLOR);

    D_set_clip_window_to_map_window();
}

/*!
  \brief Graphics frame setup
 
  Sets the source coordinate system to match the
  destination coordinate system, so that D_* functions use the same
  coordinate system as R_* functions.
 
  If <b>clear</b> is true, the frame is cleared (same as running
  <i>d.erase</i>). Otherwise, it is not cleared.
  
  \param clear non-zero code to clear the frame
*/
void D_setup_unity(int clear)
{
    double dt, db, dl, dr;

    D_get_frame(&dt, &db, &dl, &dr);

    D_set_src(dt, db, dl, dr);
    D_set_dst(dt, db, dl, dr);

    D_update_conversions();

    D_set_clip_window_to_screen_window();

    if (clear)
	D_erase(DEFAULT_BG_COLOR);

    D_set_clip_window_to_map_window();
}

/*!
  \brief Sets source coordinate system
  
  Sets the source coordinate system to its arguments, and if
  the <b>fit</b> argument is non-zero, adjusts the destination coordinate
  system to preserve the aspect ratio.
  
  If <b>clear</b> is true, the frame is cleared (same as running
  <i>d.erase</i>). Otherwise, it is not cleared.
  
  \param clear non-zero code to clear the frame
  \param fit non-zero code to adjust destination coordinate system
  \param s_top
  \param s_bottom
  \param s_left
  \param s_right
*/
void D_setup2(int clear, int fit, double st, double sb, double sl, double sr)
{
    double dt, db, dl, dr;

    D_get_frame(&dt, &db, &dl, &dr);

    D_set_src(st, sb, sl, sr);
    D_set_dst(dt, db, dl, dr);

    if (fit)
	D_fit_d_to_u();

    D_update_conversions();

    D_set_clip_window_to_screen_window();

    if (clear)
	D_erase(DEFAULT_BG_COLOR);

    D_set_clip_window_to_map_window();
}

/*!
  \brief Get driver output file

  \return file name or NULL if not defined
*/
const char *D_get_file(void)
{
    return COM_Graph_get_file();
}
