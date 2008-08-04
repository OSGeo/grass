#include "includes.h"
#include "XDRIVER.h"

/* A polygon is drawn using the current color.  It has "number"
 * verticies which are found in the absolute coordinate pairs
 * represented in the "xarray" and "yarray" arrays.  NOTE: Cursor
 * location is NOT updated in Polygon_rel(). */

void XD_Polygon_abs(const int *xarray, const int *yarray, int number)
{
    XPoint *xpnts = alloc_xpoints(number);
    int i;

    if (number < 1)
	return;

    for (i = 0; i < number; i++) {
	xpnts[i].x = (short)xarray[i];
	xpnts[i].y = (short)yarray[i];
    }

    XFillPolygon(dpy, bkupmap, gc, xpnts, number, Complex, CoordModeOrigin);
    needs_flush = 1;
}

void XD_Polygon_rel(const int *xarray, const int *yarray, int number)
{
    XPoint *xpnts = alloc_xpoints(number);
    int i;

    if (number < 1)
	return;

    xpnts[0].x = (short)(xarray[0] + cur_x);
    xpnts[0].y = (short)(yarray[0] + cur_y);

    for (i = 1; i < number; i++) {
	xpnts[i].x = (short)xarray[i];
	xpnts[i].y = (short)yarray[i];
    }

    XFillPolygon(dpy, bkupmap, gc, xpnts, number, Complex, CoordModePrevious);
    needs_flush = 1;
}
