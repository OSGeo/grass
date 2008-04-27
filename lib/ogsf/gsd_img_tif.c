/*
 * $Id$
 *
 * endian added Markus
 * changed 10/99 Jaro
 * Created new function GS_write_tif based
 * on RGB dump 
 */
#include <grass/config.h>

#ifdef HAVE_TIFFIO_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <grass/gis.h>
#include "tiffio.h"
#include <grass/gstypes.h>

unsigned short config = PLANARCONFIG_CONTIG;
unsigned short compression = -1;
unsigned short rowsperstrip = 0;


int GS_write_tif(char *name)
{
    TIFF *out;
    int y, x;
    unsigned int xsize, ysize;
    int mapsize, linebytes;
    unsigned char *buf, *tmpptr;
    unsigned char *pixbuf;

    gsd_getimage(&pixbuf, &xsize, &ysize);

    out = TIFFOpen(name, "w");
    if (out == NULL) {
	G_warning("Cannot open file for output.");
	return(1);
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

    if (rowsperstrip != (unsigned short) - 1) {
	rowsperstrip = (unsigned short) (8 * 1024 / linebytes);
    }

    TIFFSetField(out, TIFFTAG_ROWSPERSTRIP,
		 rowsperstrip == 0 ? 1 : rowsperstrip);

    /* Done with Header Info */
    for (y = 0; y < ysize; y++) {
	int yy = ysize - y - 1;
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
    free(pixbuf);
    (void) TIFFClose(out);

    return (0);
}

#endif /* HAVE_TIFF */

