#include <stdio.h>
#include <stdlib.h>
#include "includes.h"
#include "XDRIVER.h"

void XD_Polydots_abs(const int *xarray, const int *yarray, int number)
{
    XPoint *xpnts = alloc_xpoints(number);
    int i;

    if (number < 1)
	return;

    for (i = 0; i < number; i++) {
	xpnts[i].x = (short)xarray[i];
	xpnts[i].y = (short)yarray[i];
    }

    XDrawPoints(dpy, bkupmap, gc, xpnts, number, CoordModeOrigin);
    cur_x = xpnts[number - 1].x;
    cur_y = xpnts[number - 1].y;
    needs_flush = 1;
}

void XD_Polydots_rel(const int *xarray, const int *yarray, int number)
{
    XPoint *xpnts = alloc_xpoints(number);
    int i;

    if (number < 1)
	return;

    xpnts[0].x = (short)(xarray[0] + cur_x);
    xpnts[0].y = (short)(yarray[0] + cur_y);
    cur_x = xpnts[0].x;
    cur_y = xpnts[0].y;

    for (i = 1; i < number; i++) {
	xpnts[i].x = (short)xarray[i];
	xpnts[i].y = (short)yarray[i];
	cur_x += xpnts[i].x;
	cur_y += xpnts[i].y;
    }

    XDrawLines(dpy, bkupmap, gc, xpnts, number, CoordModePrevious);
    needs_flush = 1;
}
