
/**
 * \file auto_mask.c
 *
 * \brief GIS Library - Auto masking routines.
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

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "G.h"


/**
 * \brief Checks for auto masking.
 *
 * On first call, opens the mask file if declared and available and 
 * allocates buffer for reading mask rows.
 * On second call, returns 0 or 1.
 *
 * \return 0 if mask unset or unavailable
 * \return 1 if mask set and available and ready to use
 */

int G__check_for_auto_masking(void)
{
    struct Cell_head cellhd;

    /* if mask is switched off (-2) return -2
       if G__.auto_mask is not set (-1) or set (>=0) recheck the MASK */

    if (G__.auto_mask < -1)
	return G__.auto_mask;

    /* if(G__.mask_fd > 0) G_free (G__.mask_buf); */

    /* look for the existence of the MASK file */
    G__.auto_mask = (G_find_cell("MASK", G_mapset()) != 0);

    if (G__.auto_mask <= 0)
	return 0;

    /* check MASK projection/zone against current region */
    if (G_get_cellhd("MASK", G_mapset(), &cellhd) >= 0) {
	if (cellhd.zone != G_zone() || cellhd.proj != G_projection()) {
	    G__.auto_mask = 0;
	    return 0;
	}
    }

    G_unopen_cell(G__.mask_fd);
    G__.mask_fd = G__open_cell_old("MASK", G_mapset());
    if (G__.mask_fd < 0) {
	G__.auto_mask = 0;
	G_warning(_("Unable to open automatic MASK file"));
	return 0;
    }

    /*    G__.mask_buf = G_allocate_cell_buf(); */

    G__.auto_mask = 1;

    return 1;
}


/**
 * \brief Suppresses masking.
 *
 * \return always returns 0
 */

int G_suppress_masking(void)
{
    if (G__.auto_mask > 0) {
	G_close_cell(G__.mask_fd);
	/* G_free (G__.mask_buf); */
	G__.mask_fd = -1;
    }
    G__.auto_mask = -2;

    return 0;
}


/**
 * \brief Unsuppresses masking.
 *
 * \return always returns 0
 */

int G_unsuppress_masking(void)
{
    if (G__.auto_mask < -1) {
	G__.mask_fd = -1;
	G__check_for_auto_masking();
    }

    return 0;
}
