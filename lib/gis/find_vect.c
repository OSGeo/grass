/*!
  \file gis/find_vect.c
  
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

#ifdef HAVE_OGR
#include <ogr_api.h>
#endif

static const char *find_ogr(const char *, const char *);

/*!
  \brief Finds a vector map
 
  Searches for a vector map from the mapset search list or in a
  specified mapset. Returns the mapset name where the vector map was
  found.
  
  NOTES:
  If the user specifies a fully qualified vector map which exists,
  then G_find_vector() modifies <i>name</i> by removing the
  "@<i>mapset</i>" part.
 
  Rejects all names that begin with "."
  
  \param name vector map name
  \param mapset mapset name or "" for search path

  \return pointer to a string with name of mapset where vector map was found
  \return NULL if not found
*/
const char *G_find_vector(char *name, const char *mapset)
{
    return G_find_file(GV_DIRECTORY, name, mapset);
}

/*!
 * \brief Find a vector map (look but don't touch)
 *
 * The same as G_find_vector() but doesn't remove the "@<i>mapset</i>"
 * qualification from <i>name</i>, if present.
 *
 * Returns NULL if the map wasn't found, or the mapset the vector was
 * found in if it was.
 *
 * \param name vector map name
 * \param mapset mapset name where to search
 *
 * \return pointer to buffer containing mapset name
 * \return NULL when vector map not found
 */
const char *G_find_vector2(const char *name, const char *mapset)
{
    const char *ogr_mapset;

    /* check OGR mapset first */
    ogr_mapset = find_ogr(name, mapset);
    if (ogr_mapset)
	return ogr_mapset;
    
    return G_find_file2(GV_DIRECTORY, name, mapset);
}

const char *find_ogr(const char *name, const char *mapset)
{
    const char *pname, *pmapset;
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    
    if(G__name_is_fully_qualified(name, xname, xmapset)) {
	pname = xname;
	pmapset = xmapset;
    }
    else {
	pname = name;
	pmapset = mapset;
    }
    
    if(pmapset && strcmp(pmapset, "OGR") == 0) {
	/* unique mapset "OGR", check OGR datasource instead */
#ifdef HAVE_OGR
	OGRDataSourceH Ogr_ds;
	
	G_debug(1, "OGR mapset detected");

	OGRRegisterAll();

	/* datasource handle */
	Ogr_ds = OGROpen(pname, FALSE, NULL);
	if (Ogr_ds == NULL)
	    G_fatal_error(_("Unable to open OGR data source '%s'"),
			  pname);
	
	return "OGR";
#else
	G_fatal_error(_("Unique OGR mapset detected, OGR support is missing"));
#endif
    }

    /* OGR mapset not detected */
    return NULL;
}
