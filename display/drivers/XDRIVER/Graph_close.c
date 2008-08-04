/* Close down the graphics processing.  This gets called only at driver
 * termination time. */

#include "includes.h"

void XD_Graph_close(void)
{
    XFreePixmap(dpy, bkupmap);
    XDestroyWindow(dpy, grwin);
}
