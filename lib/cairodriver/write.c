#include "cairodriver.h"

#if CAIRO_HAS_XLIB_SURFACE
#include <X11/Xlib.h>
#include <cairo-xlib.h>
#endif

void write_image(void)
{
    G_debug(1, "write_image");

    if (!modified)
	return;

    if (mapped)
	return;

    if (!cairo || !surface)
	return;

    if (file_type == FTYPE_PPM) {
	G_debug(1, "Writing image to %s", file_name);
	write_ppm();
    }
    else if (file_type == FTYPE_BMP) {
	G_debug(1, "Writing image to %s", file_name);
	write_bmp();
    }
#if CAIRO_HAS_PNG_FUNCTIONS
    else if (file_type == FTYPE_PNG) {
	G_debug(1, "Writing image to %s", file_name);
	cairo_surface_write_to_png(surface, file_name);
    }
#endif
#if CAIRO_HAS_XLIB_SURFACE
    else if (file_type == FTYPE_X11) {
	XFlush(cairo_xlib_surface_get_display(surface));
    }
#endif
    /* vector format files are written directly to file */

    modified = 0;
}
