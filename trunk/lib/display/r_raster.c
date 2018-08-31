/*!
  \file lib/display/r_raster.c

  \brief Display Library - Raster graphics subroutines

  (C) 2001-2015 by the GRASS Development Team

  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.

  \author Original author CERL
  \author Monitors support by Martin Landa <landa.martin gmail.com>
*/

#include <grass/config.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/display.h>
#include <grass/spawn.h>

#include "driver.h"

extern const struct driver *PNG_Driver(void);
extern const struct driver *PS_Driver(void);
extern const struct driver *HTML_Driver(void);
#ifdef USE_CAIRO
extern const struct driver *Cairo_Driver(void);
#endif

static struct {
    double t, b, l, r;
} screen, frame;

static void init(void)
{
    const char *fenc = getenv("GRASS_ENCODING");
    const char *font = getenv("GRASS_FONT");
    const char *line_width = getenv("GRASS_RENDER_LINE_WIDTH");
    const char *text_size = getenv("GRASS_RENDER_TEXT_SIZE");
    const char *frame_str = getenv("GRASS_RENDER_FRAME");

    D_font(font ? font : "romans");

    if (fenc)
	D_encoding(fenc);

    if (line_width)
	COM_Line_width(atof(line_width));

    if (text_size) {
	double s = atof(text_size);
	D_text_size(s, s);
    }

    D_text_rotation(0);

    COM_Get_window(&screen.t, &screen.b, &screen.l, &screen.r);
    if (frame_str) {
	sscanf(frame_str, "%lf,%lf,%lf,%lf", &frame.t, &frame.b, &frame.l, &frame.r);
	COM_Set_window(frame.t, frame.b, frame.l, frame.r);
    }
    else
	frame = screen;
}

/*!
  \brief Open display driver

  Default display driver is Cairo, if not available PNG is used.

  \return 0 on success
*/
int D_open_driver(void)
{
    const char *p, *c, *m;
    const struct driver *drv;
    
    G_debug(1, "D_open_driver():");
    p = getenv("GRASS_RENDER_IMMEDIATE");
    c = getenv("GRASS_RENDER_COMMAND");
    m = G_getenv_nofatal("MONITOR");
    
    if (!p && (m || c)) {
        char *cmd;
        char progname[GPATH_MAX];

        cmd = G_recreate_command();
        
        if (c && m) {
            G_warning(_("Both %s and %s are defined. "
                        "%s will be ignored."),
                      "GRASS_RENDER_COMMAND", "MONITOR",
                      "MONITOR");
            m = NULL;
        }
        
        if (c) 
            sprintf(progname, "%s", c);
        else { /* monitors managed by d.mon -> call default renderer */
            char element[GPATH_MAX];
            
            G_temp_element(element);
            strcat(element, "/");
            strcat(element, "MONITORS");
            strcat(element, "/");
            strcat(element, m);
            G_file_name(progname, element, "render.py", G_mapset());
        }

        G_debug(1, "rendering redirected to %s", progname);
        /* assuming Python script here (could be extended in the future) */
        G_spawn_ex(getenv("GRASS_PYTHON"), getenv("GRASS_PYTHON"), progname,
                   cmd, NULL);
        
        G_free(cmd);
        
        /* force exiting GRASS command, leave rendering on
         * GRASS_RENDER_COMMAND program */
        exit(0);
    }

    if (!p)
	G_fatal_error(_("Neither %s (managed by d.mon command) nor %s "
                        "(used for direct rendering) defined"),
		      "MONITOR", "GRASS_RENDER_IMMEDIATE");

    if (p && G_strcasecmp(p, "default") == 0)
	p = NULL;
    
    drv =
	(p && G_strcasecmp(p, "png")   == 0) ? PNG_Driver() :
	(p && G_strcasecmp(p, "ps")    == 0) ? PS_Driver() :
	(p && G_strcasecmp(p, "html")  == 0) ? HTML_Driver() :
#ifdef USE_CAIRO
	(p && G_strcasecmp(p, "cairo") == 0) ? Cairo_Driver() :
	Cairo_Driver();
#else
	PNG_Driver();
#endif
	
    if (p && G_strcasecmp(drv->name, p) != 0)
	G_warning(_("Unknown display driver <%s>"), p);
    G_verbose_message(_("Using display driver <%s>..."), drv->name);
    LIB_init(drv);

    init();

    return 0;
}

