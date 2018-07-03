/*!
   \file lib/ogsf/gsd_img_ppm.c

   \brief OGSF library - PPM stuff

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008 by the GRASS Development Team

   - added little/big endian test Markus Neteler
   - modified to PPM by Bob Covill <bcovill@tekmap.ns.ca>
   - changed 10/99 Jaro
   - Created new function GS_write_ppm based on RGB dump 

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown USACERL, GMSL/University of Illinois
   \author Markus Neteler
   \author Bob Covill
   \author Jaro Hofierka
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/ogsf.h>

/*!
   \brief Save current GL screen to ppm file

   \param name file name

   \return 1 on failure
   \return 0 on success
 */
int GS_write_ppm(const char *name)
{
    unsigned int x;
    int y;
    unsigned int xsize, ysize;
    FILE *fp;
    unsigned char *pixbuf;

    if (0 == gsd_getimage(&pixbuf, &xsize, &ysize)) {
	G_warning(_("Unable to get image of current GL screen"));
	return (1);
    }

    if (NULL == (fp = fopen(name, "w"))) {
	G_warning(_("Unable to open file <%s> for writing"), name);
	return (1);
    }

    fprintf(fp, "P6\n%d %d\n255\n", xsize, ysize);

    for (y = ysize - 1; y >= 0; y--) {
	for (x = 0; x < xsize; x++) {
	    unsigned char r = pixbuf[(y * xsize + x) * 4 + 0];
	    unsigned char g = pixbuf[(y * xsize + x) * 4 + 1];
	    unsigned char b = pixbuf[(y * xsize + x) * 4 + 2];

	    fputc((int)r, fp);
	    fputc((int)g, fp);
	    fputc((int)b, fp);
	}

    }
    G_free(pixbuf);
    fclose(fp);

    return (0);
}

/*!
   \brief Write zoom to file

   \param name file name
   \param xsize,ysize

   \return 1 on failure
   \return 0 on success
 */
int GS_write_zoom(const char *name, unsigned int xsize, unsigned int ysize)
{
    unsigned int x;
    int y;
    FILE *fp;
    unsigned char *pixbuf;

    if (0 == gsd_writeView(&pixbuf, xsize, ysize)) {
	G_warning(_("Unable to write view"));
	return (1);
    }

    if (NULL == (fp = fopen(name, "w"))) {
	G_warning(_("Unable to open file <%s> for writing"), name);
	return (1);
    }

    fprintf(fp, "P6\n%d %d\n255\n", xsize, ysize);

    for (y = ysize - 1; y >= 0; y--) {
	for (x = 0; x < xsize; x++) {
	    unsigned char r = pixbuf[(y * xsize + x) * 4 + 0];
	    unsigned char g = pixbuf[(y * xsize + x) * 4 + 1];
	    unsigned char b = pixbuf[(y * xsize + x) * 4 + 2];

	    fputc((int)r, fp);
	    fputc((int)g, fp);
	    fputc((int)b, fp);
	}

    }
    free(pixbuf);
    fclose(fp);

    return (0);
}
