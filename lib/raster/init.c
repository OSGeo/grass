
/**
 * \file lib/raster/init.c
 * 
 * \brief Raster Library - Handles program initialization.
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
#include <grass/raster.h>
#include <grass/glocale.h>

#include "R.h"

struct R__ R__;

static int initialized = 0; /** Is set when engine is initialized */
static int init(void);

/**
 * \brief Initialize GRASS GIS engine.
 *
 * Initializes GIS engine and ensures a valid mapset is available.
 *
 * \return always returns 0 on success
 * \return exit() is called on error
 */

void Rast_init(void)
{
    Rast__init();
}


/**
 * \brief Checks to see if GIS engine is initialized.
 *
 * \return
 */

void Rast__check_init(void)
{
    if (initialized)
	return;

    G_fatal_error(_("Raster library not initialized. Programmer forgot to call Rast_init()."));
}


void Rast__init(void)
{
    if (G_is_initialized(&initialized))
	return;
    init();
    G_initialize_done(&initialized);
}

void Rast__error_handler(void *p)
{
    Rast__unopen_all();
}

static int init(void)
{
    Rast__init_window();

    /* no histograms */
    R__.want_histogram = 0;

    /* set the write type for floating maps */
    R__.fp_type = getenv("GRASS_FP_DOUBLE") ? DCELL_TYPE : FCELL_TYPE;

    /* Set masking flag unknown */
    R__.auto_mask = -1;
    R__.mask_fd = -1;

    R__.nbytes = sizeof(CELL);
    R__.compression_type = getenv("GRASS_INT_ZLIB") ? 2 : 1;

    G_add_error_handler(Rast__error_handler, NULL);

    initialized = 1;

    return 0;
}

void Rast_init_all(void)
{
    Rast__init();
    Rast__check_for_auto_masking();
    Rast_init_gdal();
}
