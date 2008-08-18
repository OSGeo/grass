/*
 * D_new_window(name, t, b, l, r)
 *   creates a new window with given coordinates
 *   if "name" is an empty string, the routine returns a unique
 *   string in "name"
 *
 * D_show_window(color)
 *   outlines current window in color (from ../colors.h)
 *
 * D_get_screen_window(t, b, l, r)
 *   returns current window's coordinates 
 *
 * D_check_map_window(wind)
 *   if map window (m_win) already assigned
 *       map window is read into the struct "wind"
 *   else
 *       struct "wind" is written to map window (m_win)
 *
 * D_erase_window()
 *   Erases the window on scree.  Does not affect window contents list.
 */

#include <string.h>
#include <grass/colors.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>

static struct {
    double t, b, l, r;
} screen_window;
static int screen_window_set;

static struct Cell_head map_window;
static int map_window_set;

void D_set_window(double t, double b, double l, double r)
{
    screen_window.t = t;
    screen_window.b = b;
    screen_window.l = l;
    screen_window.r = r;
    screen_window_set = 1;
    R_set_window(t, b, l, r);
}

/*!
 * \brief retrieve current frame coordinates
 *
 * Returns current frame's
 * coordinates in the pointers <b>top, bottom, left</b>, and <b>right.</b>
 *
 *  \param top
 *  \param bottom
 *  \param left
 *  \param right
 *  \return int
 */

void D_get_screen_window(double *t, double *b, double *l, double *r)
{
    if (!screen_window_set) {
	R_get_window(
	    &screen_window.t, &screen_window.b,
	    &screen_window.l, &screen_window.r);
	screen_window_set = 1;
    }

    *t = screen_window.t;
    *b = screen_window.b;
    *l = screen_window.l;
    *r = screen_window.r;
}


/*!
 * \brief assign/retrieve current map region
 *
 * Graphics frames can have GRASS map regions associated with
 * them. This routine passes the map <b>region</b> to the current graphics
 * frame. If a GRASS region is already associated with the graphics frame, its
 * information is copied into <b>region</b> for use by the calling module.
 * Otherwise <b>region</b> is associated with the current graphics frame.
 * Note this routine is called by <i>D_setup.</i>
 *
 *  \param region
 *  \return void
 */

void D_check_map_window(struct Cell_head *wind)
{
    if (map_window_set)
	*wind = map_window;
    else
    {
	map_window = *wind;
	map_window_set = 1;
    }
}

/*!
 * \brief erase current frame
 *
 * Erases the frame on the
 * screen using the currently selected color.
 *
 *  \param ~
 */

void D_erase(const char *color)
{
    double t, b, l, r;
    int colorindex;

    D_get_screen_window(&t, &b, &l, &r);

    /* Parse and select background color */
    colorindex = D_parse_color(color, 0);
    D_raster_use_color(colorindex);

    /* Do the plotting */
    R_box_abs(l, t, r, b);
}

