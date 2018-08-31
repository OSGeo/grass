/*!
  \file lib/gis/find_rast3d.c
  
  \brief GIS library - Find a 3D raster map
  
  (C) 2001-2009, 2011 by the GRASS Development Team
  
`  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.

  \author Original author CERL
*/

#include <grass/gis.h>

/*!
  \brief Search for a 3D raster map in current search path or
  in a specified mapset.

  Note: rejects all names that begin with '.'

  \param name map name
  \param mapset  mapset to search. ("" for search path)
  
  \return pointer to a string with name of mapset where the map was found
  \return NULL if not found
*/
const char *G_find_raster3d(const char *name, const char *mapset)
{
    return G_find_file2_misc("grid3", "cell", name, mapset);
}
