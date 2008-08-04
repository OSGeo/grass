
#include "XDRIVER.h"
#include "includes.h"

#define LOOP_PER_SERVICE 15

int XD_Work_stream(void)
{
    return ConnectionNumber(dpy);
}

void XD_Do_work(int opened)
{
    static int cmd_loop_count;

    if (opened) {
	if (--cmd_loop_count <= 0) {
	    service_xevent(opened);	/* take care of any events */
	    cmd_loop_count = LOOP_PER_SERVICE;
	}
    }
    else {
	service_xevent(opened);
	XNoOp(dpy);		/* see if X is still running */
	cmd_loop_count = 0;
    }
}
