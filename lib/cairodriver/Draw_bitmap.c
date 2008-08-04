#include "cairodriver.h"

static cairo_surface_t *fix_surface(cairo_surface_t * src)
{
    int width = cairo_image_surface_get_width(src);
    int height = cairo_image_surface_get_height(src);
    int stride = cairo_image_surface_get_stride(src);
    cairo_format_t format = cairo_image_surface_get_format(src);
    unsigned char *data = cairo_image_surface_get_data(src);
    cairo_surface_t *dst = cairo_image_surface_create(format, width, height);
    int stride2 = cairo_image_surface_get_stride(dst);
    unsigned char *data2 = cairo_image_surface_get_data(dst);
    int i;

    for (i = 0; i < height; i++) {
	void *p = data + i * stride;
	void *q = data2 + i * stride2;
	int n = stride < stride2 ? stride : stride2;

	memcpy(q, p, n);
    }

    cairo_surface_destroy(src);
    return dst;
}

void Cairo_draw_bitmap(int ncols, int nrows, int threshold,
		       const unsigned char *buf)
{
    cairo_surface_t *surf;

    G_debug(1, "Cairo_draw_bitmap: %d %d %d", ncols, nrows, threshold);

    surf = cairo_image_surface_create_for_data((unsigned char *)buf,
					       CAIRO_FORMAT_A8, ncols, nrows,
					       ncols);

    if (cairo_surface_status(surf) != CAIRO_STATUS_SUCCESS)
	G_fatal_error("Cairo_draw_bitmap: Failed to create source");

    surf = fix_surface(surf);

    cairo_mask_surface(cairo, surf, cur_x, cur_y);

    cairo_surface_destroy(surf);
    modified = 1;
}
