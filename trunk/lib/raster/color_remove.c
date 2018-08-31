/*!
  \file lib/raster/color_remove.c
 
  \brief Raster Library - remove color table of raster map
  
  (C) 2007 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Glynn Clements
*/

#include <string.h>
#include <stdio.h>
#include <grass/gis.h>

/*!
  \brief Remove color table of raster map

  \param name name of raster map
  \param mapset name of mapset 

  \return -1 on error
  \return 0 color table not found
  \return 1 on success
*/
int Rast_remove_colors(const char *name, const char *mapset)
{
    char element[GMAPSET_MAX + 6];
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    int stat;

    if (G_name_is_fully_qualified(name, xname, xmapset)) {
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
