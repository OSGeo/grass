/*!
   \file lib/imagery/sigfile.c
   
   \brief Imagery Library - Signature file functions (statistics for i.maxlik).
 
   (C) 2001-2008, 2013 by the GRASS Development Team
   
   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.
   
   \author USA CERL
*/

#include <string.h>
#include <grass/imagery.h>

/*!
   \brief Create signature file

   \param group group name
   \param subgroup subgroup name in given group
   \param name signature filename

   \return pointer to FILE*
   \return NULL on error
*/
FILE *I_fopen_signature_file_new(const char *group,
				 const char *subgroup, const char *name)
{
    char element[GPATH_MAX];
    char group_name[GNAME_MAX], group_mapset[GMAPSET_MAX];
    FILE *fd;

    if (!G_name_is_fully_qualified(group, group_name, group_mapset)) {
	strcpy(group_name, group);
    }

    /* create sigset directory */
    sprintf(element, "%s/subgroup/%s/sig", group_name, subgroup);
    G__make_mapset_element_misc("group", element);

    sprintf(element, "subgroup/%s/sig/%s", subgroup, name);

    fd = G_fopen_new_misc("group", element, group_name);
    
    return fd;
}

/*!
   \brief Open existing signature file

   \param group group name (may be fully qualified)
   \param subgroup subgroup name in given group
   \param name signature filename

   \return pointer to FILE*
   \return NULL on error
*/
FILE *I_fopen_signature_file_old(const char *group,
				 const char *subgroup, const char *name)
{
    char element[GPATH_MAX];
    char group_name[GNAME_MAX], group_mapset[GMAPSET_MAX];
    FILE *fd;

    G_unqualified_name(group, NULL, group_name, group_mapset);
    sprintf(element, "subgroup/%s/sig/%s", subgroup, name);

    fd = G_fopen_old_misc("group", element, group_name, group_mapset);
    
    return fd;
}
