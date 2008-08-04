#include <stdio.h>
#include "includes.h"
#include "XDRIVER.h"

void XD_Box_abs(int x1, int y1, int x2, int y2)
{
    int tmp;

    if (x1 > x2) {
	tmp = x1;
	x1 = x2;
	x2 = tmp;
    }
    if (y1 > y2) {
	tmp = y1;
	y1 = y2;
	y2 = tmp;
    }

    XFillRectangle(dpy, bkupmap, gc, x1, y1, x2 - x1, y2 - y1);
    needs_flush = 1;
}
