
/**
 * \file auto_mask.c
 *
 * \brief Raster Library - Auto masking routines.
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
#include <grass/raster.h>
#include <grass/glocale.h>

#include "R.h"


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

int Rast__check_for_auto_masking(void)
{
    struct Cell_head cellhd;

    Rast__init();

    /* if mask is switched off (-2) return -2
       if R__.auto_mask is not set (-1) or set (>=0) recheck the MASK */

    if (R__.auto_mask < -1)
	return R__.auto_mask;

    /* if(R__.mask_fd > 0) G_free (R__.mask_buf); */

    /* look for the existence of the MASK file */
    R__.auto_mask = (G_find_raster("MASK", G_mapset()) != 0);

    if (R__.auto_mask <= 0)
	return 0;

    /* check MASK projection/zone against current region */
    Rast_get_cellhd("MASK", G_mapset(), &cellhd);
    if (cellhd.zone != G_zone() || cellhd.proj != G_projection()) {
	R__.auto_mask = 0;
	return 0;
    }

    if (R__.mask_fd >= 0)
	Rast_unopen(R__.mask_fd);
    R__.mask_fd = Rast__open_old("MASK", G_mapset());
    if (R__.mask_fd < 0) {
	R__.auto_mask = 0;
	G_warning(_("Unable to open automatic MASK file"));
	return 0;
    }

    /*    R__.mask_buf = Rast_allocate_c_buf(); */

    R__.auto_mask = 1;

    return 1;
}


/**
 * \brief Suppresses masking.
 *
 * \return
 */

void Rast_suppress_masking(void)
{
    Rast__init();

    if (R__.auto_mask > 0) {
	Rast_close(R__.mask_fd);
	/* G_free (R__.mask_buf); */
	R__.mask_fd = -1;
    }
    R__.auto_mask = -2;
}


/**
 * \brief Unsuppresses masking.
 *
 * \return
 */

void Rast_unsuppress_masking(void)
{
    Rast__init();

    if (R__.auto_mask < -1) {
	R__.mask_fd = -1;
	Rast__check_for_auto_masking();
    }
}
