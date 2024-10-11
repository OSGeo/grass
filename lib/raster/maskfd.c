/*!
 * \file lib/raster/maskfd.c
 *
 * \brief Raster Library - Mask file descriptor and state.
 *
 * (C) 2001-2024 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 * \author Vaclav Petras (documentation)
 */

#include <grass/gis.h>
#include <grass/raster.h>

#include "R.h"

/*!
 * \brief Test for raster mask presence and get file descriptor if present.
 *
 * This function tests the mask presence and takes into account the state of
 * auto-masking in the library, so mask is considered as not present when
 * masking is suppressed regardless of the presence of the mask raster.
 *
 * \return -1 if mask is not present
 * \return file descriptor if raster mask is present and active
 */
int Rast_maskfd(void)
{
    Rast__check_for_auto_masking();

    return R__.auto_mask > 0 ? R__.mask_fd : -1;
}
