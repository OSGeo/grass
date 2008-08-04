#include "includes.h"

void XD_Line_width(int width)
{
    XGCValues gc_values;

    gc_values.line_width = (width < 0 ? 0 : width);
    gc_values.cap_style = CapRound;
    XChangeGC(dpy, gc, GCLineWidth | GCCapStyle, &gc_values);
}
