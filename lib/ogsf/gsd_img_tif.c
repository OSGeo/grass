/*!
   \file lib/ogsf/gsd_img_tif.c

   \brief OGSF library - TIFF stuff

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008 by the GRASS Development Team

   - added little/big endian test Markus Neteler
   - modified to PPM by Bob Covill <bcovill@tekmap.ns.ca>
   - changed 10/99 Jaro
   - Created new function GS_write_tif based on RGB dump 

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown USACERL, GMSL/University of Illinois
   \author Markus Neteler
   \author Jaro Hofierka
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <grass/config.h>

#ifdef HAVE_TIFFIO_H

#include <stdlib.h>
#include <sys/types.h>
#ifdef __CYGWIN__
 #include <w32api/wtypes.h>
#endif
#include <grass/gis.h>
#include <grass/ogsf.h>
#include <grass/glocale.h>

#include <tiffio.h>

unsigned short config = PLANARCONFIG_CONTIG;
unsigned short compression = -1;
unsigned short rowsperstrip = 0;

/*!
   \brief Write data to tif file

   \param name filename

   \return 1 on error
   \return 0 on success
 */
int GS_write_tif(const char *name)
{
    TIFF *out;
    unsigned int y, x;
    unsigned int xsize, ysize;
    int mapsize, linebytes;
    unsigned char *buf, *tmpptr;
    unsigned char *pixbuf;

    if (0 == gsd_getimage(&pixbuf, &xsize, &ysize)) {
	G_warning(_("Unable to get image of current GL screen"));
	return (1);
    }

    out = TIFFOpen(name, "w");
    if (out == NULL) {
	G_warning(_("Unable to open file <%s> for writing"), name);
	return (1);
    }

    /* Write out TIFF Tags */
    /* Assuming 24 bit RGB Tif */
    TIFFSetField(out, TIFFTAG_IMAGEWIDTH, xsize);
    TIFFSetField(out, TIFFTAG_IMAGELENGTH, ysize);
    TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 24 > 8 ? 3 : 1);
    TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 24 > 1 ? 8 : 1);
    TIFFSetField(out, TIFFTAG_PLANARCONFIG, config);
    mapsize = 1 << 24;

    TIFFSetField(out, TIFFTAG_PHOTOMETRIC, 24 > 8 ?
		 PHOTOMETRIC_RGB : PHOTOMETRIC_MINISBLACK);

    linebytes = ((xsize * ysize + 15) >> 3) & ~1;

    if (TIFFScanlineSize(out) > linebytes) {
	buf = (unsigned char *)G_malloc(linebytes);
    }
    else {
	buf = (unsigned char *)G_malloc(TIFFScanlineSize(out));
    }

    if (rowsperstrip != (unsigned short)-1) {
	rowsperstrip = (unsigned short)(8 * 1024 / linebytes);
    }

    TIFFSetField(out, TIFFTAG_ROWSPERSTRIP,
		 rowsperstrip == 0 ? 1 : rowsperstrip);

    /* Done with Header Info */
    for (y = 0; y < ysize; y++) {
	unsigned int yy = ysize - y - 1;

	tmpptr = buf;

	for (x = 0; x < (xsize); x++) {
	    *tmpptr++ = pixbuf[(yy * xsize + x) * 4 + 0];
	    *tmpptr++ = pixbuf[(yy * xsize + x) * 4 + 1];
	    *tmpptr++ = pixbuf[(yy * xsize + x) * 4 + 2];
	}

	if (TIFFWriteScanline(out, buf, y, 0) < 0) {
	    break;
	}
    }

    G_free((void *)pixbuf);
    (void)TIFFClose(out);

    return (0);
}

#endif /* HAVE_TIFF */
