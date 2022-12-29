/*!
  \file lib/cairodriver/draw_bitmap.c

  \brief GRASS cairo display driver - draw bitmap

  (C) 2007-2008 by Lars Ahlzen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
  \author Glynn Clements  
*/

#include <grass/glocale.h>

#include "cairodriver.h"

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
    int stride;
    unsigned char *data;
    int i;

    G_debug(1, "Cairo_Bitmap: %d %d %d", ncols, nrows, threshold);

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1,5,8)
    stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, ncols);
#else
#define MULTIPLE 4
    stride = (ncols + (MULTIPLE - 1)) / MULTIPLE * MULTIPLE;
#endif
    data = malloc(stride * nrows);
    surf = cairo_image_surface_create_for_data(
	data, CAIRO_FORMAT_A8, ncols, nrows, stride);

    if (cairo_surface_status(surf) != CAIRO_STATUS_SUCCESS)
	G_fatal_error(_("Cairo_Bitmap: Failed to create source"));

    for (i = 0; i < nrows; i++)
	memcpy(&data[i * stride], &buf[i * ncols], ncols);

    cairo_surface_mark_dirty(surf);
    cairo_mask_surface(cairo, surf, cur_x, cur_y);

    cairo_surface_destroy(surf);
    ca.modified = 1;
}

