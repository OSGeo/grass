/*!
   \file lib/gis/find_rast3d.c

   \brief GIS library - Find a 3D raster map

   SPDX-FileCopyrightText: 2001-2009, 2011 Other GRASS authors
   SPDX-License-Identifier: GPL-2.0-or-later

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
