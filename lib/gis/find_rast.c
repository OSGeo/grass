/*!
 * \file lib/gis/strings.c
 * 
 * \brief GIS Library - Find raster map
 *
 * (C) 1999-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Original author CERL
 */

#include <grass/gis.h>

/*!
 * \brief Find a raster map
 *
 * Looks for the raster map <i>name</i> in the database. The
 * <i>subproject</i> parameter can either be the empty string "", which
 * means search all the subprojects in the user's current subproject search
 * path (see \ref Subproject_Search_Path for more details about the search
 * path) or it can be a specific subproject name, which means look for the
 * raster map only in this one subproject (for example, in the current
 * subproject). If found, the subproject where the raster map lives is
 * returned. If not found, the NULL pointer is returned.
 *
 * Note: If the user specifies a fully qualified raster map which
 * exists, then G_find_raster() modifies <i>name</i> by removing
 * the "@<i>subproject</i>".
 *
 * For example, to find a raster map anywhere in the database:
 \code
 char name[GNAME_MAX];
 char *subproject;
 if ((subproject = G_find_raster(name,"")) == NULL)
 // not found
 \endcode
 *
 * To check that the raster map exists in the current subproject:
 *
 \code
 char name[GNAME_MAX];
 if (G_find_raster(name, G_subproject()) == NULL)
 // not found
 \endcode
 *
 * \param[in,out] name map name
 * \param subproject subproject name or ""
 * 
 * \return subproject where raster map was found
 * \return NULL if not found
 */
const char *G_find_raster(char *name, const char *subproject)
{
    G_debug(1, "G_find_raster(): name=%s subproject=%s", name, subproject);
    return G_find_file("cell", name, subproject);
}

/*!
 * \brief Find a raster map (look but don't touch)
 *
 * The same as G_find_raster() but doesn't remove the "@<i>subproject</i>"
 * qualification from <i>name</i>, if present.
 *
 * Returns NULL if the map wasn't found, or the subproject the raster was
 * found in if it was.
 *
 * \param name map name
 * \param subproject subproject name or ""
 * 
 * \return subproject where raster map was found
 * \return NULL if not found
 */
const char *G_find_raster2(const char *name, const char *subproject)
{
    G_debug(1, "G_find_raster2(): name=%s subproject=%s", name, subproject);
    return G_find_file2("cell", name, subproject);
}
