/*!
  \file lib/cairodriver/raster.c

  \brief GRASS cairo display driver - draw raster

  (C) 2007-2014 by Lars Ahlzen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
  \author Glynn Clements  
*/

#include <math.h>

#include "cairodriver.h"
#include <grass/glocale.h>

#define MAX_IMAGE_SIZE 32767
#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

static int src_t, src_b, src_l, src_r, src_w, src_h;
static int dst_t, dst_b, dst_l, dst_r, dst_w, dst_h;

static int *trans;

static cairo_surface_t *src_surf;
static unsigned char *src_data;
static int src_stride, ca_row;

static int masked;

static double scale(double k, int src_0, int src_1, int dst_0, int dst_1)
{
    return dst_0 + (double) (k - src_0) * (dst_1 - dst_0) / (src_1 - src_0);
}

static int scale_fwd_y(int sy)
{
    return (int)floor(scale(sy, src_t, src_b, dst_t, dst_b) + 0.5);
}

static int scale_rev_x(int dx)
{
    return (int)floor(scale(dx + 0.5, dst_l, dst_r, src_l, src_r));
}

static int next_row(int sy, int dy)
{
    sy++;

    for (;;) {
	int y = scale_fwd_y(sy);

	if (y > dy)
	    return sy - 1;
	sy++;
    }
}

/*!
  \brief Start drawing raster

  \todo are top and left swapped?

  \param mask non-zero int for mask
  \param s source (map) extent (left, right, top, bottom)
  \param d destination (image) extent (left, right, top, bottom)
*/
void Cairo_begin_raster(int mask, int s[2][2], double d[2][2])
{
    int i;
    cairo_status_t status;
    
    masked = mask;

    src_l = s[0][0];
    src_r = s[0][1];
    src_t = s[1][0];
    src_b = s[1][1];

    src_w = src_r - src_l;
    src_h = src_b - src_t;

    dst_l = (int) floor(d[0][0] + 0.5);
    dst_r = (int) floor(d[0][1] + 0.5);
    dst_t = (int) floor(d[1][0] + 0.5);
    dst_b = (int) floor(d[1][1] + 0.5);

    dst_w = dst_r - dst_l;
    dst_h = dst_b - dst_t;

    G_debug(1, "Cairo_begin_raster(): masked=%d, src_lrtb=%d %d %d %d -> w/h=%d %d, "
            "dst_lrtb=%d %d %d %d -> w/h=%d %d",
            masked, src_l, src_r, src_t, src_b, src_w, src_h,
            dst_l, dst_r, dst_t, dst_b, dst_w, dst_h);

    /* create source surface */
    src_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ca.width, ca.height);
    status = cairo_surface_status(src_surf);
    if (status != CAIRO_STATUS_SUCCESS)
        G_fatal_error("%s - %s - size: %dx%d (cairo limit: %dx%d)",
                      _("Failed to create cairo surface"),
                      cairo_status_to_string (status), ca.width, ca.height,
                      MAX_IMAGE_SIZE, MAX_IMAGE_SIZE);

    src_data = cairo_image_surface_get_data(src_surf);
    src_stride = cairo_image_surface_get_stride(src_surf);
    ca_row = 0;

    /* allocate buffer for down-sampling data */
    trans = G_malloc(dst_w * sizeof(int));
    for (i = 0; i < dst_w; i++)
        trans[i] = scale_rev_x(dst_l + i);
}

/*!
  \brief Draw raster row

  \param n number of cells
  \param row raster row (starting at 0)
  \param red,grn,blu,nul red,green,blue and null value

  \return next row
*/
int Cairo_raster(int n, int row,
		 const unsigned char *red, const unsigned char *grn,
		 const unsigned char *blu, const unsigned char *nul)
{
    int d_y0 = scale_fwd_y(row + 0);
    int d_y1 = scale_fwd_y(row + 1);
    int d_rows = d_y1 - d_y0;
    int x0 = MAX(0        - dst_l, 0);
    int x1 = MIN(ca.width - dst_l, dst_w);
    int y0 = MAX(0         - d_y0, 0);
    int y1 = MIN(ca.height - d_y0, d_rows);
    int x, y;

    if (y1 <= y0)
        return next_row(row, d_y1);

    G_debug(3, "Cairo_raster(): n=%d row=%d", n, row);

    for (x = x0; x < x1; x++) {
	int xx = dst_l + x;
        int j = trans[x];
	unsigned int c;

	if (masked && nul && nul[j])
	    c = 0;
	else {
	    unsigned int r = red[j];
	    unsigned int g = grn[j];
	    unsigned int b = blu[j];
	    unsigned int a = 0xFF;
	    c = (a << 24) + (r << 16) + (g << 8) + (b << 0);
	}

	for (y = y0; y < y1; y++) {
	    int yy = d_y0 + y;
	    *(unsigned int *)(src_data + yy * src_stride + xx * 4) = c;
        }
    }

    ca.modified = 1;
    ca_row++;

    return next_row(row, d_y1);
}

/*!
  \brief Finish drawing raster
*/
void Cairo_end_raster(void)
{
    G_debug(1, "Cairo_end_raster()");

    /* paint source surface onto destination (scaled) */
    cairo_save(cairo);
    /* cairo_translate(cairo, dst_l, dst_t); */
    /* cairo_scale(cairo, dst_w / src_w, dst_h / src_h); */
    cairo_surface_mark_dirty(src_surf);
    cairo_set_source_surface(cairo, src_surf, 0, 0);
    cairo_pattern_set_filter(cairo_get_source(cairo), CAIRO_FILTER_NEAREST);
    cairo_paint(cairo);
    cairo_restore(cairo);

    /* cleanup */
    G_free(trans);
    cairo_surface_destroy(src_surf);
    ca.modified = 1;
}
