/*!
  \file lib/gis/find_vect.c
  
  \brief GIS library - Find a vector map
  
  (C) 2001-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.

  \author Original author CERL
*/

#include <string.h>

#include <grass/config.h>
#include <grass/gis.h>
#include <grass/vect/dig_defines.h>
#include <grass/glocale.h>

/*!
  \brief Finds a vector map
 
  Searches for a vector map from the subproject search list or in a
  specified subproject. Returns the subproject name where the vector map was
  found.
  
  NOTES:
  If the user specifies a fully qualified vector map which exists,
  then G_find_vector() modifies <i>name</i> by removing the
  "@<i>subproject</i>" part.
 
  Rejects all names that begin with "."
  
  \param name vector map name
  \param subproject subproject name or "" for search path

  \return pointer to a string with name of subproject where vector map was found
  \return NULL if not found
*/
const char *G_find_vector(char *name, const char *subproject)
{
    G_debug(1, "G_find_vector(): name=%s subproject=%s", name, subproject);
    return G_find_file(GV_DIRECTORY, name, subproject);
}

/*!
 * \brief Find a vector map (look but don't touch)
 *
 * The same as G_find_vector() but doesn't remove the "@<i>subproject</i>"
 * qualification from <i>name</i>, if present.
 *
 * Returns NULL if the map wasn't found, or the subproject the vector was
 * found in if it was.
 *
 * \param name vector map name
 * \param subproject subproject name where to search
 *
 * \return pointer to buffer containing subproject name
 * \return NULL when vector map not found
 */
const char *G_find_vector2(const char *name, const char *subproject)
{
    G_debug(1, "G_find_vector2(): name=%s subproject=%s", name, subproject);
    return G_find_file2(GV_DIRECTORY, name, subproject);
}

