/*!
   \file gis/find_file.c

   \brief GIS library - Find GRASS data base files

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Original author CERL
 */

#include <string.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>

static const char *find_file(int misc, const char *dir,
			     const char *element, const char *name,
			     const char *mapset)
{
    char path[GPATH_MAX];
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    const char *pname, *pmapset;
    int n;

    if (*name == 0)
	return NULL;
    *path = 0;

    /*
     * if name is in the fully qualified format, split it into
     * name, mapset (overrides what was in mapset)
     */
    if (G_name_is_fully_qualified(name, xname, xmapset)) {
	pname = xname;
	pmapset = xmapset;
    }
    else {
	pname = name;
	pmapset = mapset;
    }

    if (strcmp(element, "vector") == 0 &&
	pmapset && strcasecmp(pmapset, "ogr") == 0) {
	/* don't check for virtual OGR mapset */
	return G_store(pmapset);
    }
    
    /*
     * reject illegal names and mapsets
     */
    if (G_legal_filename(pname) == -1)
	return NULL;
    
    if (pmapset && *pmapset && G_legal_filename(pmapset) == -1)
	return NULL;

    /*
     * if no specific mapset is to be searched
     * then search all mapsets in the mapset search list
     */
    if (pmapset == NULL || *pmapset == 0) {
	int cnt = 0;
	const char *pselmapset = NULL;

	for (n = 0; (pmapset = G__mapset_name(n)); n++) {
	    if (misc)
		G_file_name_misc(path, dir, element, pname, pmapset);
	    else
		G_file_name(path, element, pname, pmapset);
	    if (access(path, 0) == 0) {
		if (!pselmapset)
		    pselmapset = pmapset;
		else
		    G_warning(_("'%s/%s' was found in more mapsets (also found in <%s>)"),
			      element, pname, pmapset);
		cnt++;
	    }
	}
	if (cnt > 0) {
	    /* If the same name exists in more mapsets and print a warning */
	    if (cnt > 1)
		G_warning(_("Using <%s@%s>"),
			  pname, pselmapset);
	    
	    return G_store(pselmapset);
	}
    }
    /*
     * otherwise just look for the file in the specified mapset.
     * since the name may have been qualified, mapset may point
     * to the xmapset, so we must should it to
     * permanent storage via G_store().
     */
    else {
	if (misc)
	    G_file_name_misc(path, dir, element, pname, pmapset);
	else
	    G_file_name(path, element, pname, pmapset);
	    
	if (access(path, 0) == 0)
	    return G_store(pmapset);
    }
    
    return NULL;
}



static const char *find_file1(
    int misc,
    const char *dir,
    const char *element, char *name, const char *mapset)
{
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    const char *pname, *pmapset;
    const char *mp;

    if (G_name_is_fully_qualified(name, xname, xmapset)) {
	pname = xname;
	pmapset = xmapset;
    }
    else {
	pname = name;
	pmapset = mapset;
    }

    mp = find_file(misc, dir, element, pname, pmapset);

    if (mp && name != pname)
	strcpy(name, pname);

    return mp;
}

/*!
 * \brief Searches for a file from the mapset search list or in a
 * specified mapset.
 *
 * Returns the mapset name where the file was found.
 *
 * If the user specifies a fully qualified element (name@mapset)
 * which exists, then G_find_file() modifies "name"
 * by removing the "@mapset" part.
 *
 * Rejects all names that begin with "."
 *
 * If <i>name</i> is of the form nnn in ppp then only mapset ppp
 * is searched.
 *
 * \param element database element (eg, "cell", "cellhd", "colr", etc)
 * \param name    file name to look for
 * \param mapset  mapset to search. if mapset is "" will search in mapset search list
 *
 * \return pointer to a string with name of mapset where file was
 * found, or NULL if not found
 */
const char *G_find_file(const char *element, char *name, const char *mapset)
{
    return find_file1(0, NULL, element, name, mapset);
}

/*!
 * \brief Searches for a file from the mapset search list or in a
 * specified mapset.
 *
 * Returns the mapset name where the file was found.
 *
 * \param dir     file directory
 * \param element database element (eg, "cell", "cellhd", "colr", etc)
 * \param name    file name to look for
 * \param mapset  mapset to search. if mapset is "" will search in mapset search list
 *
 * \return pointer to a string with name of mapset where file was
 * found, or NULL if not found
 */
const char *G_find_file_misc(const char *dir,
			     const char *element, char *name, const char *mapset)
{
    return find_file1(1, dir, element, name, mapset);
}

/*!
 * \brief Searches for a file from the mapset search list or in a
 * specified mapset. (look but don't touch)
 *
 * Returns the mapset name where the file was found.
 *
 * Exactly the same as G_find_file() except that if <i>name</i> is in
 * the form "<i>name@mapset</i>", and is found, G_find_file2() will
 * not alter <i>name</i> by removing the "@<i>mapset</i>" part.
 *
 * Rejects all names that begin with "."
 *
 * \param element    database element (eg, "cell", "cellhd", "colr", etc)
 * \param name       file name to look for
 * \param mapset     mapset to search. if mapset is "" will search in mapset search list
 *
 * \return pointer to a string with name of mapset where file was
 * found, or NULL if not found
 */
const char *G_find_file2(const char *element, const char *name, const char *mapset)
{
    return find_file(0, NULL, element, name, mapset);
}

/*!
 * \brief Searches for a file from the mapset search list or in a
 * specified mapset. (look but don't touch)
 *
 * Returns the mapset name where the file was found.
 *
 *
 * \param dir        file directory
 * \param element    database element (eg, "cell", "cellhd", "colr", etc)
 * \param name       file name to look for
 * \param mapset     mapset to search. if mapset is "" will search in mapset search list
 *
 * \return pointer to a string with name of mapset where file was
 * found, or NULL if not found
 */
const char *G_find_file2_misc(const char *dir,
			      const char *element,
			      const char *name, const char *mapset)
{
    return find_file(1, dir, element, name, mapset);
}
