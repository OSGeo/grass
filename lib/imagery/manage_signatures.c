/*!
   \file lib/imagery/manage_signatures.c

   \brief Imagery Library - Signature file management functions

   (C) 2021 by Maris Nartiss and the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Maris Nartiss
 */

#include <unistd.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

/*!
   \brief Get signature directory

   The directory will be in a form "signatures/<type>".

   \param dir [GNAME_MAX] allocated string buffer
   \param type I_SIGFILE_TYPE
 */
void I_get_signatures_dir(char *dir, I_SIGFILE_TYPE type)
{
    if (type == I_SIGFILE_TYPE_SIG) {
        sprintf(dir, "signatures%csig", HOST_DIRSEP);
    }
    else if (type == I_SIGFILE_TYPE_SIGSET) {
        sprintf(dir, "signatures%csigset", HOST_DIRSEP);
    }
    else if (type == I_SIGFILE_TYPE_LIBSVM) {
        sprintf(dir, "signatures%clibsvm", HOST_DIRSEP);
    }
    else {
        G_fatal_error("Programming error: unknown signature file type");
    }
}

/*!
   \brief Make signature dir

   Creates directories for storage of signature files of specified type.
   E.g. "<location>/<mapset>/signatures/<type>/"

   \param type I_SIGFILE_TYPE
 */
void I_make_signatures_dir(I_SIGFILE_TYPE type)
{
    char dir[GNAME_MAX];

    G_make_mapset_object_group("signatures");
    I_get_signatures_dir(dir, type);
    G_make_mapset_object_group(dir);
}

static int list_by_type(I_SIGFILE_TYPE, const char *, int, char ***);

/*!
 * \brief Remove a signature file
 *
 * If removal fails, prints a warning and returns 1.
 * It is safe to pass fully qualified names.
 *
 * \param type I_SIGFILE_TYPE signature type
 * \param name of signature to remove
 * \return 0 on success
 * \return 1 on failure
 */
int I_signatures_remove(I_SIGFILE_TYPE type, const char *name)
{
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    char dir[GNAME_MAX];

    G_debug(1, "I_signatures_remove(%d, %s);", type, name);

    /* Remove only if file is in the current mapset */
    if (G_name_is_fully_qualified(name, xname, xmapset) &&
        strcmp(xmapset, G_mapset()) != 0) {
        G_warning(_("%s is not in the current mapset (%s)"), name, G_mapset());
        return 1;
    }
    if (I_find_signature2(type, name, G_mapset())) {
        I_get_signatures_dir(dir, type);
        if (G_remove(dir, name) == 1) {
            G_verbose_message(_("%s removed"), name);
            return 0;
        }
        G_warning(_("Unable to remove %s signature"), name);
    }
    else
        G_warning(_("%s is missing"), name);
    return 1;
}

/*!
 * \brief Copy a signature file
 *
 * If copy fails, prints warning messages and returns 1.
 * It is safe to pass fully qualified names.
 *
 * \param type I_SIGFILE_TYPE signature type
 * \param old_name of old signature
 * \param old_mapset of old signature
 * \param new_name of new signature
 * \return 0 on success
 * \return 1 on failure
 */
int I_signatures_copy(I_SIGFILE_TYPE type, const char *old_name,
                      const char *old_mapset, const char *new_name)
{
    char sname[GNAME_MAX], tname[GNAME_MAX], tmapset[GMAPSET_MAX],
        xmapset[GMAPSET_MAX];
    char dir[GNAME_MAX];
    const char *smapset;
    char old_path[GPATH_MAX], new_path[GPATH_MAX];

    G_debug(1, "I_signatures_copy(%d, %s@%s, %s);", type, old_name, old_mapset,
            new_name);

    /* Copy only if mapset of new name is the current mapset */
    if (G_name_is_fully_qualified(new_name, tname, tmapset)) {
        if (strcmp(tmapset, G_mapset()) != 0) {
            G_warning(_("%s is not in the current mapset (%s)"), new_name,
                      G_mapset());
            return 1;
        }
    }
    else
        strcat(tname, new_name);

    smapset = I_find_signature2(type, old_name, old_mapset);
    if (!smapset) {
        G_warning(_("%s is missing"), old_name);
        return 1;
    }
    G_unqualified_name(old_name, NULL, sname, xmapset);

    I_make_signatures_dir(type);

    I_get_signatures_dir(dir, type);
    /* Note – we need whole directory not just an element in it thus
       G_file_name and not G_file_name_misc */
    G_file_name(old_path, dir, sname, smapset);
    G_file_name(new_path, dir, tname, G_mapset());

    if (G_recursive_copy(old_path, new_path) != 0) {
        char *mname = G_fully_qualified_name(old_name, smapset);
        G_warning(_("Unable to copy <%s> to current mapset as <%s>"), mname,
                  tname);
        G_free(mname);
        return 1;
    }
    return 0;
}

