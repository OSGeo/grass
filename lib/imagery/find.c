
/**************************************************************
* I_find_group (group)
*
* Find the a group in the current mapset
**************************************************************/
#include <grass/imagery.h>
#include <grass/gis.h>


/*!
 * \brief does group exist?
 *
 * Returns 1 if the
 * specified <b>group</b> exists in the current mapset; 0 otherwise.
 *
 *  \param group
 *  \return int
 */

int I_find_group(const char *group)
{
    if (group == NULL || *group == 0)
	return 0;

    return G_find_file2("group", group, G_mapset()) != NULL;
}

int I_find_group_file(const char *group, const char *file)
{
    if (!I_find_group(group))
	return 0;
    if (file == NULL || *file == 0)
	return 0;

    return G_find_file2_misc("group", file, group, G_mapset()) != NULL;
}

int I_find_subgroup(const char *group, const char *subgroup)
{
    char element[GNAME_MAX];

    if (!I_find_group(group))
	return 0;
    if (subgroup == NULL || *subgroup == 0)
	return 0;

    sprintf(element, "subgroup/%s", subgroup);

    return G_find_file2_misc("group", element, group, G_mapset()) != NULL;
}

int I_find_subgroup_file(const char *group, const char *subgroup,
			 const char *file)
{
    char element[GNAME_MAX * 2];

    if (!I_find_group(group))
	return 0;
    if (subgroup == NULL || *subgroup == 0)
	return 0;
    if (file == NULL || *file == 0)
	return 0;

    sprintf(element, "subgroup/%s/%s", subgroup, file);

    return G_find_file2_misc("group", element, group, G_mapset()) != NULL;
}
