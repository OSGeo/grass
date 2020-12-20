/*!
   \file lib/gis/find_file.c

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

static const char *find_element(int misc, const char *dir, const char *element)
{
    static const char *cell_elements[] = {
	"cellhd",
	"cell",
	"cats",
	"colr",
	"hist",
	"cell_misc",
	"fcell",
	"g3dcell",
	NULL
    };
    static const char *dig_elements[] = {
	"dig",
	"dig_att",
	"dig_plus",
	"dig_cats",
	"dig_misc",
	"reg",
	NULL
    };
    const char *search = misc ? dir : element;
    int i;

    for (i = 1; cell_elements[i]; i++)
	if (strcmp(search, cell_elements[i]) == 0)
	    return cell_elements[0];
    for (i = 1; dig_elements[i]; i++)
	if (strcmp(search, dig_elements[i]) == 0)
	    return dig_elements[0];
    return element;
}

static const char *find_file(int misc, const char *dir,
			     const char *element, const char *name,
			     const char *subproject)
{
    char path[GPATH_MAX];
    char xname[GNAME_MAX], xsubproject[GMAPSET_MAX];
    const char *pname, *psubproject;
    int n;

    if (*name == 0)
	return NULL;
    *path = 0;

    /*
     * if name is in the fully qualified format, split it into
     * name, subproject (overrides what was in subproject)
     */
    if (G_name_is_fully_qualified(name, xname, xsubproject)) {
	pname = xname;
	psubproject = xsubproject;
    }
    else {
	pname = name;
	psubproject = subproject;
    }

    if (strcmp(element, "vector") == 0 &&
	psubproject && strcasecmp(psubproject, "ogr") == 0) {
	/* don't check for virtual OGR subproject */
	return G_store(psubproject);
    }
    
    /*
     * reject illegal names and subprojects
     */
    if (G_legal_filename(pname) == -1)
	return NULL;
    
    if (psubproject && *psubproject && G_legal_filename(psubproject) == -1)
	return NULL;

    /*
     * if no specific subproject is to be searched
     * then search all subprojects in the subproject search list
     */
    if (psubproject == NULL || *psubproject == 0) {
	int cnt = 0;
	const char *pselsubproject = NULL;
	const char *pelement = find_element(misc, dir, element);

	for (n = 0; (psubproject = G_get_subproject_name(n)); n++) {
	    if (misc && element == pelement)
		G_file_name_misc(path, dir, pelement, pname, psubproject);
	    else
		G_file_name(path, pelement, pname, psubproject);
	    if (access(path, 0) == 0) {
		if (!pselsubproject)
		    pselsubproject = psubproject;
		else if (element == pelement)
		    G_important_message(_("Data element '%s/%s' was found in more subprojects (also found in <%s>)"),
                                        element, pname, psubproject);
		cnt++;
	    }
	}
	if (cnt > 0) {
	    if (misc)
		G_file_name_misc(path, dir, element, pname, pselsubproject);
	    else
		G_file_name(path, element, name, pselsubproject);
	    if (access(path, 0) == 0) {
		/* If the same name exists in more subprojects and print a warning */
		if (cnt > 1 && element == pelement)
		    G_important_message(_("Using <%s@%s>..."),
			      pname, pselsubproject);
	    
		return G_store(pselsubproject);
	    }
	}
    }
    /*
     * otherwise just look for the file in the specified subproject.
     * since the name may have been qualified, subproject may point
     * to the xsubproject, so we must should it to
     * permanent storage via G_store().
     */
    else {
	if (misc)
	    G_file_name_misc(path, dir, element, pname, psubproject);
	else
	    G_file_name(path, element, pname, psubproject);
	    
	if (access(path, 0) == 0)
	    return G_store(psubproject);
    }
    
    return NULL;
}



static const char *find_file1(
    int misc,
    const char *dir,
    const char *element, char *name, const char *subproject)
{
    char xname[GNAME_MAX], xsubproject[GMAPSET_MAX];
    const char *pname, *psubproject;
    const char *mp;

    if (G_name_is_fully_qualified(name, xname, xsubproject)) {
	pname = xname;
	psubproject = xsubproject;
    }
    else {
	pname = name;
	psubproject = subproject;
    }

    mp = find_file(misc, dir, element, pname, psubproject);

    if (mp && name != pname)
	strcpy(name, pname);

    return mp;
}

/*!
 * \brief Searches for a file from the subproject search list or in a
 * specified subproject.
 *
 * Returns the subproject name where the file was found.
 *
 * If the user specifies a fully qualified element (name@subproject)
 * which exists, then G_find_file() modifies "name"
 * by removing the "@subproject" part.
 *
 * Rejects all names that begin with "."
 *
 * If <i>name</i> is of the form nnn in ppp then only subproject ppp
 * is searched.
 *
 * \param element database element (eg, "cell", "cellhd", "colr", etc)
 * \param name    file name to look for
 * \param subproject  subproject to search. if subproject is "" will search in subproject search list
 *
 * \return pointer to a string with name of subproject where file was
 * found, or NULL if not found
 */
const char *G_find_file(const char *element, char *name, const char *subproject)
{
    return find_file1(0, NULL, element, name, subproject);
}

/*!
 * \brief Searches for a file from the subproject search list or in a
 * specified subproject.
 *
 * Returns the subproject name where the file was found.
 *
 * \param dir     file directory
 * \param element database element (eg, "cell", "cellhd", "colr", etc)
 * \param name    file name to look for
 * \param subproject  subproject to search. if subproject is "" will search in subproject search list
 *
 * \return pointer to a string with name of subproject where file was
 * found, or NULL if not found
 */
const char *G_find_file_misc(const char *dir,
			     const char *element, char *name, const char *subproject)
{
    return find_file1(1, dir, element, name, subproject);
}

/*!
 * \brief Searches for a file from the subproject search list or in a
 * specified subproject. (look but don't touch)
 *
 * Returns the subproject name where the file was found.
 *
 * Exactly the same as G_find_file() except that if <i>name</i> is in
 * the form "<i>name@subproject</i>", and is found, G_find_file2() will
 * not alter <i>name</i> by removing the "@<i>subproject</i>" part.
 *
 * Rejects all names that begin with "."
 *
 * \param element    database element (eg, "cell", "cellhd", "colr", etc)
 * \param name       file name to look for
 * \param subproject     subproject to search. if subproject is "" will search in subproject search list
 *
 * \return pointer to a string with name of subproject where file was
 * found, or NULL if not found
 */
const char *G_find_file2(const char *element, const char *name, const char *subproject)
{
    return find_file(0, NULL, element, name, subproject);
}

/*!
 * \brief Searches for a file from the subproject search list or in a
 * specified subproject. (look but don't touch)
 *
 * Returns the subproject name where the file was found.
 *
 *
 * \param dir        file directory
 * \param element    database element (eg, "cell", "cellhd", "colr", etc)
 * \param name       file name to look for
 * \param subproject     subproject to search. if subproject is "" will search in subproject search list
 *
 * \return pointer to a string with name of subproject where file was
 * found, or NULL if not found
 */
const char *G_find_file2_misc(const char *dir,
			      const char *element,
			      const char *name, const char *subproject)
{
    return find_file(1, dir, element, name, subproject);
}
