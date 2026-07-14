/*!
   \file write_img.c

   \brief Save current GL screen to image file.

   SPDX-FileCopyrightText: 2008, 2010 by the GRASS Development Team

   SPDX-License-Identifier: GPL-2.0-or-later.

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
