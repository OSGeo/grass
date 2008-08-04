#include "includes.h"

void XD_Set_window(int t, int b, int l, int r)
{
    XRectangle rect;

    rect.x = rect.y = 0;
    rect.width = r - l;
    rect.height = b - t;

    XSetClipRectangles(dpy, gc, l, t, &rect, 1, YXBanded);
}
