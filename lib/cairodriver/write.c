/*!
   \file lib/cairodriver/write.c

   \brief GRASS cairo display driver - write image (lower level functions)

   (C) 2007-2008 by Lars Ahlzen and the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

<<<<<<< HEAD
   \author Lars Ahlzen <lars ahlzen.com> (original contributor)
=======
   \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
   \author Glynn Clements
 */

#include "cairodriver.h"

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
#if CAIRO_HAS_XLIB_XRENDER_SURFACE
    else if (ca.file_type == FTYPE_X11) {
        G_debug(1, "Writing XID to %s", ca.file_name);
        cairo_write_xid();
    }
#endif
    /* vector format files are written directly to file */

    ca.modified = 0;
}
