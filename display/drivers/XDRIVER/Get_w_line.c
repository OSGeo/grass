/* Using mouse device, get a new screen coordinate and button number.
 * Button numbers must be the following values which correspond to the
 * following software meanings: 1 - left button 2 - middle button 3 -
 * right button
 * 
 * This is called directly by the application programs.
 * 
 * A "rubberband" line is used.  One end is fixed at the (cx, cy)
 * coordinate. The opposite end starts out at (*nx, *ny) and then
 * tracks the mouse. Upon button depression, the current coordinate is
 * returned in (*nx, *ny) and the button pressed in returned in
 * *button. */

#include <grass/gis.h>
#include "includes.h"
#include <grass/glocale.h>

/* Returns: -1  error
 *            0
 */
int XD_Get_location_with_line(int cx, int cy,	/* current x and y */
			      int *nx, int *ny,	/* new x and y */
			      int *button)
{
    int drawn = 0;
    long event_mask;
    int oldx, oldy;
    XEvent event;
    GC xor_gc;
    XGCValues gcValues;
    unsigned gcMask;
    int done;

    if (redraw_pid) {
	G_warning(_("Monitor: interactive command in redraw"));
	return -1;
    }

    G_debug(5, "Get_location_with_line()");

    /* Get events that track the pointer to resize the RubberBox until
     * ButtonReleased */
    event_mask = ButtonPressMask | PointerMotionMask;
    XSelectInput(dpy, grwin, event_mask);

    /* XOR, so double drawing returns pixels to original state */
    gcMask = GCFunction | GCPlaneMask | GCForeground | GCLineWidth;
    gcValues.function = GXxor;
    gcValues.line_width = 1;
    gcValues.plane_mask = BlackPixel(dpy, scrn) ^ WhitePixel(dpy, scrn);
    gcValues.foreground = 0xffffffff;
    xor_gc = XCreateGC(dpy, grwin, gcMask, &gcValues);

    /* Set the crosshair cursor */
    XDefineCursor(dpy, grwin, cur_xh);

    for (done = 0; !done;) {
	if (!get_xevent(event_mask, &event))
	    break;

	switch (event.type) {
	case ButtonPress:
	    *button = event.xbutton.button;
	    *nx = event.xbutton.x;
	    *ny = event.xbutton.y;
	    done = 1;
	    break;

	case MotionNotify:
	    *nx = event.xbutton.x;
	    *ny = event.xbutton.y;
	    /* do a double draw to 'erase' previous rectangle */
	    if (drawn)
		XDrawLine(dpy, grwin, xor_gc, cx, cy, oldx, oldy);
	    XDrawLine(dpy, grwin, xor_gc, cx, cy, *nx, *ny);
	    oldx = *nx;
	    oldy = *ny;
	    drawn = 1;
	    break;
	}
    }

    /* delete old line if any */
    if (drawn)
	XDrawLine(dpy, grwin, xor_gc, cx, cy, oldx, oldy);
    drawn = 0;
    XUndefineCursor(dpy, grwin);	/* reset cursor */
    XSelectInput(dpy, grwin, gemask);	/* restore normal events */
    XFreeGC(dpy, xor_gc);

    return 0;
}
