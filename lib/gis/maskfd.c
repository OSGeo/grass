
/**
 * \file maskfd.c
 *
 * \brief GIS Library - Mask functions.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2008
 */

#include <grass/gis.h>
#include "G.h"


/**
 * \brief Test for MASK.
 *
 * \return -1 if no MASK
 * \return file descriptor if MASK
 */

int G_maskfd(void)
{
    G__check_for_auto_masking();

    return G__.auto_mask > 0 ? G__.mask_fd : -1;
}