/*!
  \brief Close display driver

  If GRASS_NOTIFY is defined, run notifier.
*/
void D_close_driver(void)
{
    const char *cmd = getenv("GRASS_NOTIFY");

    COM_Graph_close();

    if (cmd)
	system(cmd);
}

/*!
  \brief Append command to the cmd file (unused)

  \todo To be removed
*/
int D_save_command(const char *cmd)
{
    return 0;
}

/*!
  \brief Erase display (internal use only)
*/
void D__erase(void)
{
    COM_Erase();
}

/*!
  \brief Set text size (width and height)
 
  \param width text pixel width
  \param height text pixel height
*/
void D_text_size(double width, double height)
{
    COM_Text_size(width, height);
}

/*!
  \brief Set text rotation

  \param rotation value
*/
void D_text_rotation(double rotation)
{
    COM_Text_rotation(rotation);
}

/*!
  \brief Draw text
  
  Writes <em>text</em> in the current color and font, at the current text
  width and height, starting at the current screen location.
  
  \param text text to be drawn
*/
void D_text(const char *text)
{
    COM_Text(text);
}

/*!
  \brief Choose font
 
  Set current font to <em>font name</em>.
  
  \param name font name
*/
void D_font(const char *name)
{
    COM_Set_font(name);
}

/*!
  \brief Set encoding

  \param name encoding name
*/
void D_encoding(const char *name)
{
    COM_Set_encoding(name);
}

/*!
  \brief Get font list

  \param[out] list list of font names
  \param[out] number of items in the list
*/
void D_font_list(char ***list, int *count)
{
    COM_Font_list(list, count);
}

/*!
  \brief Get font info

  \param[out] list list of font info
  \param[out] number of items in the list
*/
void D_font_info(char ***list, int *count)
{
    COM_Font_info(list, count);
}

/*!
 * \brief get graphical clipping window
 *
 * Queries the graphical clipping window (origin is top right)
 *
 *  \param[out] t top edge of clip window
 *  \param[out] b bottom edge of clip window
 *  \param[out] l left edge of clip window
 *  \param[out] r right edge of clip window
 *  \return ~
 */

void D_get_clip_window(double *t, double *b, double *l, double *r)
{
    COM_Get_window(t, b, l, r);
}

/*!
 * \brief set graphical clipping window
 *
 * Sets the graphical clipping window to the specified rectangle
 *  (origin is top right)
 *
 *  \param t top edge of clip window
 *  \param b bottom edge of clip window
 *  \param l left edge of clip window
 *  \param r right edge of clip window
 *  \return ~
 */

void D_set_clip_window(double t, double b, double l, double r)
{
    if (t < frame.t) t = frame.t;
    if (b > frame.b) b = frame.b;
    if (l < frame.l) l = frame.l;
    if (r > frame.r) r = frame.r;

    COM_Set_window(t, b, l, r);
}

/*!
 * \brief get graphical window (frame)
 *
 * Queries the graphical frame (origin is top right)
 *
 *  \param[out] t top edge of frame
 *  \param[out] b bottom edge of frame
 *  \param[out] l left edge of frame
 *  \param[out] r right edge of frame
 *  \return ~
 */

void D_get_frame(double *t, double *b, double *l, double *r)
{
    *t = frame.t;
    *b = frame.b;
    *l = frame.l;
    *r = frame.r;
}

/*!
 * \brief get screen bounds
 *
 * Queries the screen bounds (origin is top right)
 *
 *  \param[out] t top edge of screen
 *  \param[out] b bottom edge of screen
 *  \param[out] l left edge of screen
 *  \param[out] r right edge of screen
 *  \return ~
 */

void D_get_screen(double *t, double *b, double *l, double *r)
{
    *t = screen.t;
    *b = screen.b;
    *l = screen.l;
    *r = screen.r;
}

/*!
 * \brief set graphical clipping window to map window
 *
 * Sets the graphical clipping window to the pixel window that corresponds
 * to the current database region.
 *
 *  \param ~
 *  \return ~
 */

void D_set_clip_window_to_map_window(void)
{
    D_set_clip_window(
	D_get_d_north(), D_get_d_south(),
	D_get_d_west(), D_get_d_east());
}

/*!
 * \brief set clipping window to screen window
 *
 * Sets the clipping window to the pixel window that corresponds to the
 * full screen window. Off screen rendering is still clipped.
 *
 *  \param ~
 *  \return ~
 */

void D_set_clip_window_to_screen_window(void)
{
    COM_Set_window(frame.t, frame.b, frame.l, frame.r);
}

