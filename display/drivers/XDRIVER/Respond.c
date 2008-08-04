
#include "XDRIVER.h"
#include "includes.h"

void XD_Respond(void)
{
    XClearWindow(dpy, grwin);
    XSync(dpy, 1);
    needs_flush = 0;
}
