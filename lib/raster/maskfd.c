/*!
 * \file gis/maskfd.c
 *
 * \brief GIS Library - Mask functions.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <grass/gis.h>
#include <grass/Rast.h>

#include "G.h"

/*!
 * \brief Test for MASK.
 *
 * \return -1 if no MASK
 * \return file descriptor if MASK
 */

int Rast_maskfd(void)
{
    Rast__check_for_auto_masking();

    return G__.auto_mask > 0 ? G__.mask_fd : -1;
}
