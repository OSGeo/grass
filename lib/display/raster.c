/*!
  \file lib/display/raster.c

  \brief Display Driver - draw raster data

  (C) 2006-2011 by the GRASS Development Team

  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.

  \author Glynn Clements <glynn gclements.plus.com> (original contributor)
  \author Huidae Cho <grass4u gmail.com>
*/

#include <stdio.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include "driver.h"

extern int D__overlay_mode;

static int src[2][2];
static double dst[2][2];

static int draw_cell(int, const void *, struct Colors *, RASTER_MAP_TYPE);

/*!
  \brief Draw raster row
  
  - determine which pixel row gets the data
  - resamples the data to create a pixel array
  - determines best way to draw the array
  a - for single cat array, a move and a draw
  b - otherwise, a call to D_raster()

  Presumes the map is drawn from north to south.

  If overlay mode is desired, then call D_set_overlay_mode() first.

  \param A_row row number (starts at 0)
  \param array data buffer
  \param colors pointer to Colors structure
  \param data_type raster type (CELL, FCELL, DCELL)

  \return row number needed for next pixel row
  \return -1 nothing to draw (on error or end of raster)
*/
int D_draw_raster(int A_row,
		  const void *array,
		  struct Colors *colors, RASTER_MAP_TYPE data_type)
{
    return draw_cell(A_row, array, colors, data_type);
}

/*!
  \brief Draw raster row (DCELL)
  
  \param A_row row number (starts at 0)
  \param darray data buffer
  \param colors pointer to Colors structure

  \return 
*/
int D_draw_d_raster(int A_row, const DCELL * darray, struct Colors *colors)
{
    return draw_cell(A_row, darray, colors, DCELL_TYPE);
}

/*!
  \brief Draw raster row (FCELL)
  
  \param A_row row number (starts at 0)
  \param farray data buffer
  \param colors pointer to Colors structure

  \return row number needed for next pixel row
  \return -1 nothing to draw (on error or end of raster)
*/
int D_draw_f_raster(int A_row, const FCELL * farray, struct Colors *colors)
{
    return draw_cell(A_row, farray, colors, FCELL_TYPE);
}

/*!
  \brief Draw raster row (CELL)

  The <b>row</b> gives the map array row. The <b>carray</b> array
  provides the categories for each raster value in that row.  This
  routine is called consecutively with the information necessary to
  draw a raster image from north to south. No rows can be skipped. All
  screen pixel rows which represent the current map array row are
  rendered. The routine returns the map array row which is needed to
  draw the next screen pixel row.
  
  \param A_row row number (starts at 0)
  \param carray data buffer
  \param colors pointer to Colors structure

  \return row number needed for next pixel row
  \return -1 nothing to draw (on error or end of raster)
*/
int D_draw_c_raster(int A_row, const CELL * carray, struct Colors *colors)
{
    return draw_cell(A_row, carray, colors, CELL_TYPE);
}

static int draw_cell(int A_row,
		     const void *array,
		     struct Colors *colors, RASTER_MAP_TYPE data_type)
{
    static unsigned char *red, *grn, *blu, *set;
    static int nalloc;

    int ncols = src[0][1] - src[0][0];
    int i;

    if (nalloc < ncols) {
	nalloc = ncols;
	red = G_realloc(red, nalloc);
	grn = G_realloc(grn, nalloc);
	blu = G_realloc(blu, nalloc);
	set = G_realloc(set, nalloc);
    }

    Rast_lookup_colors(array, red, grn, blu, set, ncols, colors,
			   data_type);

    if (D__overlay_mode)
	for (i = 0; i < ncols; i++) {
	    set[i] = Rast_is_null_value(array, data_type);
	    array = G_incr_void_ptr(array, Rast_cell_size(data_type));
	}

    A_row =
	COM_raster(ncols, A_row, red, grn, blu, D__overlay_mode ? set : NULL);

    return (A_row < src[1][1])
	? A_row : -1;
}

/*!
  \brief Prepare for raster graphic
 
  The raster display subsystem establishes conversion parameters based
  on the screen extent defined by <b>top, bottom, left</b>, and
  <b>right</b>, all of which are obtainable from D_get_dst() for the
  current frame.
*/
void D_raster_draw_begin(void)
{
    /* Set up the screen for drawing map */
    D_get_a(src);
    D_get_d(dst);
    COM_begin_raster(D__overlay_mode, src, dst);
}

/*!
  \brief Draw raster row in RGB mode

  \param A_row row number (starts at 0)
  \param r_raster red data buffer
  \param g_raster green data buffer
  \param b_raster blue data buffer
  \param r_colors colors used for red channel
  \param g_colors colors used for green channel
  \param b_colors colors used for blue channel
  \param r_type raster type used for red channel
  \param g_type raster type used for red channel
  \param b_type raster type used for red channel

  \return row number needed for next pixel row
  \return -1 nothing to draw (on error or end of raster)
*/
int D_draw_raster_RGB(int A_row,
		      const void *r_raster, const void *g_raster,
		      const void *b_raster, struct Colors *r_colors,
		      struct Colors *g_colors, struct Colors *b_colors,
		      RASTER_MAP_TYPE r_type, RASTER_MAP_TYPE g_type,
		      RASTER_MAP_TYPE b_type)
{
    static unsigned char *r_buf, *g_buf, *b_buf, *n_buf;
    static int nalloc;

    int r_size = Rast_cell_size(r_type);
    int g_size = Rast_cell_size(g_type);
    int b_size = Rast_cell_size(b_type);
    int ncols = src[0][1] - src[0][0];
    int i;

    /* reallocate color_buf if necessary */
    if (nalloc < ncols) {
	nalloc = ncols;
	r_buf = G_realloc(r_buf, nalloc);
	g_buf = G_realloc(g_buf, nalloc);
	b_buf = G_realloc(b_buf, nalloc);
	n_buf = G_realloc(n_buf, nalloc);
    }

    /* convert cell values to bytes */
    Rast_lookup_colors(r_raster, r_buf, n_buf, n_buf, n_buf, ncols,
			   r_colors, r_type);
    Rast_lookup_colors(g_raster, n_buf, g_buf, n_buf, n_buf, ncols,
			   g_colors, g_type);
    Rast_lookup_colors(b_raster, n_buf, n_buf, b_buf, n_buf, ncols,
			   b_colors, b_type);

    if (D__overlay_mode)
	for (i = 0; i < ncols; i++) {
	    n_buf[i] = (Rast_is_null_value(r_raster, r_type) ||
			Rast_is_null_value(g_raster, g_type) ||
			Rast_is_null_value(b_raster, b_type));

	    r_raster = G_incr_void_ptr(r_raster, r_size);
	    g_raster = G_incr_void_ptr(g_raster, g_size);
	    b_raster = G_incr_void_ptr(b_raster, b_size);
	}

    A_row = COM_raster(ncols, A_row, r_buf, g_buf, b_buf,
		       D__overlay_mode ? n_buf : NULL);

    return (A_row < src[1][1])
	? A_row : -1;
}

/*!
  \brief Finish raster rendering
*/
void D_raster_draw_end(void)
{
    COM_end_raster();
}
