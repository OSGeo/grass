
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
#include "defs.h"

void read_labels(struct Map *map)
{
    if (G_read_cats(map->name, map->mapset, &map->labels) < 0)
	exit(1);
}

char *get_label(struct Map *map, CELL cat)
{
    char *G_get_cat();

    return G_get_cat(cat, &map->labels);
}
