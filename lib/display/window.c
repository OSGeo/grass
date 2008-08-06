/*
 * D_new_window(name, t, b, l, r)
 *   creates a new window with given coordinates
 *   if "name" is an empty string, the routine returns a unique
 *   string in "name"
 *
 * D_reset_screen_window(t, b, l, r)
 *   resets the edges of the current window
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
 * D_remove_window()
 *   remove any trace of window
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
    int t, b, l, r;
} screen_window;
static int screen_window_set;

static struct Cell_head map_window;
static int map_window_set;

static void D_set_window(int t, int b, int l, int r)
{
    screen_window.t = t;
    screen_window.b = b;
    screen_window.l = l;
    screen_window.r = r;
    screen_window_set = 1;
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

int D_get_screen_window(int *t, int *b, int *l, int *r)
{
    if (!screen_window_set)
	return -1;

    *t = screen_window.t;
    *b = screen_window.b;
    *l = screen_window.l;
    *r = screen_window.r;
    return 0;
}


/*!
 * \brief create new graphics frame
 *
 * Creates a new frame <b>name</b> with
 * coordinates <b>top, bottom, left</b>, and <b>right.</b> If <b>name</b>
 * is the empty string '''' (i.e., *<b>name</b> = = 0), the routine returns a
 * unique string in <b>name.</b>
 *
 *  \param name
 *  \param top
 *  \param bottom
 *  \param left
 *  \param right
 *  \return int
 */

void D_new_window(char *name, int t, int b, int l, int r)
{
    screen_window_set = 0;
    map_window_set = 0;
    /* Display outline of new window */
    D_show_window(GRAY);
    D_set_window(t, b, l, r);
}


/*!
 * \brief create new graphics frame, with coordinates in percent
 *
 * Creates a new frame <b>name</b> with coordinates <b>top, bottom,
 * left</b>, and <b>right</b> as percentages of the screen size.
 * If <b>name</b> is the empty string "" (i.e., *<b>name</b> == 0),
 * the routine returns a unique string in <b>name.</b>
 *
 *  \param name
 *  \param bottom
 *  \param top
 *  \param left
 *  \param right
 *  \return int
 */

void D_new_window_percent(char *name, float b, float t, float l, float r)
{
    int scr_t = R_screen_top();
    int scr_b = R_screen_bot();
    int scr_l = R_screen_left();
    int scr_r = R_screen_rite();

    int win_t = 0.5 + scr_t + (scr_b - scr_t) * (100. - t) / 100.0;
    int win_b = 0.5 + scr_t + (scr_b - scr_t) * (100. - b) / 100.0;
    int win_l = 0.5 + scr_l + (scr_r - scr_l) * l / 100.0;
    int win_r = 0.5 + scr_l + (scr_r - scr_l) * r / 100.0;

    if (win_t < scr_t)
	win_t = scr_t;
    if (win_b > scr_b)
	win_b = scr_b;
    if (win_l < scr_l)
	win_l = scr_l;
    if (win_r > scr_r)
	win_r = scr_r;

    D_new_window(name, win_t, win_b, win_l, win_r);
}


/*!
 * \brief outlines current frame
 *
 * Outlines
 * current frame in <b>color.</b> Appropriate colors are found in
 * $GISBASE/src/D/libes/colors.h\remarks{$GISBASE is the directory where GRASS
 * is installed. See UNIX_Environment for details.} and are spelled
 * with lowercase letters.
 *
 *  \param color
 *  \return int
 */

void D_show_window(int color)
{
    int t, b, l, r;

    D_get_screen_window(&t, &b, &l, &r);

    D_set_window(t - 1, b + 1, l - 1, r + 1);

    R_standard_color(color);
    R_move_abs(l - 1, b);
    R_cont_abs(l - 1, t - 1);
    R_cont_abs(r, t - 1);
    R_cont_abs(r, b);
    R_cont_abs(l - 1, b);
    R_flush();

    D_set_window(t, b, l, r);
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
 *  \return int
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
 * \brief resets current frame position
 *
 * Re-establishes the screen position of a
 * frame at the location specified by <b>top, bottom, left, and right.</b>
 *
 *  \param top
 *  \param bottom
 *  \param left
 *  \param right
 *  \return int
 */

void D_reset_screen_window(int t, int b, int l, int r)
{
    D_show_window(D_translate_color(DEFAULT_BG_COLOR));

    D_set_window(t, b, l, r);

    D_show_window(D_translate_color(DEFAULT_FG_COLOR));
}

/*!
 * \brief remove a frame
 *
 * Removes any trace of the
 * current frame.
 *
 *  \param ~
 */

void D_remove_window(void)
{
    screen_window_set = 0;
    map_window_set = 0;
}

/*!
 * \brief erase current frame
 *
 * Erases the frame on the
 * screen using the currently selected color.
 *
 *  \param ~
 */

void D_erase_window(void)
{
    int t, b, l, r;

    D_get_screen_window(&t, &b, &l, &r);
    R_box_abs(l, t, r, b);
    R_flush();
}

void D_erase(const char *color)
{
    int t, b, l, r;
    int colorindex;

    D_get_screen_window(&t, &b, &l, &r);
    D_clear_window();

    /* Parse and select background color */
    colorindex = D_parse_color(color, 0);
    D_raster_use_color(colorindex);

    /* Do the plotting */
    R_box_abs(l, t, r, b);

    /* Add erase item to the pad */
    D_set_erase_color(color);
}

void D_remove_windows(void)
{
    screen_window_set = 0;
    map_window_set = 0;
}

void D_full_screen(void)
{
    D_remove_windows();

    D_new_window_percent("full_screen", 0.0, 100.0, 0.0, 100.0);

    R_standard_color(D_translate_color(DEFAULT_BG_COLOR));
    R_erase();
}

