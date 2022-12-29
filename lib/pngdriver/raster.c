/*!
  \file lib/pngdriver/raster.c

  \brief GRASS png display driver - draw raster

  (C) 2003-2014 by Per Henrik Johansen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Per Henrik Johansen (original contributor)
  \author Glynn Clements  
*/

#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include "driver.h"
#include "pngdriver.h"

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

static int *trans;
static int ncols;
static int nalloc;
static int masked;
static int src[2][2];
static int dst[2][2];

static double scale(double k, const int src[2], const int dst[2])
{
    return dst[0] + (double)(k - src[0]) * (dst[1] - dst[0]) / (src[1] -
								src[0]);
}

static int scale_fwd_y(int sy)
{
    return (int)floor(scale(sy, src[1], dst[1]) + 0.5);
}

static int scale_rev_x(int dx)
{
    return (int)floor(scale(dx + 0.5, dst[0], src[0]));
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

static void alloc_buffers(void)
{
    if (nalloc >= ncols)
	return;

    nalloc = ncols;
    trans = G_realloc(trans, nalloc * sizeof(int));
}

/*!
  \brief Start drawing raster

  \param mask non-zero int for mask
  \param s source (map) extent (left, right, top, bottom)
  \param fd destination (image) extent (left, right, top, bottom)
*/
void PNG_begin_raster(int mask, int s[2][2], double fd[2][2])
{
    int d[2][2];
    int i;

    d[0][0] = (int) floor(fd[0][0] + 0.5);
    d[0][1] = (int) floor(fd[0][1] + 0.5);
    d[1][0] = (int) floor(fd[1][0] + 0.5);
    d[1][1] = (int) floor(fd[1][1] + 0.5);

    ncols = d[0][1] - d[0][0];

    memcpy(src, s, sizeof(src));
    memcpy(dst, d, sizeof(dst));
    masked = mask;

    alloc_buffers();

    for (i = 0; i < ncols; i++)
	trans[i] = scale_rev_x(d[0][0] + i);
}

/*!
  \brief Draw raster row

  \param n number of cells
  \param row raster row (starts at 0)
  \param red,grn,blu,nul red,green,blue and null value

  \return next row
*/
int PNG_raster(int n, int row,
	       const unsigned char *red, const unsigned char *grn,
	       const unsigned char *blu, const unsigned char *nul)
{
    int d_y0 = scale_fwd_y(row + 0);
    int d_y1 = scale_fwd_y(row + 1);
    int d_rows = d_y1 - d_y0;
    int x0 = max(png.clip_left - dst[0][0], 0);
    int x1 = min(png.clip_rite - dst[0][0], ncols);
    int y0 = max(png.clip_top - d_y0, 0);
    int y1 = min(png.clip_bot - d_y0, d_rows);
    int x, y;

    if (y1 <= y0)
	return next_row(row, d_y1);

    for (x = x0; x < x1; x++) {
	int xx = dst[0][0] + x;
	int j = trans[x];
	int c;

	if (masked && nul && nul[j])
	    continue;

	c = png_get_color(red[j], grn[j], blu[j], 0);

	for (y = y0; y < y1; y++) {
	    int yy = d_y0 + y;

	    png.grid[yy * png.width + xx] = c;
	}
    }

    png.modified = 1;

    return next_row(row, d_y1);
}
