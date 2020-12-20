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

/*!
  \brief Builds full path names to GIS data files

  If <i>name</i> is of the form "nnn@ppp" then path is set as if name
  had been "nnn" and subproject had been "ppp" (subproject parameter itself is
  ignored in this case).
  
  \param[out] path buffer to hold resultant full path to file
  \param element database element (eg, "cell", "cellhd", "vector", etc)
  \param name name of file to build path to (fully qualified names allowed)
  \param subproject subproject name

  \return pointer to <i>path</i> buffer
*/
char *G_file_name(char *path,
		   const char *element, const char *name, const char *subproject)
{
    return file_name(path, NULL, element, name, subproject, NULL);
}

/*!
  \brief Builds full path names to GIS misc data files

  \param[out] path buffer to hold resultant full path to file
  \param dir misc directory
  \param element database element (eg, "cell", "cellhd", "vector", etc)
  \param name name of file to build path to (fully qualified names allowed)
  \param subproject subproject name

  \return pointer to <i>path</i> buffer
*/
char *G_file_name_misc(char *path,
			const char *dir,
			const char *element,
			const char *name, const char *subproject)
{
    return file_name(path, dir, element, name, subproject, NULL);
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
  \param subproject subproject name

  \return pointer to <i>path</i> buffer
*/
char *G_file_name_tmp(char *path,
                      const char *element,
                      const char *name, const char *subproject)
{
    const char *env, *tmp_path;

    tmp_path = NULL;
    env = getenv("GRASS_VECTOR_TMPDIR_MAPSET");
    if (env && strcmp(env, "0") == 0) {
        tmp_path = getenv("TMPDIR");
    }
    
    return file_name(path, NULL, element, name, subproject, tmp_path);
}

char *file_name(char *path,
                const char *dir, const char *element, const char *name,
                const char *subproject, const char *base)
{
    const char *pname = name;
    
    if (base && *base) {
        sprintf(path, "%s", base);
    }
    else {
        char xname[GNAME_MAX];
        char xsubproject[GMAPSET_MAX];
        char *project = G__project_path();
        
        /*
         * if a name is given, build a file name
         * must split the name into name, subproject if it is
         * in the name@subproject format
         */
        if (name && *name && G_name_is_fully_qualified(name, xname, xsubproject)) {
            pname = xname;
            sprintf(path, "%s%c%s", project, HOST_DIRSEP, xsubproject);
        }
        else if (subproject && *subproject)
            sprintf(path, "%s%c%s", project, HOST_DIRSEP, subproject);
        else
            sprintf(path, "%s%c%s", project, HOST_DIRSEP, G_subproject());
        G_free(project);
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
