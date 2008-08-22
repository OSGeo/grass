#include "cairodriver.h"

#if CAIRO_HAS_XLIB_SURFACE
#include <X11/Xlib.h>
#include <cairo-xlib.h>
#endif

void cairo_write_image(void)
{
    G_debug(1, "write_image");

    if (!ca.modified)
	return;

    if (ca.mapped)
	return;

    if (!cairo || !surface)
	return;

    if (ca.file_type == FTYPE_PPM) {
	G_debug(1, "Writing image to %s", ca.file_name);
	cairo_write_ppm();
    }
    else if (ca.file_type == FTYPE_BMP) {
	G_debug(1, "Writing image to %s", ca.file_name);
	cairo_write_bmp();
    }
#if CAIRO_HAS_PNG_FUNCTIONS
    else if (ca.file_type == FTYPE_PNG) {
	G_debug(1, "Writing image to %s", ca.file_name);
	cairo_surface_write_to_png(surface, ca.file_name);
    }
#endif
#if CAIRO_HAS_XLIB_SURFACE
    else if (ca.file_type == FTYPE_X11) {
	XFlush(cairo_xlib_surface_get_display(surface));
    }
#endif
    /* vector format files are written directly to file */

    ca.modified = 0;
}
