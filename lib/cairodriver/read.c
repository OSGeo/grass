/*!
  \file lib/cairodriver/read.c

  \brief GRASS cairo display driver - read image (lower level functions)

  (C) 2007-2008, 2011 by Lars Ahlzen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
  \author Glynn Clements  
*/

#include "cairodriver.h"

void cairo_read_image(void)
{
    G_debug(1, "read_image");

    if (!cairo || !surface)
	return;

    if (ca.file_type == FTYPE_PPM) {
	G_debug(1, "Reading image from %s", ca.file_name);
	cairo_read_ppm();
    }
    else if (ca.file_type == FTYPE_BMP) {
	G_debug(1, "Reading image from %s", ca.file_name);
	cairo_read_bmp();
    }
#if CAIRO_HAS_PNG_FUNCTIONS
    else if (ca.file_type == FTYPE_PNG) {
	cairo_surface_t *img_surf;

	G_debug(1, "Reading image from %s", ca.file_name);

	img_surf = cairo_image_surface_create_from_png(ca.file_name);
	if (!img_surf)
	    return;

	cairo_save(cairo);
	cairo_set_source_surface(cairo, img_surf, 0, 0);
	cairo_paint(cairo);
	cairo_restore(cairo);

	cairo_surface_destroy(img_surf);
    }
#endif
#if CAIRO_HAS_XLIB_XRENDER_SURFACE
    else if (ca.file_type == FTYPE_X11) {
	G_debug(1, "Reading XID from %s", ca.file_name);
	cairo_read_xid();
    }
#endif
    /* vector format files are written directly to file */

    ca.modified = 0;
}
