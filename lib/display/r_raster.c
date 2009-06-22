/*!
  \file display/r_raster.c

  \brief Display Library - Raster graphics subroutines

  (C) 2001-2009 by the GRASS Development Team

  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.

  \author Original author CERL
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

#include "driver.h"


extern const struct driver *PNG_Driver(void);
extern const struct driver *PS_Driver(void);
extern const struct driver *HTML_Driver(void);
#ifdef USE_CAIRO
extern const struct driver *Cairo_Driver(void);
#endif

static void init(void)
{
    const char *fenc = getenv("GRASS_ENCODING");
    const char *font = getenv("GRASS_FONT");
    const char *line_width = getenv("GRASS_LINE_WIDTH");
    const char *text_size = getenv("GRASS_TEXT_SIZE");
    const char *frame = getenv("GRASS_FRAME");

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

    if (frame) {
	double t, b, l, r;
	sscanf(frame, "%lf,%lf,%lf,%lf", &t, &b, &l, &r);
	COM_Set_window(t, b, l, r);
    }
}

int D_open_driver(void)
{
    const char *p = getenv("GRASS_RENDER_IMMEDIATE");
    const struct driver *drv =
	(p && G_strcasecmp(p, "PNG") == 0) ? PNG_Driver() :
	(p && G_strcasecmp(p, "PS") == 0) ? PS_Driver() :
	(p && G_strcasecmp(p, "HTML") == 0) ? HTML_Driver() :
#ifdef USE_CAIRO
	(p && G_strcasecmp(p, "cairo") == 0) ? Cairo_Driver() :
	Cairo_Driver();
#else
	PNG_Driver();
#endif

    LIB_init(drv);

    init();

    return 0;
}

void D_close_driver(void)
{
    const char *cmd = getenv("GRASS_NOTIFY");

    COM_Graph_close();

    if (cmd)
	system(cmd);
}

void D__erase(void)
{
    COM_Erase();
}

/*!
 * \brief set text size
 *
 * Sets text pixel width and height to <b>width</b> and <b>height.</b>
 *
 *  \param width
 *  \param height
 *  \return void
 */

void D_text_size(double width, double height)
{
    COM_Text_size(width, height);
}

void D_text_rotation(double rotation)
{
    COM_Text_rotation(rotation);
}

/*!
 * \brief get clipping frame
 *
 * Retrieve clipping frame
 *
 *  \param t top
 *  \param b bottom
 *  \param l left
 *  \param r right
 *  \return void
 */

void D_get_window(double *t, double *b, double *l, double *r)
{
    return COM_Get_window(t, b, l, r);
}

/*!
 * \brief write text
 *
 * Writes <b>text</b> in the current color and font, at the current text
 * width and height, starting at the current screen location.
 *
 *  \param sometext
 *  \return void
 */

void D_text(const char *text)
{
    COM_Text(text);
}

/*!
 * \brief choose font
 *
 * Set current font to <b>font name</b>.
 * 
 *  \param name
 *  \return void
 */

void D_font(const char *name)
{
    COM_Set_font(name);
}

void D_encoding(const char *name)
{
    COM_Set_encoding(name);
}

void D_font_list(char ***list, int *count)
{
    COM_Font_list(list, count);
}

void D_font_info(char ***list, int *count)
{
    COM_Font_info(list, count);
}

