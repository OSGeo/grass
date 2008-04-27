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

void 
parse (int argc, char *argv[], struct Parms *parms)
{
    struct Option *maps, *fs;
    struct Flag *labels, *overlap;
    char *name, *mapset;

    /* please, remove before GRASS 7 released */
    struct Flag *q_flag;

    maps = G_define_option();

    maps->key = "maps";
    maps->key_desc = "map1,map2";
    maps->required = YES;
    maps->multiple = NO;
    maps->type = TYPE_STRING;
    maps->description = _("Maps for computing inter-class distances");
    maps->gisprompt = "old,cell,raster";

    fs  = G_define_option();
    fs->key = "fs";
    fs->required = NO;
    fs->multiple = NO;
    fs->answer   = ":";       /* colon is default output fs */
    fs->type = TYPE_STRING;
    fs->description = _("Output field separator");

    labels = G_define_flag();
    labels -> key = 'l';
    labels->description = _("Include category labels in the output");

    overlap = G_define_flag();
    overlap -> key = 'o';
    overlap->description = _("Report zero distance if rasters are overlapping");

    /* please, remove before GRASS 7 released */
    q_flag = G_define_flag() ;
    q_flag->key         = 'q' ;  
    q_flag->description = _("Run quietly") ;

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* please, remove before GRASS 7 released */
    if(q_flag->answer) {
        G_putenv("GRASS_VERBOSE","0");
        G_warning(_("The '-q' flag is superseded and will be removed "
            "in future. Please use '--quiet' instead."));
    }

    name = parms->map1.name = maps->answers[0];
    mapset = parms->map1.mapset = G_find_cell (name, "");
    if (mapset == NULL)
        G_fatal_error ( _("Raster map <%s> not found"), name);

    parms->map1.fullname = G_fully_qualified_name (name,mapset);

    name = parms->map2.name = maps->answers[1];
    mapset = parms->map2.mapset = G_find_cell (name, "");
    if (mapset == NULL)
        G_fatal_error ( _("Raster map <%s> not found"), name);

    parms->map2.fullname = G_fully_qualified_name (name,mapset);

    parms->labels = labels->answer ? 1 : 0;
    parms->fs = fs->answer;
    parms->overlap = overlap->answer ? 1:0;
}
