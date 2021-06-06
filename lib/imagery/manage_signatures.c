/*!
   \file lib/imagery/manage_sinatures.c

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

static int list_by_type(int, const char *, int, char ***);

/*!
 * \brief Remove a signature file
 * 
 * If removal fails, prints a warning and returns 1.
 * It is safe to pass fully qualified names.
 * 
 * \param type SIGFILE_TYPE_ signature type
 * \param name of signature to remove
 * \return 0 on success
 * \return 1 on failure
 */
int I_signatures_remove(int type, const char *name)
{
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    char selem[GNAME_MAX];

    G_debug(1, "I_signatures_remove(%d, %s);", type, name);

    /* Remove only if file is in the current mapset */
    if (G_name_is_fully_qualified(name, xname, xmapset) &&
        strcmp(xmapset, G_mapset()) != 0) {
        G_warning(_("%s is not in the current mapset (%s)"), name,
                  G_mapset());
        return 1;
    }
    if (I_find_signature2(type, name, G_mapset())) {
        if (type == SIGFILE_TYPE_SIG)
            sprintf(selem, "signatures%csig", HOST_DIRSEP);
        else if (type == SIGFILE_TYPE_SIGSET) {
            sprintf(selem, "signatures%csigset", HOST_DIRSEP);
        }
        if (G_remove(selem, name) == 1) {
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
 * \param type SIGFILE_TYPE_ signature type
 * \param name of old signature
 * \param mapset of old signature
 * \param name of new signature
 * \return 0 on success
 * \return 1 on failure
 */
int I_signatures_copy(int type, const char *old_name, const char *old_mapset,
                      const char *new_name)
{
    char tname[GNAME_MAX], tmapset[GMAPSET_MAX];
    char selem[GNAME_MAX];
    const char *smapset;
    char old_path[GPATH_MAX], new_path[GPATH_MAX];

    G_debug(1, "I_signatures_copy(%d, %s@%s, %s);", type, old_name,
            old_mapset, new_name);

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

    if (type == SIGFILE_TYPE_SIG)
        sprintf(selem, "signatures%csig", HOST_DIRSEP);
    else if (type == SIGFILE_TYPE_SIGSET) {
        sprintf(selem, "signatures%csigset", HOST_DIRSEP);
    }

    G_make_mapset_element(selem);
    G_file_name(old_path, selem, old_name, smapset);
    G_file_name(new_path, selem, tname, G_mapset());
    if (G_copy_file(old_path, new_path) != 1) {
        G_warning(_("Unable to copy <%s> to current mapset as <%s>"),
                  G_fully_qualified_name(old_name, smapset), tname);
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
 * \param type SIGFILE_TYPE_ signature type
 * \param name of old signature
 * \param name of new signature
 * \return 0 on success
 * \return 1 on failure
 */
int I_signatures_rename(int type, const char *old_name, const char *new_name)
{
    char sname[GNAME_MAX], tname[GNAME_MAX], tmapset[GMAPSET_MAX];
    const char *smapset;
    char selem[GNAME_MAX];
    char old_path[GPATH_MAX], new_path[GPATH_MAX];

    G_debug(1, "I_signatures_rename(%d, %s, %s);", type, old_name, new_name);

    /* Rename only if source and destinaiton mapset is the current mapset */
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

    if (type == SIGFILE_TYPE_SIG)
        sprintf(selem, "signatures%csig", HOST_DIRSEP);
    else if (type == SIGFILE_TYPE_SIGSET) {
        sprintf(selem, "signatures%csigset", HOST_DIRSEP);
    }

    G_file_name(old_path, selem, sname, tmapset);
    G_file_name(new_path, selem, tname, tmapset);
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
 * \param type SIGFILE_TYPE_ signature type
 * \param mapset optional mapset to search in or NULL
 * \param pointer to array of found signature strings or NULL if none found
 * \return count of signature strings in the array
 */
int I_signatures_list_by_type(int type, const char *mapset, char ***out_list)
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

static int list_by_type(int type, const char *mapset, int base,
                        char ***out_list)
{
    int count = 0;
    char path[GPATH_MAX];
    char element[GNAME_MAX];
    char **dirlist;

    if (type == SIGFILE_TYPE_SIG)
        sprintf(element, "signatures%csig", HOST_DIRSEP);
    else if (type == SIGFILE_TYPE_SIGSET)
        sprintf(element, "signatures%csigset", HOST_DIRSEP);

    G_file_name(path, element, "", mapset);

    if (access(path, 0) != 0) {
        return count;
    }

    dirlist = G_ls2(path, &count);
    if (count == 0)
        return count;

    /* Make items fully qualified names */
    int mapset_len = strlen(mapset);

    *out_list =
        (char **)G_realloc(*out_list, (base + count) * sizeof(char *));
    for (int i = 0; i < count; i++) {
        (*out_list)[base + i] =
            (char *)G_malloc((strlen(dirlist[i]) + 1 + mapset_len + 1) *
                             sizeof(char));
        sprintf((*out_list)[base + i], "%s@%s", dirlist[i], mapset);
    }

    return count;
}
