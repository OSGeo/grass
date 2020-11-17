/*!
   \file lib/gis/file_name.c

   \brief GIS library - Determine GRASS data base file name

   (C) 2001-2015 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Original author CERL
 */

#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>

#include "gis_local_proto.h"

static char *file_name(char *, const char *, const char *,
                       const char *, const char *, const char *);
static void append_char(char*, char);
char *strsep_(char **, const char *);

/*!
  \brief Builds full path names to GIS data files

  If <i>name</i> is of the form "nnn@ppp" then path is set as if name
  had been "nnn" and mapset had been "ppp" (mapset parameter itself is
  ignored in this case).
  
  \param[out] path buffer to hold resultant full path to file
  \param element database element (eg, "cell", "cellhd", "vector", etc)
  \param name name of file to build path to (fully qualified names allowed)
  \param mapset mapset name

  \return pointer to <i>path</i> buffer
*/
char *G_file_name(char *path,
		   const char *element, const char *name, const char *mapset)
{
    return file_name(path, NULL, element, name, mapset, NULL);
}

/*!
  \brief Builds full path names to GIS misc data files

  \param[out] path buffer to hold resultant full path to file
  \param dir misc directory
  \param element database element (eg, "cell", "cellhd", "vector", etc)
  \param name name of file to build path to (fully qualified names allowed)
  \param mapset mapset name

  \return pointer to <i>path</i> buffer
*/
char *G_file_name_misc(char *path,
			const char *dir,
			const char *element,
			const char *name, const char *mapset)
{
    return file_name(path, dir, element, name, mapset, NULL);
}

/*!
  \brief Builds full path names to GIS data files in temporary directory (for internal use only)

  By default temporary directory is located
  $LOCATION/$MAPSET/.tmp/$HOSTNAME. If GRASS_VECTOR_TMPDIR_MAPSET is
  set to "0", the temporary directory is located in TMPDIR
  (environmental variable defined by the user or GRASS initialization
  script if not given). Note that GRASS_VECTOR_TMPDIR_MAPSET variable
  is currently used only by vector library.

  \param[out] path buffer to hold resultant full path to file
  \param element database element (eg, "cell", "cellhd", "vector", etc)
  \param name name of file to build path to (fully qualified names allowed)
  \param mapset mapset name

  \return pointer to <i>path</i> buffer
*/
char *G_file_name_tmp(char *path,
                      const char *element,
                      const char *name, const char *mapset,
                      const char *type)
{
    const char *env, *tmp_path;
    const char path_sep =
    #ifdef _WIN32
        '\\';
    #else
        '/';
    #endif

    tmp_path = NULL;
    if (type && strcmp(type, "vector") == 0) {
      env = getenv("GRASS_VECTOR_TMPDIR_MAPSET");
      if (env && strcmp(env, "0") == 0)
        tmp_path = getenv("TMPDIR");
      else if (env)
        tmp_path = getenv("GRASS_VECTOR_TMPDIR_MAPSET");
    }
    else if (element) {
        int c = 1;
        char *found;
        char *e;
        e = strdup(element);
        while ((found = strsep_(&e, &path_sep))) {
            if (c == 3 && strcmp(found, "vector") == 0) {
                env = getenv("GRASS_VECTOR_TMPDIR_MAPSET");
                if (env && strcmp(env, "0") == 0)
                    tmp_path = getenv("TMPDIR");
                else if (env)
                    tmp_path = getenv("GRASS_VECTOR_TMPDIR_MAPSET");
               break;
            }
            c++;
        }
        G_free(e);
    }
    if (!type && !tmp_path) {
        env = getenv("GRASS_RASTER_TMPDIR_MAPSET");
        if (env && strcmp(env, "0") == 0)
            tmp_path = getenv("TMPDIR");
        else if (env)
            tmp_path = getenv("GRASS_RASTER_TMPDIR_MAPSET");
    }

    G_debug(2, "G_file_name_tmp(): path = %s, tmp_path = %s", path, tmp_path);

    return file_name(path, NULL, element, name, mapset, tmp_path);
}

char *file_name(char *path,
                const char *dir, const char *element, const char *name,
                const char *mapset, const char *base)
{
    const char *pname = name;
    
    if (base && *base) {
        sprintf(path, "%s", base);
    }
    else {
        char xname[GNAME_MAX];
        char xmapset[GMAPSET_MAX];
        char *location = G__location_path();
        
        /*
         * if a name is given, build a file name
         * must split the name into name, mapset if it is
         * in the name@mapset format
         */
        if (name && *name && G_name_is_fully_qualified(name, xname, xmapset)) {
            pname = xname;
            sprintf(path, "%s%c%s", location, HOST_DIRSEP, xmapset);
        }
        else if (mapset && *mapset)
            sprintf(path, "%s%c%s", location, HOST_DIRSEP, mapset);
        else
            sprintf(path, "%s%c%s", location, HOST_DIRSEP, G_mapset());
        G_free(location);
    }

    if (dir && *dir) { /* misc element */
	append_char(path, HOST_DIRSEP);
	strcat(path, dir);

        if (pname && *pname) {
            append_char(path, HOST_DIRSEP);
            strcat(path, pname);
        }

        if (element && *element) {
            append_char(path, HOST_DIRSEP);
            strcat(path, element);
        }
    }
    else {
        if (element && *element) {
            append_char(path, HOST_DIRSEP);
            strcat(path, element);
        }
        
        if (pname && *pname) {
            append_char(path, HOST_DIRSEP);
            strcat(path, pname);
        }
    }

    G_debug(2, "G_file_name(): path = %s", path);
    
    return path;
}

void append_char(char* s, char c)
{
        int len = strlen(s);
        s[len] = c;
        s[len+1] = '\0';
}

char *strsep_(char **stringp, const char *delim)
{
  char *start = *stringp;
  char *p;

  p = (start != NULL) ? strpbrk(start, delim) : NULL;

  if (p == NULL)
       *stringp = NULL;
  else
  {
      *p = "\0";
      *stringp = p + 1;
  }

  return start;
}