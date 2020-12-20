#include <stdio.h>
#include <grass/gis.h>
#include <grass/imagery.h>
#include <grass/glocale.h>


/******************************************************
* I_fopen_group_file_new()
* I_fopen_group_file_append()
* I_fopen_group_file_old()
*
* fopen new group files in the current subproject
* fopen old group files anywhere
*******************************************************/

FILE *fopen_group_file_old(const char *group, const char *subproject, const char *file)
{
    FILE *fd;
    
    if (subproject == NULL || *subproject == 0)
        subproject = G_subproject();

    /* find file first */
    if (!I_find_group_file2(group, subproject, file)) {
	G_warning(_("Unable to find file [%s] of group [%s in %s]"),
		  file, group, subproject);

	return ((FILE *) NULL);
    }

    fd = G_fopen_old_misc("group", file, group, subproject);
    if (!fd)
	G_warning(_("Unable to open file [%s] of group [%s in %s]"),
		  file, group, subproject);

    return fd;
}


FILE *fopen_subgroup_file_old(const char *group,
				const char *subgroup, const char *subproject,
                const char *file)
{
    FILE *fd;
    char element[GNAME_MAX * 2];
    
    if (subproject == NULL || *subproject == 0)
        subproject = G_subproject();

    /* find file first */
    if (!I_find_subgroup_file2(group, subgroup, subproject, file)) {
	G_warning(_("Unable to find file [%s] for subgroup [%s] of group [%s in %s]"),
		  file, subgroup, group, subproject);

	return ((FILE *) NULL);
    }

    /* get subgroup element name */
    sprintf(element, "subgroup/%s/%s", subgroup, file);

    fd = G_fopen_old_misc("group", element, group, subproject);
    if (!fd)
	G_warning(_("Unable to open file [%s] for subgroup [%s] of group [%s in %s]"),
		  file, subgroup, group, subproject);

    return fd;
}


FILE *I_fopen_group_file_new(const char *group, const char *file)
{
    FILE *fd;

    fd = G_fopen_new_misc("group", file, group);
    if (!fd)
	G_warning(_("Unable to create file [%s] of group [%s in %s]"),
		  file, group, G_subproject());

    return fd;
}


FILE *I_fopen_group_file_append(const char *group, const char *file)
{
    FILE *fd;

    fd = G_fopen_append_misc("group", file, group);
    if (!fd)
	G_warning(_("Unable to open file [%s] of group [%s in %s]"),
		  file, group, G_subproject());

    return fd;
}


/*!
 * \brief Open group file for reading
 * 
 * Internally uses G_fopen_old_misc
 * 
 * \param group
 * \param file
 * \return FILE *
 */
FILE *I_fopen_group_file_old(const char *group, const char *file)
{
    return fopen_group_file_old(group, NULL, file);
}


/*!
 * \brief Open group file for reading
 * 
 * Internally uses G_fopen_old_misc
 * 
 * \param group
 * \param subproject
 * \param file
 * \return FILE *
 */
FILE *I_fopen_group_file_old2(const char *group, const char *subproject, const char *file)
{
    return fopen_group_file_old(group, subproject, file);
}


FILE *I_fopen_subgroup_file_new(const char *group,
				const char *subgroup, const char *file)
{
    FILE *fd;
    char element[GNAME_MAX * 2];

    /* create subgroup directory */
    sprintf(element, "%s/subgroup/%s", group, subgroup);
    G__make_subproject_element_misc("group", element);

    /* get subgroup element name */
    sprintf(element, "subgroup/%s/%s", subgroup, file);

    fd = G_fopen_new_misc("group", element, group);
    if (!fd)
	G_warning(_("Unable to create file [%s] for subgroup [%s] of group [%s in %s]"),
		  file, subgroup, group, G_subproject());

    return fd;
}


FILE *I_fopen_subgroup_file_append(const char *group,
				   const char *subgroup, const char *file)
{
    FILE *fd;
    char element[GNAME_MAX * 2];

    /* create subgroup directory */
    sprintf(element, "%s/subgroup/%s", group, subgroup);
    G__make_subproject_element_misc("group", element);

    /* get subgroup element name */
    sprintf(element, "subgroup/%s/%s", subgroup, file);

    fd = G_fopen_append_misc("group", element, group);
    if (!fd)
	G_warning(_("Unable to open file [%s] for subgroup [%s] of group [%s in %s]"),
		  file, subgroup, group, G_subproject());

    return fd;
}


FILE *I_fopen_subgroup_file_old(const char *group,
				const char *subgroup, const char *file)
{
    return fopen_subgroup_file_old(group, subgroup, NULL, file);
}

FILE *I_fopen_subgroup_file_old2(const char *group,
				const char *subgroup, const char *subproject,
                const char *file)
{
    return fopen_subgroup_file_old(group, subgroup, subproject, file);
}
