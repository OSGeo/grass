#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include "includes.h"
#include "XDRIVER.h"

void XD_draw_bitmap(int ncols, int nrows, int threshold,
		    const unsigned char *array)
{
    static GC and_gc, or_gc;
    static int have_gcs;
    XImage *grimage;
    XWindowAttributes xwa;
    int bytes_per_line = (ncols + 7) / 8;
    int bytes = bytes_per_line * nrows;
    unsigned char *data = G_malloc(bytes);
    int i, j;

    if (!XGetWindowAttributes(dpy, grwin, &xwa))
	return;

    if (!have_gcs) {
	XGCValues new;

	new.background = ~0UL;
	new.foreground = 0UL;
	new.function = GXand;
	and_gc =
	    XCreateGC(dpy, grwin, GCForeground | GCBackground | GCFunction,
		      &new);

	new.background = 0UL;
	new.function = GXor;
	or_gc = XCreateGC(dpy, grwin, GCBackground | GCFunction, &new);

	have_gcs = 1;
    }

    grimage = XCreateImage(dpy, xwa.visual, 1, XYBitmap,
			   0, data, ncols, nrows, 8, bytes_per_line);

    for (j = 0; j < nrows; j++)
	for (i = 0; i < ncols; i++)
	    XPutPixel(grimage, i, j,
		      array[j * ncols + i] > threshold ? 1 : 0);

    XPutImage(dpy, bkupmap, and_gc, grimage, 0, 0, cur_x, cur_y, ncols,
	      nrows);

    XSetForeground(dpy, or_gc, current_color);
    XPutImage(dpy, bkupmap, or_gc, grimage, 0, 0, cur_x, cur_y, ncols, nrows);

    XDestroyImage(grimage);

    needs_flush = 1;
}
