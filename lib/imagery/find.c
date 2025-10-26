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
 * \param mapset
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

    snprintf(element, sizeof(element), "subgroup%c%s", HOST_DIRSEP, subgroup);
    G_debug(5, "I_find_subgroup() element: %s", element);

    return G_find_file2_misc("group", element, group, G_mapset()) != NULL;
}

/*!
 * \brief Searches for a subgroup in specified mapset or any mapset if mapset is
 * not set
 *
 * \param group
 * \param subgroup
 * \param mapset
 * \return int 1 if subrgoup was found, 0 otherwise
 */
int I_find_subgroup2(const char *group, const char *subgroup,
                     const char *mapset)
{
    char element[GNAME_MAX];

    if (!I_find_group2(group, mapset))
        return 0;
    if (subgroup == NULL || *subgroup == 0)
        return 0;

    snprintf(element, sizeof(element), "subgroup%c%s", HOST_DIRSEP, subgroup);
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

    snprintf(element, sizeof(element), "subgroup%c%s%c%s", HOST_DIRSEP,
             subgroup, HOST_DIRSEP, file);
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

    snprintf(element, sizeof(element), "subgroup%c%s%c%s", HOST_DIRSEP,
             subgroup, HOST_DIRSEP, file);
    G_debug(5, "I_find_subgroup_file2() element: %s", element);

    return G_find_file2_misc("group", element, group, mapset) != NULL;
}

/*!
 * \brief Find mapset containing signature file
 *
 * Looks for the signature <i>name</i> of type <i>type</i>
 * in the database. The <i>mapset</i> parameter can either be
 * the empty string "", which means search all the mapsets in
 * the users current mapset search path
 * (see \ref Mapset_Search_Path for more details about the search
 * path) or it can be a specific mapset name, which means look for the
 * signature only in this one mapset (for example, in the current
 * mapset). If found, the mapset where the signature lives is
 * returned. If not found, the NULL pointer is returned.
 *
 * Note: If the user specifies a fully qualified signature name which
 * exists, then I_find_signature() modifies <i>name</i> by removing
 * the "@<i>mapset</i>".
 * Use I_find_signature2 if altering passed in name is not desired.
 *
 * \param type I_SIGFILE_TYPE
 * \param name of signature
 * \param mapset set NULL to search in all mapsets
 * \return mapset or NULL
 */
const char *I_find_signature(I_SIGFILE_TYPE type, char *name,
                             const char *mapset)
{
    char sdir[GNAME_MAX]; /* 'signatures/type\0' */

    G_debug(1, "I_find_signature(): type=%d name=%s mapset=%s", type, name,
            mapset);

    I_get_signatures_dir(sdir, type);

    /* We do not search for a specific file as file name is up to signature type
     */
    return G_find_file(sdir, name, mapset);
}

/*!
 * \brief Find mapset containing signature (look but don't touch)
 *
 * Looks for the signature <i>name</i> of type <i>type</i>
 * in the database. The <i>mapset</i> parameter can either be
 * the empty string "", which means search all the mapsets in
 * the users current mapset search path
 * (see \ref Mapset_Search_Path for more details about the search
 * path) or it can be a specific mapset name, which means look for the
 * signature only in this one mapset (for example, in the current
 * mapset). If found, the mapset where the signature lives is
 * returned. If not found, the NULL pointer is returned.
 *
 * Note: The passed name argument is not altered.
 * Use I_find_signature if stripping mapset part from the name is desired.
 *
 * \param type I_SIGFILE_TYPE
 * \param name of signature
 * \param mapset set NULL to search in all mapsets
 * \return mapset or NULL
 */
const char *I_find_signature2(I_SIGFILE_TYPE type, const char *name,
                              const char *mapset)
{
    char sdir[GNAME_MAX]; /* 'signatures/type\0' */

    G_debug(1, "I_find_signature2(): type=%d name=%s mapset=%s", type, name,
            mapset);

    I_get_signatures_dir(sdir, type);

    return G_find_file2(sdir, name, mapset);
}
