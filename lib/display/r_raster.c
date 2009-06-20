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
#include <grass/display_raster.h>

#include "driver.h"


extern const struct driver *PNG_Driver(void);
extern const struct driver *PS_Driver(void);
extern const struct driver *HTML_Driver(void);
#ifdef USE_CAIRO
extern const struct driver *Cairo_Driver(void);
#endif

static void set_window(double, double, double, double);

static void init(void)
{
    const char *fenc = getenv("GRASS_ENCODING");
    const char *font = getenv("GRASS_FONT");
    const char *line_width = getenv("GRASS_LINE_WIDTH");
    const char *text_size = getenv("GRASS_TEXT_SIZE");
    const char *frame = getenv("GRASS_FRAME");

    R_font(font ? font : "romans");

    if (fenc)
	R_encoding(fenc);

    if (line_width)
	R__line_width(atof(line_width));

    if (text_size) {
	double s = atof(text_size);
	R_text_size(s, s);
    }

    R_text_rotation(0);

    if (frame) {
	double t, b, l, r;
	sscanf(frame, "%lf,%lf,%lf,%lf", &t, &b, &l, &r);
	set_window(t, b, l, r);
    }
}

int R_open_driver(void)
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

void R_close_driver(void)
{
    const char *cmd = getenv("GRASS_NOTIFY");

    COM_Graph_close();

    if (cmd)
	system(cmd);
}

/*!
 * \brief select standard color
 *
 * Selects the
 * standard <b>color</b> to be used in subsequent draw commands.  The
 * <b>color</b> value is best retrieved using <i>D_translate_color.</i>
 * See Display_Graphics_Library.
 *
 *  \param index
 *  \return void
 */

void R__standard_color(int index)
{
    COM_Standard_color(index);
}

/*!
 * \brief select color
 *
 * When in
 * float mode (see <i>R_color_table_float</i>), this call selects the color
 * most closely matched to the <b>red, grn</b>, and <b>blue</b> intensities
 * requested. These values must be in the range of 0-255.
 *
 *  \param red
 *  \param grn
 *  \param blue
 *  \return void
 */

void R__RGB_color(int red, int grn, int blu)
{
    COM_Color_RGB(red, grn, blu);
}

/*!
 * \brief change the width of line
 *
 * Changes the <b>width</b> of line to be used in subsequent draw commands.
 *
 *  \param width
 *  \return void
 */

void R__line_width(double width)
{
    COM_Line_width(width);
}

/*!
 * \brief erase screen
 *
 * Erases the entire screen to black.
 *
 *  \param void
 *  \return void
 */

void R_erase(void)
{
    COM_Erase();
}

/*!
 * \brief move current location
 *
 * Move the current location to the absolute screen coordinate <b>x,y.</b>
 * Nothing is drawn on the screen.
 *
 *  \param x
 *  \param y
 *  \return void
 */

void R__pos_abs(double x, double y)
{
    COM_Pos_abs(x, y);
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

void R_text_size(double width, double height)
{
    COM_Text_size(width, height);
}

void R_text_rotation(double rotation)
{
    COM_Text_rotation(rotation);
}

/*!
 * \brief set clipping frame
 *
 * Subsequent drawing operations will be clipped to the screen frame
 * defined by <b>top, bottom, left, right.</b>
 *
 *  \param t top
 *  \param b bottom
 *  \param l left
 *  \param r right
 *  \return void
 */

void set_window(double t, double b, double l, double r)
{
    COM_Set_window(t, b, l, r);
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

void R_get_window(double *t, double *b, double *l, double *r)
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

void R_text(const char *text)
{
    COM_Text(text);
}

/*!
 * \brief get text extents
 *
 * The extent of the area enclosing the <b>text</b>
 * is returned in the integer pointers <b>top, bottom, left</b>, and
 * <b>right.</b> No text is actually drawn. This is useful for capturing the
 * text extent so that the text location can be prepared with proper background
 * or border.
 *
 *  \param sometext
 *  \param t top
 *  \param b bottom
 *  \param l left
 *  \param r right
 *  \return void
 */

void R__get_text_box(const char *text, double *t, double *b, double *l, double *r)
{
    COM_Get_text_box(text, t, b, l, r);
}

/*!
 * \brief choose font
 *
 * Set current font to <b>font name</b>.
 * 
 *  \param name
 *  \return void
 */

void R_font(const char *name)
{
    COM_Set_font(name);
}

void R_encoding(const char *name)
{
    COM_Set_encoding(name);
}

void R_font_list(char ***list, int *count)
{
    COM_Font_list(list, count);
}

void R_font_info(char ***list, int *count)
{
    COM_Font_info(list, count);
}

void R__begin_scaled_raster(int mask, int src[2][2], double dst[2][2])
{
    COM_begin_raster(mask, src, dst);
}

int R__scaled_raster(int n, int row,
		     const unsigned char *red, const unsigned char *grn,
		     const unsigned char *blu, const unsigned char *nul)
{
    return COM_raster(n, row, red, grn, blu, nul);
}

void R__end_scaled_raster(void)
{
    COM_end_raster();
}

void R__begin(void)
{
    COM_Begin();
}

void R__move(double x, double y)
{
    COM_Move(x, y);
}

void R__cont(double x, double y)
{
    COM_Cont(x, y);
}

void R__close(void)
{
    COM_Close();
}

void R__stroke(void)
{
    COM_Stroke();
}

void R__fill(void)
{
    COM_Fill();
}

void R__point(double x, double y)
{
    COM_Point(x, y);
}

