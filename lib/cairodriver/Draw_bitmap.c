/*!
  \file cairodriver/Draw_bitmap.c

  \brief GRASS cairo display driver - draw bitmap

  (C) 2007-2008 by Lars Ahlzen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
  \author Glynn Clements  
*/

#include <grass/glocale.h>

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

/*!
  \brief Draw bitmap

  \param ncols,nrows number of columns and rows
  \param threshold threshold value
  \param buf data buffer
*/
void Cairo_Bitmap(int ncols, int nrows, int threshold,
		       const unsigned char *buf)
{
    cairo_surface_t *surf;

    G_debug(1, "Cairo_Bitmap: %d %d %d", ncols, nrows, threshold);

    surf = cairo_image_surface_create_for_data((unsigned char *)buf,
					       CAIRO_FORMAT_A8, ncols, nrows,
					       ncols);

    if (cairo_surface_status(surf) != CAIRO_STATUS_SUCCESS)
	G_fatal_error(_("Cairo_Bitmap: Failed to create source"));

    surf = fix_surface(surf);

    cairo_mask_surface(cairo, surf, cur_x, cur_y);

    cairo_surface_destroy(surf);
    ca.modified = 1;
}
