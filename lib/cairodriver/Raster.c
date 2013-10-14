/*!
  \file cairodriver/Raster.c

  \brief GRASS cairo display driver - draw raster

  (C) 2007-2008 by Lars Ahlzen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
  \author Glynn Clements  
*/


#include "cairodriver.h"
#include <grass/glocale.h>

static int src_t, src_b, src_l, src_r, src_w, src_h;
static double dst_t, dst_b, dst_l, dst_r, dst_w, dst_h;

static cairo_surface_t *src_surf;
static unsigned char *src_data;
static int src_stride;

static int masked;

/*!
  \brief Start drawing raster

  \param mask
  \param s
  \param d
*/
void Cairo_begin_raster(int mask, int s[2][2], double d[2][2])
{
    cairo_status_t status;
    
    G_debug(1, "Cairo_begin_raster: %d, %d %d %d %d, %f %f %f %f",
	    mask,
	    s[0][0], s[0][1], s[1][0], s[1][1],
	    d[0][0], d[0][1], d[1][0], d[1][1]);

    masked = mask;

    /* TODO: are top and left swapped? */

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

    G_debug(1, " src (TBLR): %d %d %d %d, dst (TBLR) %f %f %f %f",
	    src_t, src_b, src_l, src_r, dst_t, dst_b, dst_l, dst_r);

    /* create source surface */
    src_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, src_w, src_h);
    status = cairo_surface_status(src_surf);
    if (status != CAIRO_STATUS_SUCCESS)
	G_fatal_error("Cairo_begin_raster(): %s (%s)",
                      _("Failed to create surface"),
                      cairo_status_to_string (status));

    src_data = cairo_image_surface_get_data(src_surf);
    src_stride = cairo_image_surface_get_stride(src_surf);
}

/*!
  \brief Draw raster row

  \param n number of cell
  \param row raster row
  \param red,grn,blu,nul red,green,blue and null value

  \return next row
*/
int Cairo_raster(int n, int row,
		 const unsigned char *red, const unsigned char *grn,
		 const unsigned char *blu, const unsigned char *nul)
{
    unsigned int *dst =
	(unsigned int *)(src_data + (row - src_t) * src_stride);
    int i;

    G_debug(3, "Cairo_raster: %d %d", n, row);

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
    G_debug(1, "Cairo_end_raster");

    /* paint source surface onto dstination (scaled) */
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
