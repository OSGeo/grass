#include <grass/config.h>

#ifdef HAVE_SOCKET

#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/graphics.h>

#include "transport.h"

/*!
 * \brief screen left edge
 *
 * Returns the coordinate of the left edge of the screen.
 *
 *  \param void
 *  \return int
 */

int REM_screen_left(void)
{
    int l;

    _send_ident(SCREEN_LEFT);
    _get_int(&l);
    return l;
}

/*!
 * \brief screen right edge
 *
 * Returns the coordinate of the right edge of the screen.
 *
 *  \param void
 *  \return int
 */

int REM_screen_rite(void)
{
    int r;

    _send_ident(SCREEN_RITE);
    _get_int(&r);
    return r;
}


/*!
 * \brief bottom of screen
 *
 * Returns the coordinate of the bottom of the screen.
 *
 *  \param void
 *  \return int
 */

int REM_screen_bot(void)
{
    int b;

    _send_ident(SCREEN_BOT);
    _get_int(&b);
    return b;
}


/*!
 * \brief top of screen
 *
 * Returns the coordinate of the top of the screen.
 *
 *  \param void
 *  \return int
 */

int REM_screen_top(void)
{
    int t;

    _send_ident(SCREEN_TOP);
    _get_int(&t);
    return t;
}

void REM_get_num_colors(int *n)
{
    _send_ident(GET_NUM_COLORS);
    _get_int(n);
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
 */

void REM_standard_color(int index)
{
    _send_ident(STANDARD_COLOR);
    _send_int(&index);
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
 */

void REM_RGB_color(unsigned char red, unsigned char grn, unsigned char blu)
{
    _send_ident(RGB_COLOR);
    _send_char(&red);
    _send_char(&grn);
    _send_char(&blu);
}

/*!
 * \brief change the width of line
 *
 * Changes the <b>width</b> of line to be used in subsequent draw commands.
 *
 *  \param width
 */

void REM_line_width(int width)
{
    _send_ident(LINE_WIDTH);
    _send_int(&width);
}

/*!
 * \brief erase screen
 *
 * Erases the entire screen to black.
 *
 *  \param void
 */

void REM_erase(void)
{
    _send_ident(ERASE);
}

/*!
 * \brief move current location
 *
 * Move the current location to the absolute screen coordinate <b>x,y.</b>
 * Nothing is drawn on the screen.
 *
 *  \param x
 *  \param y
 */

void REM_move_abs(int x, int y)
{
    _send_ident(MOVE_ABS);
    _send_int(&x);
    _send_int(&y);
}

/*!
 * \brief move current location
 *
 * Shift the current screen location by the values in <b>dx</b> and <b>dy</b>:
 \code
 Newx = Oldx + dx;
 Newy = Oldy + dy;
 \endcode
 * Nothing is drawn on the screen.
 *
 *  \param x dx
 *  \param y dy
 */

void REM_move_rel(int x, int y)
{
    _send_ident(MOVE_REL);
    _send_int(&x);
    _send_int(&y);
}

/*!
 * \brief draw line
 *
 * Draw a line using the current color, selected via <i>R_color</i>, from the 
 * current location to the location specified by <b>x,y.</b> The current location
 * is updated to <b>x,y.</b>
 *
 *  \param x
 *  \param y
 */

void REM_cont_abs(int x, int y)
{
    _send_ident(CONT_ABS);
    _send_int(&x);
    _send_int(&y);
}

/*!
 * \brief draw line
 *
 * Draw a line using the
 * current color, selected via <i>R_color</i>, from the current location to
 * the relative location specified by <b>x</b> and <b>y.</b> The current
 * location is updated:
 \code
 Newx = Oldx + x;
 Newy = Oldy + y;
 \endcode
 *
 *  \param x
 *  \param y
 */

void REM_cont_rel(int x, int y)
{
    _send_ident(CONT_REL);
    _send_int(&x);
    _send_int(&y);
}

/*!
 * \brief draw a series of dots
 *
 * Pixels at the <b>num</b> absolute positions in the <b>x</b> and
 * <b>y</b> arrays are turned to the current color. The current location is
 * left updated to the position of the last dot.
 *
 *  \param xarray x
 *  \param yarray y
 *  \param number
 */

void REM_polydots_abs(const int *xarray, const int *yarray, int number)
{
    _send_ident(POLYDOTS_ABS);
    _send_int(&number);
    _send_int_array(number, xarray);
    _send_int_array(number, yarray);
}

/*!
 * \brief draw a series of dots
 *
 * Pixels at the <b>number</b> relative positions in the <b>x</b> and
 * <b>y</b> arrays are turned to the current color. The first position is
 * relative to the starting current location; the succeeding positions are then
 * relative to the previous position. The current location is updated to the
 * position of the last dot.
 *
 *  \param xarray x
 *  \param yarray y
 *  \param number
 */

void REM_polydots_rel(const int *xarray, const int *yarray, int number)
{
    _send_ident(POLYDOTS_REL);
    _send_int(&number);
    _send_int_array(number, xarray);
    _send_int_array(number, yarray);
}

/*!
 * \brief draw an open polygon
 *
 * The <b>number</b> absolute positions in the <b>x</b> and <b>y</b>
 * arrays are used to generate a multisegment line (often curved). This line is
 * drawn with the current color. The current location is left updated to the
 * position of the last point.
 * <b>Note.</b> It is not assumed that the line is closed, i.e., no line is
 * drawn from the last point to the first point.
 *
 *  \param xarray x
 *  \param yarray y
 *  \param number
 */

void REM_polyline_abs(const int *xarray, const int *yarray, int number)
{
    _send_ident(POLYLINE_ABS);
    _send_int(&number);
    _send_int_array(number, xarray);
    _send_int_array(number, yarray);
}

/*!
 * \brief draw an open polygon
 *
 * The <b>number</b> relative positions in the <b>x</b> and <b>y</b>
 * arrays are used to generate a multisegment line (often curved). The first
 * position is relative to the starting current location; the succeeding
 * positions are then relative to the previous position. The current location is
 * updated to the position of the last point. This line is drawn with the current
 * color.
 * <b>Note.</b> No line is drawn between the last point and the first point.
 *
 *  \param xarray x
 *  \param yarray y
 *  \param number
 */

void REM_polyline_rel(const int *xarray, const int *yarray, int number)
{
    _send_ident(POLYLINE_REL);
    _send_int(&number);
    _send_int_array(number, xarray);
    _send_int_array(number, yarray);
}

/*!
 * \brief draw a closed polygon
 *
 * The <b>number</b> absolute positions in the <b>x</b> and <b>y</b> arrays
 * outline a closed polygon which is filled with the current color. The current
 * location is undefined afterwards.
 *
 *  \param xarray x
 *  \param yarray y
 *  \param number
 */

void REM_polygon_abs(const int *xarray, const int *yarray, int number)
{
    _send_ident(POLYGON_ABS);
    _send_int(&number);
    _send_int_array(number, xarray);
    _send_int_array(number, yarray);
}

/*!
 * \brief draw a closed polygon
 *
 * The <b>number</b> relative positions in the <b>x</b> and <b>y</b>
 * arrays outline a closed polygon which is filled with the current color. The
 * first position is relative to the starting current location; the succeeding
 * positions are then relative to the previous position. The current location is
 * undefined afterwards.
 *
 *  \param xarray x
 *  \param yarray y
 *  \param number
 */

void REM_polygon_rel(const int *xarray, const int *yarray, int number)
{
    _send_ident(POLYGON_REL);
    _send_int(&number);
    _send_int_array(number, xarray);
    _send_int_array(number, yarray);
}

/*!
 * \brief fill a box
 *
 * A box is drawn in the current color using the coordinates <b>x1,y1</b> and
 * <b>x2,y2</b> as opposite corners of the box. The current location is undefined
 * afterwards
 *
 *  \param x1
 *  \param y1
 *  \param x2
 *  \param y2
 */

void REM_box_abs(int x1, int y1, int x2, int y2)
{
    _send_ident(BOX_ABS);
    _send_int(&x1);
    _send_int(&y1);
    _send_int(&x2);
    _send_int(&y2);
}


/*!
 * \brief fill a box
 *
 * A box is drawn in the current color using the current location as one corner 
 * and the current location plus <b>x</b> and <b>y</b> as the opposite corner 
 * of the box. The current location is undefined afterwards.
 *
 *  \param x
 *  \param y
 */

void REM_box_rel(int x, int y)
{
    _send_ident(BOX_REL);
    _send_int(&x);
    _send_int(&y);
}

/*!
 * \brief set text size
 *
 * Sets text pixel width and height to <b>width</b> and <b>height.</b>
 *
 *  \param width
 *  \param height
 */

void REM_text_size(int width, int height)
{
    _send_ident(TEXT_SIZE);
    _send_int(&width);
    _send_int(&height);
}

void REM_text_rotation(float rotation)
{
    _send_ident(TEXT_ROTATION);
    _send_float(&rotation);
}

/*!
 * \brief set text clipping frame
 *
 * Subsequent calls to <i>R_text</i> will have text strings
 * clipped to the screen frame defined by <b>top, bottom, left, right.</b>
 *
 *  \param t top
 *  \param b bottom
 *  \param l left
 *  \param r right
 */

void REM_set_window(int t, int b, int l, int r)
{
    _send_ident(SET_WINDOW);
    _send_int(&t);
    _send_int(&b);
    _send_int(&l);
    _send_int(&r);
}

/*!
 * \brief write text
 *
 * Writes <b>text</b> in the current color and font, at the current text
 * width and height, starting at the current screen location.
 *
 *  \param sometext
 */

void REM_text(const char *sometext)
{
    _send_ident(TEXT);
    _send_text(sometext);
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
 */

void REM_get_text_box(const char *sometext, int *t, int *b, int *l, int *r)
{
    _send_ident(GET_TEXT_BOX);
    _send_text(sometext);
    _get_int(t);
    _get_int(b);
    _get_int(l);
    _get_int(r);
}

/*!
 * \brief choose font
 *
 * Set current font to <b>font name</b>. Available fonts are:
 * 
 <table>
 <tr><td><b>Font Name</b></td><td><b>Description</b></td></tr>
 <tr><td>cyrilc </td><td> cyrillic</td></tr>
 <tr><td>gothgbt </td><td> Gothic Great Britain triplex</td></tr>
 <tr><td>gothgrt </td><td>  Gothic German triplex</td></tr>
 <tr><td>gothitt </td><td>  Gothic Italian triplex</td></tr>
 <tr><td>greekc </td><td> Greek complex</td></tr>
 <tr><td>greekcs </td><td> Greek complex script</td></tr>
 <tr><td>greekp </td><td> Greek plain</td></tr>
 <tr><td>greeks </td><td> Greek simplex</td></tr>
 <tr><td>italicc </td><td>  Italian complex</td></tr>
 <tr><td>italiccs </td><td> Italian complex small</td></tr>
 <tr><td>italict </td><td> Italian triplex</td></tr>
 <tr><td>romanc </td><td> Roman complex</td></tr>
 <tr><td>romancs </td><td> Roman complex small</td></tr>
 <tr><td>romand </td><td> Roman duplex</td></tr>
 <tr><td>romanp </td><td> Roman plain</td></tr>
 <tr><td>romans </td><td> Roman simplex</td></tr>
 <tr><td>romant </td><td> Roman triplex</td></tr>
 <tr><td>scriptc </td><td> Script complex</td></tr>
 <tr><td>scripts </td><td> Script simplex</td></tr>
 </table>
 *
 *  \param name
 */

void REM_font(const char *name)
{
    _send_ident(FONT);
    _send_text(name);
}

void REM_charset(const char *name)
{
    _send_ident(CHARSET);
    _send_text(name);
}

static void font_list(char ***list, int *count, int op)
{
    char **fonts;
    int num_fonts;
    int i;

    _send_ident(op);
    _get_int(&num_fonts);

    fonts = G_malloc(num_fonts * sizeof(char *));
    for (i = 0; i < num_fonts; i++)
	fonts[i] = G_store(_get_text_2());

    *list = fonts;
    *count = num_fonts;
}

void REM_font_list(char ***list, int *count)
{
    font_list(list, count, FONT_LIST);
}

void REM_font_info(char ***list, int *count)
{
    font_list(list, count, FONT_INFO);
}

void REM_panel_save(const char *name, int t, int b, int l, int r)
{
    close(creat(name, 0666));

    _send_ident(PANEL_SAVE);
    _send_text(name);
    _send_int(&t);
    _send_int(&b);
    _send_int(&l);
    _send_int(&r);
    R_stabilize();
}

void REM_panel_restore(const char *name)
{
    _send_ident(PANEL_RESTORE);
    _send_text(name);
    R_stabilize();
}

void REM_panel_delete(const char *name)
{
    _send_ident(PANEL_DELETE);
    _send_text(name);
    R_stabilize();

    unlink(name);
}

void REM_begin_scaled_raster(int mask, int src[2][2], int dst[2][2])
{
    _send_ident(BEGIN_SCALED_RASTER);
    _send_int(&mask);
    _send_int_array(4, &src[0][0]);
    _send_int_array(4, &dst[0][0]);
}

int REM_scaled_raster(int n, int row,
		      const unsigned char *red, const unsigned char *grn,
		      const unsigned char *blu, const unsigned char *nul)
{
    int z = !!nul;
    int t;

    _send_ident(SCALED_RASTER);
    _send_int(&n);
    _send_int(&row);
    _send_char_array(n, red);
    _send_char_array(n, grn);
    _send_char_array(n, blu);
    _send_char_array(n, nul ? nul : red);
    _send_int(&z);
    _get_int(&t);
    return t;
}

void REM_end_scaled_raster(void)
{
    _send_ident(END_SCALED_RASTER);
}

void REM_bitmap(int ncols, int nrows, int threshold, const unsigned char *buf)
{
    _send_ident(BITMAP);
    _send_int(&ncols);
    _send_int(&nrows);
    _send_int(&threshold);
    _send_char_array(ncols * nrows, buf);
}

#endif /* HAVE_SOCKET */
