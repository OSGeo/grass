/*!
 * \file lib/raster/maskfd.c
 *
 * \brief Raster Library - Mask functions.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <grass/gis.h>
#include <grass/raster.h>

#include "R.h"

/*!
 * \brief Test for MASK.
 *
 * \return -1 if no MASK
 * \return file descriptor if MASK
 */

int Rast_maskfd(void)
{
    Rast__check_for_auto_masking();

    return R__.auto_mask > 0 ? R__.mask_fd : -1;
}
