/*!
  \file lib/cairodriver/raster.c

  \brief GRASS cairo display driver - draw raster

  (C) 2007-2014 by Lars Ahlzen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
  \author Glynn Clements  
*/


#include "cairodriver.h"
#include <grass/glocale.h>

#define MAX_IMAGE_SIZE 32767

static int src_t, src_b, src_l, src_r, src_w, src_h;
static double dst_t, dst_b, dst_l, dst_r, dst_w, dst_h;

static cairo_surface_t *src_surf;
static unsigned char *src_data;
static int src_stride;

static int masked;

/*!
  \brief Start drawing raster

  \todo are top and left swapped?

  \param mask non-zero int for mask
  \param s source (map) extent (left, right, top, bottom)
  \param d destination (image) extent (left, right, top, bottom)
*/
void Cairo_begin_raster(int mask, int s[2][2], double d[2][2])
{
    cairo_status_t status;
    
    masked = mask;

    src_l = s[0][0];
    src_r = s[0][1];
    src_t = s[1][0];
    src_b = s[1][1];

    src_w = src_r - src_l;
    src_h = src_b - src_t;

    dst_l = d[0][0];
    dst_r = d[0][1];
    dst_t = d[1][0];
    dst_b = d[1][1];

    dst_w = dst_r - dst_l;
    dst_h = dst_b - dst_t;

    G_debug(1, "Cairo_begin_raster(): masked=%d, src_lrtb=%d %d %d %d -> w/h=%d/%d, "
            "dst_lrtb=%f %f %f %f -> w/h=%f %f",
            masked, src_l, src_r, src_t, src_b, src_w, src_h,
            dst_l, dst_r, dst_t, dst_b, dst_w, dst_h);

    /* create source surface */
    src_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, src_w, src_h);
    status = cairo_surface_status(src_surf);
    if (status != CAIRO_STATUS_SUCCESS)
        G_fatal_error("%s - %s - size: %dx%d (cairo limit: %dx%d)",
                      _("Failed to create cairo surface"),
                      cairo_status_to_string (status), src_w, src_h,
                      MAX_IMAGE_SIZE, MAX_IMAGE_SIZE);
    
    src_data = cairo_image_surface_get_data(src_surf);
    src_stride = cairo_image_surface_get_stride(src_surf);
}

/*!
  \brief Draw raster row

  \param n number of cell
  \param row raster row (starting at 0)
  \param red,grn,blu,nul red,green,blue and null value

  \return next row
*/
int Cairo_raster(int n, int row,
		 const unsigned char *red, const unsigned char *grn,
		 const unsigned char *blu, const unsigned char *nul)
{
    int i;
    unsigned int *dst;

    dst = (unsigned int *)(src_data + (row - src_t) * src_stride);

    G_debug(3, "Cairo_raster(): n=%d row=%d", n, row);

    for (i = 0; i < n; i++) {
	if (masked && nul && nul[i])
	    *dst++ = 0;
	else {
	    unsigned int r = red[i];
	    unsigned int g = grn[i];
	    unsigned int b = blu[i];
	    unsigned int a = 0xFF;

	    *dst++ = (a << 24) + (r << 16) + (g << 8) + (b << 0);
	}
    }

    return row + 1;
}

/*!
  \brief Finish drawing raster
*/
void Cairo_end_raster(void)
{
    G_debug(1, "Cairo_end_raster()");

    /* paint source surface onto destination (scaled) */
    cairo_save(cairo);
    cairo_translate(cairo, dst_l, dst_t);
    cairo_scale(cairo, dst_w / src_w, dst_h / src_h);
    cairo_surface_mark_dirty(src_surf);
    cairo_set_source_surface(cairo, src_surf, 0, 0);
    cairo_pattern_set_filter(cairo_get_source(cairo), CAIRO_FILTER_NEAREST);
    cairo_paint(cairo);
    cairo_restore(cairo);

    /* cleanup */
    cairo_surface_destroy(src_surf);
    ca.modified = 1;
}
