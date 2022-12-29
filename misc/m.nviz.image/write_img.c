/*!
   \file write_img.c

   \brief Save current GL screen to image file.

   (C) 2008, 2010 by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   Based on visualization/nviz/src/anim_support.c

   \author Updated/modified by Martin Landa <landa.martin gmail.com>
 */

#include "local_proto.h"

#include <grass/ogsf.h>

/*!
   \brief Save current GL screen to an ppm file.

   \param name filename

   \return 0 on success
   \return 1 on failure (failed to write image)
   \return 2 on failure (unsupported format)
 */
int write_img(const char *name, int format)
{
    if (format == FORMAT_PPM)
	return GS_write_ppm(name);
#ifdef HAVE_TIFFIO_H
    else if (format == FORMAT_TIF)
	return GS_write_tif(name);
#endif
    else
	return 2;

    return 0;
}
