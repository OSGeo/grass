#include "cairodriver.h"

void read_image(void)
{
    G_debug(1, "read_image");

    if (!cairo || !surface)
	return;

    if (file_type == FTYPE_PPM) {
	G_debug(1, "Reading image from %s", file_name);
	read_ppm();
    }
    else if (file_type == FTYPE_BMP) {
	G_debug(1, "Reading image from %s", file_name);
	read_bmp();
    }
#if CAIRO_HAS_PNG_FUNCTIONS
    else if (file_type == FTYPE_PNG) {
	cairo_surface_t *img_surf;

	G_debug(1, "Reading image from %s", file_name);

	img_surf = cairo_image_surface_create_from_png(file_name);
	if (!img_surf)
	    return;

	cairo_save(cairo);
	cairo_set_source_surface(cairo, img_surf, 0, 0);
	cairo_paint(cairo);
	cairo_restore(cairo);

	cairo_surface_destroy(img_surf);
    }
#endif
    /* vector format files are written directly to file */

    modified = 0;
}
