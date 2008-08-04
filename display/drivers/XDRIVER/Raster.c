
/******************************************************************************
 * These routines support the drawing of multi-band images on the graphics
 * device.
 *
 * All intensity values are represented in unsigned (8-byte) values.  That is
 * with values between and including 0 and 255.
 *
 ******************************************************************************
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "includes.h"
#include <grass/gis.h>
#include "XDRIVER.h"

static int *trans;
static int ncols;
static int nalloc;
static int masked;
static int src[2][2];
static int dst[2][2];
static XImage *img;

extern unsigned long find_color(unsigned int r, unsigned int g,
				unsigned int b);

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

static void raster_row(int x0, int y0, int ncols, int nrows,
		       const unsigned char *nul)
{
    int i, j;

    if (!masked || !nul) {
	for (j = 0; j < nrows; j++)
	    XPutImage(dpy, bkupmap, gc, img, 0, 0, x0, y0 + j, ncols, 1);
	return;
    }

    for (i = 0; i < ncols;) {
	int i0;

	for (; i < ncols && nul[trans[i]]; i++) ;

	i0 = i;

	for (; i < ncols && !nul[trans[i]]; i++) ;

	if (i == i0)
	    continue;

	for (j = 0; j < nrows; j++)
	    XPutImage(dpy, bkupmap, gc, img, i0, 0, x0 + i0, y0 + j, i - i0,
		      1);
    }
}

static void alloc_ximage(void)
{
    XWindowAttributes attr;

    if (img)
	XDestroyImage(img);

    XGetWindowAttributes(dpy, grwin, &attr);

    img = XCreateImage(dpy, attr.visual, attr.depth, ZPixmap,
		       0, NULL, ncols, 1, 8, 0);

    img->data = G_malloc(img->bytes_per_line + 4);
    *(int *)(img->data + img->bytes_per_line) = 0xDEADBEEF;

    if (!img)
	G_fatal_error("unable to allocate XImage");
}

void XD_begin_scaled_raster(int mask, int s[2][2], int d[2][2])
{
    int i;

    ncols = d[0][1] - d[0][0];

    memcpy(src, s, sizeof(src));
    memcpy(dst, d, sizeof(dst));
    masked = mask;

    alloc_buffers();
    alloc_ximage();

    for (i = 0; i < ncols; i++)
	trans[i] = scale_rev_x(d[0][0] + i);
}

int XD_scaled_raster(int n, int row,
		     const unsigned char *red, const unsigned char *grn,
		     const unsigned char *blu, const unsigned char *nul)
{
    int d_y0 = scale_fwd_y(row + 0);
    int d_y1 = scale_fwd_y(row + 1);
    int d_rows = d_y1 - d_y0;
    int x;

    if (d_rows <= 0)
	return next_row(row, d_y0);

    for (x = 0; x < ncols; x++) {
	int j = trans[x];
	int c;

	if (masked && nul && nul[j])
	    continue;

	c = find_color(red[j], grn[j], blu[j]);

	XPutPixel(img, x, 0, c);
    }

    raster_row(dst[0][0], d_y0, ncols, d_rows, nul);

    needs_flush = 1;

    return next_row(row, d_y1);
}
