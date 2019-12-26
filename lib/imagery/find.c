
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
 * Use I_find_group2 to search in all or a specific mapset.
 *
 *  \param group
 *  \return int 1 if group was found, 0 otherwise
 */

int I_find_group(const char *group)
{
    if (group == NULL || *group == 0)
        return 0;

    return G_find_file2("group", group, G_mapset()) != NULL;
}

/*!
 * \brief Does the group exists?
 *
 *  Finds a group in specified mapset or any mapset if mapset is not set.
 *  Internally uses G_find_file2().
 *
 *  \param group
 *  \param mapset
 *  \return int 1 if group was found, 0 otherwise
 */

int I_find_group2(const char *group, const char *mapset)
{
    return G_find_file2("group", group, mapset) != NULL;
}

/*!
 * \brief Searches for a group file in the current mapset
 * 
 * \param group
 * \param file
 * \return int 1 if file was found, 0 otherwise
 */
int I_find_group_file(const char *group, const char *file)
{
    if (!I_find_group(group))
        return 0;
    if (file == NULL || *file == 0)
        return 0;

    return G_find_file2_misc("group", file, group, G_mapset()) != NULL;
}

/*!
 * \brief Searches for a group file in the specified mapset
 * 
 * \param group
 * \param file
 * \return int 1 if file was found, 0 otherwise
 */
int I_find_group_file2(const char *group, const char *mapset, const char *file)
{
    if (!I_find_group2(group, mapset))
        return 0;
    if (file == NULL || *file == 0)
        return 0;

    return G_find_file2_misc("group", file, group, mapset) != NULL;
}

/*!
 * \brief Searches for a subgroup in the current mapset
 * 
 * \param group
 * \param subgroup
 * \return int 1 if subgroup was found, 0 otherwise
 */
int I_find_subgroup(const char *group, const char *subgroup)
{
    char element[GNAME_MAX];

    if (!I_find_group(group))
        return 0;
    if (subgroup == NULL || *subgroup == 0)
        return 0;

    sprintf(element, "subgroup%c%s", HOST_DIRSEP, subgroup);
    G_debug(5, "I_find_subgroup() element: %s", element);

    return G_find_file2_misc("group", element, group, G_mapset()) != NULL;
}

/*!
 * \brief Searches for a subgroup in specified mapset or any mapset if mapset is not set
 * 
 * \param group
 * \param subgroup
 * \param mapset
 * \return int 1 if subrgoup was found, 0 otherwise
 */
int I_find_subgroup2(const char *group, const char *subgroup, const char *mapset)
{
    char element[GNAME_MAX];

    if (!I_find_group2(group, mapset))
        return 0;
    if (subgroup == NULL || *subgroup == 0)
        return 0;

    sprintf(element, "subgroup%c%s", HOST_DIRSEP, subgroup);
    G_debug(5, "I_find_subgroup2() element: %s", element);

    return G_find_file2_misc("group", element, group, mapset) != NULL;
}

/*!
 * \brief Searches for a subgroup file in the current mapset
 * 
 * \param group
 * \param subgroup
 * \param file
 * \return int 1 if file was found, 0 otherwise
 */
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

    sprintf(element, "subgroup%c%s%c%s", HOST_DIRSEP, subgroup, HOST_DIRSEP, file);
    G_debug(5, "I_find_subgroup_file() element: %s", element);

    return G_find_file2_misc("group", element, group, G_mapset()) != NULL;
}

/*!
 * \brief Searches for a subgroup file in the specified mapset
 * 
 * \param group
 * \param subgroup
 * \param mapset
 * \param file
 * \return int 1 if file was found, 0 otherwise
 */
int I_find_subgroup_file2(const char *group, const char *subgroup,
                         const char *mapset, const char *file)
{
    char element[GNAME_MAX * 2];

    if (!I_find_group2(group, mapset))
        return 0;
    if (subgroup == NULL || *subgroup == 0)
        return 0;
    if (file == NULL || *file == 0)
        return 0;

    sprintf(element, "subgroup%c%s%c%s", HOST_DIRSEP, subgroup, HOST_DIRSEP, file);
    G_debug(5, "I_find_subgroup_file2() element: %s", element);

    return G_find_file2_misc("group", element, group, mapset) != NULL;
}

/*!
 * \brief does signature file exists?
 *
 * Returns 1 if the
 * specified <b>signature</b> exists in the specified subgroup; 0 otherwise.
 * 
 * Should be used to check if signature file exists after G_parser run
 * when generating new signature file.
 *
 *  \param group - group where to search
 *  \param subgroup - subgroup containing signatures
 *  \param type - type of signature ("sig" or "sigset")
 *  \param file - name of signature file
 *  \return int
 */
int I_find_signature_file(const char *group, const char *subgroup,
                     const char *type, const char *file)
{
    char element[GNAME_MAX * 2];
    
    if (!I_find_group(group))
        return 0;
    if (subgroup == NULL || *subgroup == 0)
        return 0;
    if (type == NULL || *type == 0)
        return 0;
    if (file == NULL || *file == 0)
        return 0;
        
    sprintf(element, "subgroup%c%s%c%s%c%s", HOST_DIRSEP, subgroup, HOST_DIRSEP, type, HOST_DIRSEP, file);
    G_debug(5, "I_find_signature_file() element: %s", element);
    
    return G_find_file2_misc("group", element, group, G_mapset()) != NULL;
}
