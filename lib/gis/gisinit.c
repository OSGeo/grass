/*!
  \file lib/gis/gisinit.c
  
  \brief GIS Library - Handles program initialization.
  
  (C) 2001-2008, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author GRASS GIS Development Team
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <locale.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "G.h"
#include "gis_local_proto.h"

struct G__ G__;

static int initialized = 0; /** Is set when engine is initialized */
static int gisinit(void);

/*!
  \brief Initialize GIS Library and ensures a valid mapset is available.
  
  \param version
  \param pgm program (module) name
  
  \return always returns 0 on success
  \return G_fatal_error() is called on error
*/
void G__gisinit(const char *version, const char *pgm)
{
    const char *mapset;

    if (initialized)
	return;

    G_set_program_name(pgm);

    /* verify version of GRASS headers (and anything else in include) */
    if (strcmp(version, GIS_H_VERSION) != 0)
	G_fatal_error(_("Module built against version %s but "
			"trying to use version %s. "
			"You need to rebuild GRASS GIS or untangle multiple installations."),
                        version, GIS_H_VERSION);
    
    /* Make sure location and mapset are set */
    G_location_path();
    mapset = G_mapset();
    switch (G_mapset_permissions(mapset)) {
    case 1:
	break;
    case 0:
	G_fatal_error(_("MAPSET %s - permission denied"), mapset);
	break;
    default:
	G_fatal_error(_("MAPSET %s not found at %s"), mapset, G_location_path());
	break;
    }

    gisinit();
}


/*!
  \brief Initialize GIS Library
  
  Initializes GIS engine, but does not check for a valid mapset.
*/
void G__no_gisinit(const char *version)
{
    if (initialized)
	return;

    /* verify version of GRASS headers (and anything else in include) */
    if (strcmp(version, GIS_H_VERSION) != 0)
	G_fatal_error(_("Module built against version %s but "
			"trying to use version %s. "
			"You need to rebuild GRASS GIS or untangle multiple installations."),
                        version, GIS_H_VERSION);
    gisinit();
}


/*!
  \brief Checks to see if GIS engine is initialized.
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
    char *zlib;

#ifdef __MINGW32__
    _fmode = O_BINARY;
#endif
    /* Mark window as not set */
    G__.window_set = 0;

    /* byte order */
    G__.little_endian = G_is_little_endian();

    zlib = getenv("GRASS_ZLIB_LEVEL");
    /* Valid zlib compression levels -1 - 9 */
    /* zlib default: Z_DEFAULT_COMPRESSION = -1, equivalent to 6 
     * level 0 means no compression
     * as used here, 1 gives the best compromise between speed and compression */
    G__.compression_level = (zlib && *zlib && isdigit(*zlib)) ? atoi(zlib) : 1;
    if (G__.compression_level < -1 || G__.compression_level > 9)
	G__.compression_level = 1;

    initialized = 1;

    setlocale(LC_NUMERIC, "C");

    return 0;
}

/*!
  \brief Initialize environment
*/
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
    G__get_list_of_mapsets();
    G__home();
    G__machine_name();
    G_whoami();
    G_read_datum_table();
    G_read_ellipsoid_table(0);
}
