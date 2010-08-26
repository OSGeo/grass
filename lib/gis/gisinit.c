
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <locale.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "G.h"

struct G__ G__;

static int initialized = 0; /** Is set when engine is initialized */
static int gisinit(void);


/**
 * \brief Initialize GRASS GIS engine.
 *
 * Initializes GIS engine and ensures a valid mapset is available.
 *
 * \param[in] pgm Program (module) name
 * \return always returns 0 on success
 * \return exit() is called on error
 */

void G__gisinit(const char *version, const char *pgm)
{
    const char *mapset;

    if (initialized)
	return;

    G_set_program_name(pgm);

    if (strcmp(version, GIS_H_VERSION) != 0)
	G_fatal_error(_("Incompatible library version for module. "
			"You need to rebuild GRASS or untangle multiple installations."));

    /* Make sure location and mapset are set */
    G_location_path();
    mapset = G_mapset();
    switch (G__mapset_permissions(mapset)) {
    case 1:
	break;
    case 0:
	G_fatal_error(_("MAPSET %s - permission denied"), mapset);
	break;
    default:
	G_fatal_error(_("MAPSET %s not found"), mapset);
	break;
    }

    gisinit();
}


/**
 * \brief Initialize GRASS GIS engine.
 *
 * Initializes GIS engine, but does not check for a valid mapset.
 *
 * \return
 */

void G__no_gisinit(const char *version)
{
    if (initialized)
	return;

    if (strcmp(version, GIS_H_VERSION) != 0)
	G_fatal_error(_("Incompatible library version for module. "
			"You need to rebuild GRASS or untangle multiple installations."));

    gisinit();
}


/**
 * \brief Checks to see if GIS engine is initialized.
 *
 * \return
 */

void G__check_gisinit(void)
{
    if (initialized)
	return;
    G_warning(_("System not initialized. Programmer forgot to call G_gisinit()."));
    G_sleep(3);
    exit(EXIT_FAILURE);
}


static int gisinit(void)
{
#ifdef __MINGW32__
    _fmode = O_BINARY;
#endif
    /* Mark window as not set */
    G__.window_set = 0;

    initialized = 1;

    setlocale(LC_NUMERIC, "C");

    return 0;
}

void G_init_all(void)
{
    G__check_gisinit();
    G_init_env();
    G_init_logging();
    G__init_window();
    G_init_locale();
    G_init_debug();
    G_verbose();
    G_init_tempfile();
    G_get_list_of_mapsets();
    G__home();
    G__machine_name();
    G_whoami();
    G_read_datum_table();
    G_read_ellipsoid_table(0);
}
