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

static char **mapset_name;
static char **mapset_name2;
static int nmapset = 0;
static int nmapset2 = 0;
static int new_mapset(const char *);
static int get_list_of_mapsets(void);

/*!
   \brief Get name of the n'th mapset from the mapset_name[] list.

   The first call will initialize the list.

   \param n mapset index

   \return mapset name
   \return NULL if mapset not found
 */
char *G__mapset_name(int n)
{
    /*
     * first call will detect no mapsets in list
     * and go look up the list
     */
    if (nmapset == 0)
	get_list_of_mapsets();
    /*
     * must not run off the bounds of the list
     */
    if (n < 0 || n >= nmapset)
	return ((char *)NULL);

    return mapset_name[n];
}

static int get_list_of_mapsets(void)
{
    char name[GNAME_MAX];
    FILE *fd;

    /*
     * the list of mapsets is in SEARCH_PATH file in the mapset
     */
    mapset_name = NULL;
    if ((fd = G_fopen_old("", "SEARCH_PATH", G_mapset()))) {
	while (fscanf(fd, "%s", name) == 1)
	    if (G__mapset_permissions(name) >= 0)
		new_mapset(name);
	fclose(fd);
    }
    /*
     * if no list, then set the list to the current mapset followed
     * by PERMANENT
     */
    if (!nmapset) {
	char *perm;
	char *cur;

	cur = G_mapset();
	perm = "PERMANENT";

	new_mapset(cur);
	if (strcmp(perm, cur) != 0 && G__mapset_permissions(perm) >= 0)
	    new_mapset(perm);
    }

    return 0;
}

static int new_mapset(const char *name)
{
    /*
     * extend mapset name list and store name
     * note: assumes G_realloc will become G_malloc if mapset_name == NULL
     */
    nmapset++;
    mapset_name =
	(char **)G_realloc((char *)mapset_name, nmapset * sizeof(char *));
    mapset_name[nmapset - 1] = G_store(name);

    return 0;
}

/*!
   \brief Define alternative mapset search path

   \return 0
 */
int G__create_alt_search_path(void)
{
    nmapset2 = nmapset;
    mapset_name2 = mapset_name;

    nmapset = 0;

    return 0;
}

/*!
   \brief Switch mapset search path

   \return 0
 */
int G__switch_search_path(void)
{
    int n;
    char **names;

    n = nmapset2;
    names = mapset_name2;

    nmapset2 = nmapset;
    mapset_name2 = mapset_name;

    nmapset = n;
    mapset_name = names;

    return 0;
}

/*!
   \brief Reset number of mapsets

   \return 0
 */
int G_reset_mapsets(void)
{
    nmapset = 0;

    return 0;
}

/*!
   \brief Get list of available mapsets for current location

   List is updated by each call to this function

   \return pointer to zero terminated array of available mapsets.
 */
char **G_available_mapsets(void)
{
    int i, n;
    static int alloc = 0;
    static char **mapsets = NULL;
    DIR *dir;
    struct dirent *ent;
    char buf[1024];
    struct stat st;

    G_debug(3, "G_available_mapsets");

    if (alloc == 0) {		/* alloc some space, so that if something failes we can return array */
	alloc = 50;
	mapsets = (char **)G_calloc(alloc, sizeof(char *));
    }
    else {			/* free old strings and reset pointers to NULL */
	i = 0;
	while (mapsets[i]) {
	    G_free(mapsets[i]);
	    mapsets[i] = NULL;
	}
    }

    n = 0;
    dir = opendir(G_location_path());
    if (dir == NULL)
	return mapsets;

    while ((ent = readdir(dir))) {
	sprintf(buf, "%s/%s/WIND", G_location_path(), ent->d_name);
	if (stat(buf, &st) == 0) {
	    G_debug(4, "%s is mapset", ent->d_name);
	    /* Realloc space if necessary */
	    if (n + 2 >= alloc) {
		alloc += 50;
		mapsets = (char **)G_realloc(mapsets, alloc * sizeof(char *));
		for (i = n; i < alloc; i++)
		    mapsets[i] = NULL;
	    }
	    /* Add to list */
	    mapsets[n] = G_store(ent->d_name);
	    n++;
	}
	else {
	    G_debug(4, "%s is not mapset", ent->d_name);
	}
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

    for (i = 0; i < nmapset; i++) {
	if (strcmp(mapset_name[i], mapset) == 0)
	    return;
    }
    new_mapset(mapset);
}
