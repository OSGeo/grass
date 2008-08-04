/*
 ****************************************************************************
 *
 * MODULE:       GRASS 5 gis library, gishelp.c
 * AUTHOR(S):    unknown
 * PURPOSE:      Print help information
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/*
 **********************************************************************
 *
 *  G_gishelp(helpfile, request)
 *      char *helpfile           help directory where "request" is found
 *      char *request            help file desired
 *
 *   Prints a helpfile to the screen.  Helpfiles are stored in directories
 *   associated with different GRID programs.  A given file will be
 *   found in   $GISBASE/txt/"helpfile"/"request".
 *
 **********************************************************************/

#include <grass/gis.h>
#include <grass/glocale.h>
#include <unistd.h>
#include <stdlib.h>
#include <grass/spawn.h>
#define GEN_HELP	"gen_help"

int G_gishelp(const char *helpfile, const char *request)
{
    char file[1024];

    if (request == NULL)
	request = GEN_HELP;

    sprintf(file, "%s/txt/%s/%s", G_getenv("GISBASE"), helpfile, request);

    if (!access(file, 04)) {
	fprintf(stderr, _("one moment...\n"));
	G_spawn(getenv("GRASS_PAGER"), getenv("GRASS_PAGER"), file, NULL);
    }
    else {
	fprintf(stderr, _("No help available for command [%s]\n"), request);
    }

    return 0;
}
