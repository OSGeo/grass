/*!
   \file lib/gis/subproject_nme.c

   \brief GIS library - Subproject name, search path routines.

   (C) 1999-2014 The GRASS development team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.
 */

#include <grass/config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <grass/gis.h>

#include "gis_local_proto.h"

static struct state {
    struct list {
	char **names;
	int count;
	int size;
    } path, path2;
} state;

static struct state *st = &state;

static void new_subproject(const char *);

/*!
   \brief Get name of the n'th subproject from the current subproject search path.

   The first call will initialize the list.

   \param n subproject index

   \return subproject name
   \return NULL if subproject not found
 */
const char *G_get_subproject_name(int n)
{
    G__get_list_of_subprojects();

    if (n < 0 || n >= st->path.count)
	return NULL;

    return st->path.names[n];
}

/*!
  \brief Fill list of subprojects from search path (internal use only)
*/
void G__get_list_of_subprojects(void)
{
    FILE *fp;
    const char *cur;

    if (st->path.count > 0)
	return;

    st->path.count = 0;
    st->path.size = 0;
    st->path.names = NULL;

    cur = G_subproject();
    new_subproject(cur);

    fp = G_fopen_old("", "SEARCH_PATH", G_subproject());
    if (fp) {
	char name[GNAME_MAX];
	while (fscanf(fp, "%s", name) == 1) {
	    if (strcmp(name, cur) == 0)
		continue;
	    if (G_subproject_permissions(name) >= 0)
		new_subproject(name);
	}
	fclose(fp);
    }
    else {
	static const char perm[] = "PERMANENT";
	if (strcmp(perm, cur) != 0 && G_subproject_permissions(perm) >= 0)
	    new_subproject(perm);
    }
}

void new_subproject(const char *name)
{
    if (st->path.count >= st->path.size) {
	st->path.size += 10;
	st->path.names = G_realloc(st->path.names, st->path.size * sizeof(char *));
    }

    st->path.names[st->path.count++] = G_store(name);
}

/*!
   \brief Define alternative subproject search path
 */
void G_create_alt_search_path(void)
{
    st->path2.count = st->path.count;
    st->path2.names = st->path.names;

    st->path.count = 0;
}

/*!
   \brief Switch subproject search path
 */
void G_switch_search_path(void)
{
    int count;
    char **names;

    count = st->path2.count;
    names = st->path2.names;

    st->path2.count = st->path.count;
    st->path2.names = st->path.names;

    st->path.count = count;
    st->path.names = names;
}

/*!
   \brief Reset number of subprojects
 */
void G_reset_subprojects(void)
{
    st->path.count = 0;
}

/*!
   \brief Get list of available subprojects for current project

   List is updated by each call to this function.

   \return pointer to NULL terminated array of available subprojects
 */
char **G_get_available_subprojects(void)
{
    char *project;
    char **subprojects = NULL;
    int alloc = 50;
    int n = 0;
    DIR *dir;
    struct dirent *ent;

    G_debug(3, "G_get_available_subprojects");

    subprojects = G_calloc(alloc, sizeof(char *));

    project = G_project_path();
    dir = opendir(project);
    if (!dir) {
        G_free(project);
        return subprojects;
    }

    while ((ent = readdir(dir))) {
	char buf[GPATH_MAX];
	struct stat st;

	sprintf(buf, "%s/%s/WIND", project, ent->d_name);

	if (G_stat(buf, &st) != 0) {
	    G_debug(4, "%s is not subproject", ent->d_name);
	    continue;
	}

	G_debug(4, "%s is subproject", ent->d_name);

	if (n + 2 >= alloc) {
	    alloc += 50;
	    subprojects = G_realloc(subprojects, alloc * sizeof(char *));
	}

	subprojects[n++] = G_store(ent->d_name);
    }
	subprojects[n] = NULL;

    closedir(dir);
    G_free(project);

    return subprojects;
}

/*!
   \brief Add subproject to the list of subprojects in search path

   Subproject is add in memory only, not to the SEARCH_PATH file!
   List is check first if already exists.

   \param subproject subproject name to be added to the search path
 */
void G_add_subproject_to_search_path(const char *subproject)
{
    if (!G_is_subproject_in_search_path(subproject))
	new_subproject(subproject);
}

/*!
  \brief Check if given subproject is in search path

  \param subproject subproject name

  \return 1 subproject found in search path
  \return 0 subproject not found
*/
int G_is_subproject_in_search_path(const char *subproject)
{
    int i;
    
    for (i = 0; i < st->path.count; i++) {
	if (strcmp(st->path.names[i], subproject) == 0)
	    return 1;
    }

    return 0;
}
