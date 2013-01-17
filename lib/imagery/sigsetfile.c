/*!
  \file lib/imagery/sigsetfile.c
 
  \brief Imagery Library - Open/Create signiture files
 
  (C) 2001-2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author GRASS GIS Development Team
*/

#include <string.h>

#include <grass/gis.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

/*!
  \brief Create new signiture file in given group/subgroup

  Note: Prints warning on error and returns NULL.

  \param group name of group
  \param subgroup name of subgroup
  \param name name of signiture file

  \return pointer to FILE
  \return NULL on error
*/
FILE *I_fopen_sigset_file_new(const char *group, const char *subgroup,
			      const char *name)
{
    char element[GPATH_MAX];
    char group_name[GNAME_MAX], mapset[GMAPSET_MAX];
    FILE *fd;

    if (G_name_is_fully_qualified(group, group_name, mapset)) {
	if (strcmp(mapset, G_mapset()) != 0)
	    G_warning(_("Unable to create signature file <%s> for subgroup <%s> "
			"of group <%s> - <%s> is not current mapset"),
		      name, subgroup, group, mapset);
    }
    else { 
	strcpy(group_name, group);
    }

    /* create sigset directory */
    sprintf(element, "%s/subgroup/%s/sigset", group_name, subgroup);
    G__make_mapset_element_misc("group", element);

    sprintf(element, "subgroup/%s/sigset/%s", subgroup, name);

    fd = G_fopen_new_misc("group", element, group_name);
    if (fd == NULL)
	G_warning(_("Unable to create signature file <%s> for subgroup <%s> "
		    "of group <%s>"),
		  name, subgroup, group);
    
    return fd;
}

/*!
  \brief Open existing signiture file in given group/subgroup

  Note: Prints warning on error and returns NULL.

  \param group name of group
  \param subgroup name of subgroup
  \param name name of signiture file

  \return pointer to FILE
  \return NULL on error
*/
FILE *I_fopen_sigset_file_old(const char *group, const char *subgroup,
			      const char *name)
{
    char element[GPATH_MAX];
    char group_name[GNAME_MAX], group_mapset[GMAPSET_MAX];
    FILE *fd;

    if (!G_name_is_fully_qualified(group, group_name, group_mapset)) {
	strcpy(group_name, group);
    }

    sprintf(element, "subgroup/%s/sigset/%s", subgroup, name);

    fd = G_fopen_old_misc("group", element, group_name, G_mapset());
    
    return fd;
}
