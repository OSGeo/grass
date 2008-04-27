/**
 * \file gisinit.c
 * 
 * \brief Handles program initialization.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 2000-2006
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <grass/gis.h>
#include "G.h"
#include <grass/glocale.h>

struct G__ G__ ;
static int initialized = 0; /** Is set when engine is initialized */
static int gisinit(void);


/**
 * \fn int G_gisinit (const char *pgm)
 *
 * \brief Initialize GRASS GIS engine.
 *
 * Initializes GIS engine and ensures a valid mapset is available.
 *
 * \param[in] pgm Program (module) name
 * \return always returns 0 on success
 * \return exit() is called on error
 */

int G_gisinit(const char *pgm)
{
    char *mapset;

    if ( initialized )
	return 0;

    G_set_program_name (pgm);

   /* Make sure location and mapset are set */
    G_location_path();
    switch (G__mapset_permissions (mapset = G_mapset()))
    {
    case 1:
	    break;
    case 0:
	    G_fatal_error (_("MAPSET %s - permission denied"), mapset);
	    break;
    default:
	    G_fatal_error (_("MAPSET %s not found"), mapset);
	    break;
    }

    gisinit();

    return 0;
}


/**
 * \fn int G_no_gisinit (void)
 *
 * \brief Initialize GRASS GIS engine.
 *
 * Initializes GIS engine, but does not check for a valid mapset.
 *
 * \return always returns 0 on success
 */

int G_no_gisinit(void)
{
    if ( initialized )
	return 0;

    gisinit();

    return 0;
}


/**
 * \fn int G__check_gisinit (void)
 *
 * \brief Checks to see if GIS engine is initialized.
 *
 * \return 1 on success
 * \return exit() is called on error
 */

int G__check_gisinit(void)
{
    if (initialized) return 1;
    G_warning (_("System not initialized. Programmer forgot to call G_gisinit()."));
    G_sleep(3);
    exit(EXIT_FAILURE);
}


static int gisinit(void)
{
    /* Mark window as not set */
    G__.window_set = 0 ;

    /* no histograms */
    G__.want_histogram = 0;

    /* Set compressed data buffer size to zero */
    G__.compressed_buf_size = 0;
    G__.work_buf_size = 0;
    G__.null_buf_size = 0;
    G__.mask_buf_size = 0;
    G__.temp_buf_size = 0;
    /* mask buf we always want to keep allocated */
    G__reallocate_mask_buf();

    /* set the write type for floating maps */
    G__.fp_type = FCELL_TYPE;
    G__.fp_nbytes = XDR_FLOAT_NBYTES;

    /* Set masking flag unknown */
    G__.auto_mask = -1 ;

    /* set architecture dependant bit patterns for embeded null vals */
    G__init_null_patterns();

    initialized = 1;

    return 0;
}
