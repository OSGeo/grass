/*!
  \file gis/mapset.c
  
  \brief GIS library - environment routines (mapset)
  
  (C) 2001-2009 by the GRASS Development Team
  
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.

  \author Original author CERL
 */

#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>

/*!
 * \brief Get current mapset name
 *
 * Returns the name of the current mapset in the current
 * location. This routine is often used when accessing files in the
 * current mapset. See Mapsets for an explanation of mapsets.
 *
 * G_fatal_error() is called on error.
 *
 * \return pointer mapset name
 */
const char *G_mapset(void)
{
    const char *m = G__mapset();

    if (!m)
	G_fatal_error(_("MAPSET is not set"));

    return m;
}

/*!
 * \brief Get current mapset name
 *
 * \return pointer mapset name
 * \return NULL on error
 */
const char *G__mapset(void)
{
    return G__getenv("MAPSET");
}
