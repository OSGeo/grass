
/****************************************************************************
 *
 * MODULE:       gis library
 * AUTHOR(S):    Glynn Clements <glynn@gclements.plus.com>
 * COPYRIGHT:    (C) 2007 Glynn Clements
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <grass/gis.h>

int G_remove_colors(const char *name, const char *mapset)
{
    char element[GMAPSET_MAX + 6];
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    int stat;

    if (G__name_is_fully_qualified(name, xname, xmapset)) {
	if (strcmp(xmapset, mapset) != 0)
	    return -1;
	name = xname;
    }

    /* get rid of existing colr2, if any */
    sprintf(element, "colr2/%s", mapset);
    stat = G_remove(element, name);

    if (strcmp(mapset, G_mapset()) == 0)
	stat = G_remove("colr", name);

    return stat;
}