/*!
 * \brief Rename a signature file
 *
 * If rename fails, prints warning messages and returns 1.
 * It is safe to pass fully qualified names.
 *
 * \param type I_SIGFILE_TYPE signature type
 * \param old_name name of old signature
 * \param new_name name of new signature
 * \return 0 on success
 * \return 1 on failure
 */
int I_signatures_rename(I_SIGFILE_TYPE type, const char *old_name,
                        const char *new_name)
{
    char sname[GNAME_MAX], tname[GNAME_MAX], tmapset[GMAPSET_MAX];
    char dir[GNAME_MAX];
    const char *smapset;
    char old_path[GPATH_MAX], new_path[GPATH_MAX];

    G_debug(1, "I_signatures_rename(%d, %s, %s);", type, old_name, new_name);

    /* Rename only if source and destination mapset is the current mapset */
    if (G_name_is_fully_qualified(old_name, sname, tmapset)) {
        if (strcmp(tmapset, G_mapset()) != 0) {
            G_warning(_("%s is not in the current mapset (%s)"), old_name,
                      G_mapset());
            return 1;
        }
    }
    else
        strcat(sname, old_name);
    if (G_name_is_fully_qualified(new_name, tname, tmapset)) {
        if (strcmp(tmapset, G_mapset()) != 0) {
            G_warning(_("%s is not in the current mapset (%s)"), new_name,
                      G_mapset());
            return 1;
        }
    }
    else
        strcat(tname, new_name);

    smapset = I_find_signature2(type, old_name, tmapset);
    if (!smapset) {
        G_warning(_("%s is missing"), old_name);
        return 1;
    }

    I_get_signatures_dir(dir, type);
    /* Note – we need whole directory not just an element in it thus
       G_file_name and not G_file_name_misc */
    G_file_name(old_path, dir, sname, tmapset);
    G_file_name(new_path, dir, tname, tmapset);

    if (G_rename_file(old_path, new_path) != 0) {
        G_warning(_("Unable to rename <%s> to <%s>"), old_name, new_name);
        return 1;
    }
    return 0;
}

/*!
 * \brief Get list of existing signatures by type
 *
 * Fills passed list with fully qualified names of existing signatures.
 *
 * If no mapset is passed, all mapsets in the search path are used.
 * If no signatures are found, returns 0 and list is set to NULL.
 *
 * The function will assign memory for the list. It is up to callee to
 * free the memory of each list item and the list itself.
 *
 * \param type I_SIGFILE_TYPE signature type
 * \param mapset optional mapset to search in or NULL
 * \param out_list pointer to array of found signature strings or NULL if none found
 * \return count of signature strings in the array
 */
int I_signatures_list_by_type(I_SIGFILE_TYPE type, const char *mapset,
                              char ***out_list)
{
    int base = 0;

    *out_list = NULL;
    if (mapset == NULL) {
        for (int n = 0; (mapset = G_get_mapset_name(n)); n++) {
            base += list_by_type(type, mapset, base, out_list);
        }
    }
    else {
        base += list_by_type(type, mapset, base, out_list);
    }

    return base;
}

/*!
 * \brief Free memory allocated by I_signatures_list_by_type
 *
 * Calls G_free for all list items returned by I_signatures_list_by_type()
 * Sets pointer to NULL to prevent accidental use after free.
 *
 * \param int Return value of I_signatures_list_by_type()
 * \param pointer to array filled by I_signatures_list_by_type()
 */
void I_free_signatures_list(int count, char ***list)
{
    for (int n = 0; n < count; n++) {
        G_free((*list)[n]);
    }
    G_free(*list);
    *list = NULL;
}

static int list_by_type(I_SIGFILE_TYPE type, const char *mapset, int base,
                        char ***out_list)
{
    int count = 0;
    char path[GPATH_MAX];
    char dir[GNAME_MAX];
    char **dirlist;

    I_get_signatures_dir(dir, type);
    G_file_name(path, dir, "", mapset);

    if (access(path, 0) != 0) {
        return count;
    }

    dirlist = G_ls2(path, &count);
    if (count == 0) {
        G_free(dirlist);
        return count;
    }

    /* Make items fully qualified names */
    int mapset_len = strlen(mapset);

    *out_list = (char **)G_realloc(*out_list, (base + count) * sizeof(char *));
    for (int i = 0; i < count; i++) {
        size_t len = (strlen(dirlist[i]) + 1 + mapset_len + 1) * sizeof(char);
        (*out_list)[base + i] = (char *)G_malloc(len);
        snprintf((*out_list)[base + i], len, "%s@%s", dirlist[i], mapset);
        G_free(dirlist[i]);
    }
    G_free(dirlist);

    return count;
}
