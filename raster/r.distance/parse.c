
/****************************************************************************
 *
 * MODULE:       r.distance
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Locates the closest points between objects in two
 *               raster maps.
 *
 * COPYRIGHT:    (C) 2003 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "defs.h"
#include <grass/gis.h>
#include <grass/glocale.h>

void parse(int argc, char *argv[], struct Parms *parms)
{
    struct Option *maps, *fs;
    struct Flag *labels, *overlap;
    const char *name, *mapset;

    maps = G_define_standard_option(G_OPT_R_MAPS);
    maps->key_desc = "name1,name2";
    maps->description = _("Name of two input raster maps for computing inter-class distances");

    fs = G_define_standard_option(G_OPT_F_SEP);
    fs->answer = ":";		/* colon is default output fs */

    labels = G_define_flag();
    labels->key = 'l';
    labels->description = _("Include category labels in the output");

    overlap = G_define_flag();
    overlap->key = 'o';
    overlap->description =
	_("Report zero distance if rasters are overlapping");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    name = parms->map1.name = maps->answers[0];
    mapset = parms->map1.mapset = G_find_raster2(name, "");
    if (mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), name);

    parms->map1.fullname = G_fully_qualified_name(name, mapset);

    name = parms->map2.name = maps->answers[1];
    mapset = parms->map2.mapset = G_find_raster2(name, "");
    if (mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), name);

    parms->map2.fullname = G_fully_qualified_name(name, mapset);

    parms->labels = labels->answer ? 1 : 0;
    parms->fs = fs->answer;
    parms->overlap = overlap->answer ? 1 : 0;
}
