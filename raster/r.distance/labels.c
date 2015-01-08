
/****************************************************************************
 *
 * MODULE:       r.distance
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *               Sort/reverse sort by distance by Huidae Cho
 *
 * PURPOSE:      Locates the closest points between objects in two 
 *               raster maps.
 *
 * COPYRIGHT:    (C) 2003-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 ***************************************************************************/

#include <stdlib.h>

#include <grass/raster.h>

#include "defs.h"

void read_labels(struct Map *map)
{
    if (Rast_read_cats(map->name, map->mapset, &map->labels) < 0)
	exit(1);
}

char *get_label(struct Map *map, CELL cat)
{
    return Rast_get_c_cat(&cat, &map->labels);
}
