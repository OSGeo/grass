
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
    char *zlib, *nulls, *cname;

    Rast__init_window();

    /* no histograms */
    R__.want_histogram = 0;

    /* set the write type for floating maps */
    R__.fp_type = getenv("GRASS_FP_DOUBLE") ? DCELL_TYPE : FCELL_TYPE;

    /* Set masking flag unknown */
    R__.auto_mask = -1;
    R__.mask_fd = -1;

    R__.nbytes = sizeof(CELL);

    R__.fileinfo_count = 0;
    R__.fileinfo = NULL;

    R__.compression_type = G_default_compressor();

    cname = getenv("GRASS_COMPRESSOR");
    /* 1: RLE
     * 2: ZLIB (DEFLATE)
     * 3: LZ4
     * 4: BZIP2
     * 5: ZSTD */
    if (cname && *cname) {
	/* ask gislib */
	R__.compression_type = G_compressor_number(cname);
	if (R__.compression_type < 1) {
	    if (R__.compression_type < 0) {
		G_warning(_("Unknown compression method <%s>, using default %s"),
		    cname, G_compressor_name(G_default_compressor()));
	    }
	    if (R__.compression_type == 0) {
		G_warning(_("No compression is not supported for GRASS raster maps, using default %s"),
		          G_compressor_name(G_default_compressor()));
	    }
	    /* use default */
	    R__.compression_type = G_default_compressor();
	}
	if (G_check_compressor(R__.compression_type) != 1) {
	    G_warning(_("This GRASS version does not support %s compression, using default %s"),
		cname, G_compressor_name(G_default_compressor()));
	    /* use default */
	    R__.compression_type = G_default_compressor();
	}
	G_debug(1, "Using %s compression",
	           G_compressor_name(R__.compression_type));
    }

    nulls = getenv("GRASS_COMPRESS_NULLS");
    R__.compress_nulls = (nulls && atoi(nulls) == 0) ? 0 : 1;

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
