#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <grass/gis.h>

static char *G__find_etc(const char *name)
{
    char path[GPATH_MAX];
    const char *pathlist = getenv("GRASS_ADDON_ETC");

    /*
     * reject illegal names
     */
    if (*name == 0 || *name == '.')
	return NULL;

    /*
     * search paths
     */
    if (pathlist) {
	char **dirs = G_tokenize(pathlist, ":");
	char *result = NULL;
	int i;

	for (i = 0; dirs[i]; i++) {
	    sprintf(path, "%s/%s", dirs[i], name);

	    if (access(path, 0) == 0) {
		result = G_store(path);
		break;
	    }
	}

	G_free_tokens(dirs);

	if (result)
	    return result;
    }

    /*
     * check application etc dir
     */
    sprintf(path, "%s/etc/%s", G_gisbase(), name);
    if (access(path, 0) == 0)
	return G_store(path);

    return NULL;
}


/*!
 * \brief searches for a file from the etc search list in GRASS_ADDON_ETC
 *      returns the full path to where the file was found.
 *
 *  note:
 *      rejects all names that begin with "."
 *
 *  \param name file name to look for
 *
 *  \return pointer to a string with full path to
 *              where file was found, or NULL if not found
 */
char *G_find_etc(const char *name)
{
    return G__find_etc(name);
}
