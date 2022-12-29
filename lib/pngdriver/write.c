/*!
  \file lib/pngdriver/write.c

  \brief GRASS png display driver - write image (lower level functions)

  (C) 2007-2014 by Glynn Clements and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Glynn Clements
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <grass/config.h>
#include <grass/gis.h>
#include "pngdriver.h"

void write_image(void)
{
    char *p = png.file_name + strlen(png.file_name) - 4;

    if (!png.modified)
	return;

    if (png.mapped)
	return;

    if (G_strcasecmp(p, ".ppm") == 0) {
	write_ppm();
	if (png.has_alpha)
	    write_pgm();
    }
    else if (G_strcasecmp(p, ".bmp") == 0)
	write_bmp();
#ifdef HAVE_PNG_H
    else if (G_strcasecmp(p, ".png") == 0)
	write_png();
#endif
    else
	G_fatal_error("write_image: unknown file type: %s", p);

    png.modified = 0;
}
