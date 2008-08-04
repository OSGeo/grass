#include <grass/config.h>

#ifdef HAVE_SOCKET

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/graphics.h>

#include "transport.h"

/*!
 * \brief get mouse location using a box
 *
 * Identical to <i>R_get_location_with_line</i> except a rubber-band box is
 * used instead of a rubber-band line.
 * <b>button</b> is set to: 1 - left, 2 - middle, 3 - right
 *
 *  \param cx
 *  \param cy
 *  \param wx
 *  \param wy
 *  \param button
 *  \return ~
 */

void REM_get_location_with_box(int cx, int cy, int *wx, int *wy, int *button)
{
    _send_ident(GET_LOCATION_WITH_BOX);
    _send_int(&cx);
    _send_int(&cy);
    _send_int(wx);
    _send_int(wy);
    _get_int(wx);
    _get_int(wy);
    _get_int(button);
}

/*!
 * \brief get mouse location using a line
 *
 * Similar to <i>R_get_location_with_pointer</i> except the pointer is
 * replaced by a line which has one end fixed at the coordinate identified
 * by the <b>x,y</b> values. The other end of the line is initialized at the
 * coordinate identified by the <b>nx,ny</b> pointers. This end then tracks
 * the mouse until a button is pressed. The mouse button (1 for left, 2 for
 * middle, and 3 for right) is returned in the <b>button</b> pointer.
 *
 *  \param cx
 *  \param cy
 *  \param wx
 *  \param wy
 *  \param button
 *  \return ~
 */

void REM_get_location_with_line(int cx, int cy, int *wx, int *wy, int *button)
{
    _send_ident(GET_LOCATION_WITH_LINE);
    _send_int(&cx);
    _send_int(&cy);
    _send_int(wx);
    _send_int(wy);
    _get_int(wx);
    _get_int(wy);
    _get_int(button);
}

/*!
 * \brief get mouse location using pointer
 *
 * A cursor is put on the screen at the location specified by the coordinate
 * found at the <b>wx,wy</b> pointers. This cursor tracks the mouse (or
 * other pointing device) until one of three mouse buttons are pressed. Upon
 * pressing, the cursor is removed from the screen, the current mouse
 * coordinates are returned by the <b>wx</b> and <b>wy</b> pointers, and the
 * mouse button (1 for left, 2 for middle, and 3 for right) is returned in
 * the <b>button</b> pointer.
 *
 *  \param wx
 *  \param wy
 *  \param button
 *  \return ~
 */

void REM_get_location_with_pointer(int *wx, int *wy, int *button)
{
    *button = 0;		/* ?, how button = -1 is used (see driver) */

    _send_ident(GET_LOCATION_WITH_POINTER);
    _send_int(wx);
    _send_int(wy);
    _send_int(button);
    _get_int(wx);
    _get_int(wy);
    _get_int(button);
}

#endif /* HAVE_SOCKET */
