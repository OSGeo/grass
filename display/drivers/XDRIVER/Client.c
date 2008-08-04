
#include <unistd.h>

#include "XDRIVER.h"
#include "includes.h"

#ifdef X11R3
extern XSizeHints *XAllocSizeHints(void);
#endif

static XSizeHints *szhints;

static void set_size(int minmax)
{
    XWindowAttributes xwa;

    if (!szhints)
	szhints = XAllocSizeHints();

    if (!XGetWindowAttributes(dpy, grwin, &xwa))
	return;

    szhints->flags = PSize;
    szhints->width = xwa.width;
    szhints->height = xwa.height;
    szhints->min_width = xwa.width;
    szhints->min_height = xwa.height;
    szhints->max_width = xwa.width;
    szhints->max_height = xwa.height;

    if (minmax)
	szhints->flags |= PMinSize | PMaxSize;

    XSetWMNormalHints(dpy, grwin, szhints);
    XFlush(dpy);
}

void XD_Client_Open(void)
{
    set_size(!redraw_pid);
}

void XD_Client_Close(void)
{
    set_size(0);
    if (redraw_pid)
	usleep(50000);
    service_xevent(0);
}
