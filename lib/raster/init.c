/**
 * \file gisinit.c
 * 
 * \brief GIS Library - Handles program initialization.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 2000-2008
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <locale.h>

#include <grass/gis.h>
#include <grass/Rast.h>
#include <grass/glocale.h>

#include "../raster/G.h"

struct G__ G__;

static int initialized = 0; /** Is set when engine is initialized */
static int init(void);

/**
 * \brief Initialize GRASS GIS engine.
 *
 * Initializes GIS engine and ensures a valid mapset is available.
 *
 * \param[in] pgm Program (module) name
 * \return always returns 0 on success
 * \return exit() is called on error
 */

void Rast__gisinit()
{
    if (initialized)
	return;

    init();
}


/**
 * \brief Checks to see if GIS engine is initialized.
 *
 * \return
 */

void Rast__check_gisinit(void)
{
    if (initialized)
	return;

    G_fatal_error(_("Raster library not initialized. Programmer forgot to call Rast_init()."));
}


static int init(void)
{
    /* Mark window as not set */
    G__.window_set = 0;

    /* no histograms */
    G__.want_histogram = 0;

    /* set the write type for floating maps */
    G__.fp_type = getenv("GRASS_FP_DOUBLE") ? DCELL_TYPE : FCELL_TYPE;

    /* Set masking flag unknown */
    G__.auto_mask = -1;

    G__.nbytes = sizeof(CELL);
    G__.compression_type = getenv("GRASS_INT_ZLIB") ? 2 : 1;

    initialized = 1;

    return 0;
}

void Rast_init_all(void)
{
    Rast__check_for_auto_masking();
    Rast_init_gdal();
}
