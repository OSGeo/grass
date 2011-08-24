/*!
  \file lib/vector/Vlib/color_remove.c
 
  \brief Vector Library - remove color table of vector map
  
  (C) 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Martin Landa <landa.martin gmail.com>
*/

#include <string.h>

#include <grass/gis.h>
#include <grass/vector.h>

/*!
  \brief Remove color table of raster map

  \param name name of raster map
  \param mapset name of mapset 

  \return -1 on error
  \return 0 color table not found
  \return 1 on success
*/
int Vect_remove_colors(const char *name, const char *mapset)
{
    char element[GPATH_MAX];
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    int stat;

    if (G_name_is_fully_qualified(name, xname, xmapset)) {
	if (strcmp(xmapset, mapset) != 0)
	    return -1;
	name = xname;
    }

    /* get rid of existing colr2, if any */
    sprintf(element, "%s/%s", GV_COLR2_DIRECTORY, mapset);
    stat = G_remove(element, name);

    if (strcmp(mapset, G_mapset()) == 0) {
	sprintf(element, "%s/%s", GV_DIRECTORY, name);
	stat = G_remove(element, GV_COLR_ELEMENT);
    }
    
    return stat;
}
