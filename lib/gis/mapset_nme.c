/*!
   \file mapset_nme.c

   \brief GIS library - Mapset name, search path routines.

   (C) 1999-2008 The GRASS development team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <grass/gis.h>

static struct state {
    struct list {
	char **names;
	int count;
	int size;
    } path, path2;
    int initialized;
} state;

static struct state *st = &state;

static void new_mapset(const char *);

/*!
   \brief Get name of the n'th mapset from the mapset_name[] list.

   The first call will initialize the list.

   \param n mapset index

   \return mapset name
   \return NULL if mapset not found
 */
const char *G__mapset_name(int n)
{
    G_get_list_of_mapsets();

    if (n < 0 || n >= st->path.count)
	return NULL;

    return st->path.names[n];
}

void G_get_list_of_mapsets(void)
{
    FILE *fp;

    if (G_is_initialized(&st->initialized))
	return;

    fp = G_fopen_old("", "SEARCH_PATH", G_mapset());
    if (fp) {
	char name[GNAME_MAX];
	while (fscanf(fp, "%s", name) == 1)
	    if (G__mapset_permissions(name) >= 0)
		new_mapset(name);
	fclose(fp);
    }

    if (!st->path.count) {
	static const char perm[] = "PERMANENT";
	const char *cur = G_mapset();

	new_mapset(cur);
	if (strcmp(perm, cur) != 0 && G__mapset_permissions(perm) >= 0)
	    new_mapset(perm);
    }

    G_initialize_done(&st->initialized);
}

static void new_mapset(const char *name)
{
    if (st->path.count >= st->path.size) {
	st->path.size += 10;
	st->path.names = G_realloc(st->path.names, st->path.size * sizeof(char *));
    }

    st->path.names[st->path.count++] = G_store(name);
}

/*!
   \brief Define alternative mapset search path

   \return 0
 */
void G__create_alt_search_path(void)
{
    st->path2.count = st->path.count;
    st->path2.names = st->path.names;

    st->path.count = 0;
}

/*!
   \brief Switch mapset search path

   \return 0
 */
void G__switch_search_path(void)
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
   \brief Reset number of mapsets

   \return 0
 */
void G_reset_mapsets(void)
{
    st->path.count = 0;
}

/*!
   \brief Get list of available mapsets for current location

   List is updated by each call to this function

   \return pointer to zero terminated array of available mapsets.
 */
char **G_available_mapsets(void)
{
    char **mapsets = NULL;
    int alloc = 0;
    int n = 0;
    DIR *dir;
    struct dirent *ent;

    G_debug(3, "G_available_mapsets");

    dir = opendir(G_location_path());
    if (!dir)
	return mapsets;

    while ((ent = readdir(dir))) {
	char buf[GPATH_MAX];
	struct stat st;

	sprintf(buf, "%s/%s/WIND", G_location_path(), ent->d_name);

	if (G_stat(buf, &st) != 0) {
	    G_debug(4, "%s is not mapset", ent->d_name);
	    continue;
	}

	G_debug(4, "%s is mapset", ent->d_name);

	if (n + 2 >= alloc) {
	    alloc += 50;
	    mapsets = G_realloc(mapsets, alloc * sizeof(char *));
	}

	mapsets[n++] = G_store(ent->d_name);
    }

    closedir(dir);

    return mapsets;
}

/*!
   \brief Add mapset to the list of mapsets in search path.

   Mapset is add in memory only, not to the SEARCH_PATH file!
   List is check first if already exists.

   \param mapset mapset name
 */
void G_add_mapset_to_search_path(const char *mapset)
{
    int i;

    for (i = 0; i < st->path.count; i++)
	if (strcmp(st->path.names[i], mapset) == 0)
	    return;

    new_mapset(mapset);
}
