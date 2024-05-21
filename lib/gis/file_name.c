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

static char *file_name(char *, const char *, const char *, const char *,
                       const char *, const char *);
static void append_char(char *, char);

/*!
   \brief Builds full path names to GIS data files

<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
   If <i>name</i> is of the form "nnn@ppp" then path is set as if name
   had been "nnn" and mapset had been "ppp" (mapset parameter itself is
   ignored in this case).
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
   If <i>name</i> is of the form "nnn@ppp" then path is set as if name
   had been "nnn" and mapset had been "ppp" (mapset parameter itself is
   ignored in this case).
=======
=======
>>>>>>> osgeo-main
  If <i>name</i> is of the form "nnn@ppp" then path is set as if name
  had been "nnn" and mapset had been "ppp" (mapset parameter itself is
  ignored in this case).
=======
   If <i>name</i> is of the form "nnn@ppp" then path is set as if name
   had been "nnn" and mapset had been "ppp" (mapset parameter itself is
   ignored in this case).
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

   Paths to files are currently in form:
   /path/to/location/mapset/element/name

   path input buffer memory must be allocated by caller.

   C:
   @code{.c}
   char path[GPATH_MAX];
   G_file_name(path, "fcell", "my_raster", "my_mapset");
   // path now is "/full/path/to/my_mapset/fcell/my_raster"
   @endcode
   Python:
   @code{.py}
   import ctypes
   from grass.pygrass.utils import decode
   from grass.lib.gis import G_file_name, GPATH_MAX

   path = ctypes.create_string_buffer(GPATH_MAX)
   path_str = decode(G_file_name(path, "elem", "name", "mapset"))
   print(path_str)
   >>> /full/path/to/mapset/elem/name
   @endcode

<<<<<<< HEAD
  \param[out] path allocated buffer to hold resultant full path to file
  \param element database element (eg, "cell", "cellhd", "vector", etc)
  \param name name of file to build path to (fully qualified names allowed)
  \param mapset mapset name
>>>>>>> 6dd43833fd (Improve G_open|find _misc function documentation (#1760))
=======
   If <i>name</i> is of the form "nnn@ppp" then path is set as if name
   had been "nnn" and mapset had been "ppp" (mapset parameter itself is
   ignored in this case).
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

   Paths to files are currently in form:
   /path/to/location/mapset/element/name

   path input buffer memory must be allocated by caller.

   C:
   @code{.c}
   char path[GPATH_MAX];
   G_file_name(path, "fcell", "my_raster", "my_mapset");
   // path now is "/full/path/to/my_mapset/fcell/my_raster"
   @endcode
   Python:
   @code{.py}
   import ctypes
   from grass.pygrass.utils import decode
   from grass.lib.gis import G_file_name, GPATH_MAX

   path = ctypes.create_string_buffer(GPATH_MAX)
   path_str = decode(G_file_name(path, "elem", "name", "mapset"))
   print(path_str)
   >>> /full/path/to/mapset/elem/name
   @endcode

   \param[out] path allocated buffer to hold resultant full path to file
   \param element database element (eg, "cell", "cellhd", "vector", etc)
   \param name name of file to build path to (fully qualified names allowed)
   \param mapset mapset name

<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
   \param[out] path allocated buffer to hold resultant full path to file
   \param element database element (eg, "cell", "cellhd", "vector", etc)
   \param name name of file to build path to (fully qualified names allowed)
   \param mapset mapset name

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
   \return pointer to <i>path</i> buffer
 */
char *G_file_name(char *path, const char *element, const char *name,
                  const char *mapset)
{
    return file_name(path, NULL, element, name, mapset, NULL);
}

/*!
   \brief Builds full path names to GIS misc data files

<<<<<<< HEAD
<<<<<<< HEAD
   Paths to misc files are currently in form:
   /path/to/location/mapset/dir/name/element
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
   Paths to misc files are currently in form:
   /path/to/location/mapset/dir/name/element
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
   Paths to misc files are currently in form:
   /path/to/location/mapset/dir/name/element
=======
>>>>>>> osgeo-main
  Paths to misc files are currently in form:
  /path/to/location/mapset/dir/name/element
=======
   Paths to misc files are currently in form:
   /path/to/location/mapset/dir/name/element
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

   path input buffer memory must be allocated by caller.

   C:
   @code{.c}
   char path[GPATH_MAX];
   G_file_name_misc(path, "cell_misc", "history", "my_raster", "my_mapset");
   // path now contains "/full/path/to/my_mapset/cell_misc/my_raster/history"
   @endcode
   Python:
   @code{.py}
   import ctypes
   from grass.pygrass.utils import decode
   from grass.lib.gis import G_file_name_misc, GPATH_MAX

   path = ctypes.create_string_buffer(GPATH_MAX)
   path_str = decode(G_file_name_misc(path, "dir", "elem", "name", "mapset"))
   print(path_str)
   >>> /full/path/to/mapset/dir/name/elem
   @endcode

<<<<<<< HEAD
  \param[out] path allocated buffer to hold resultant full path to file
  \param dir misc directory (e.g., "cell_misc", "group")
  \param element database element (in this case – file to build path to e.g., "history", "REF")
  \param name name of object (raster, group; fully qualified names allowed e.g., "my_raster@PERMANENT")
  \param mapset mapset name
>>>>>>> 6dd43833fd (Improve G_open|find _misc function documentation (#1760))
=======
   Paths to misc files are currently in form:
   /path/to/location/mapset/dir/name/element
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main

   path input buffer memory must be allocated by caller.

   C:
   @code{.c}
   char path[GPATH_MAX];
   G_file_name_misc(path, "cell_misc", "history", "my_raster", "my_mapset");
   // path now contains "/full/path/to/my_mapset/cell_misc/my_raster/history"
   @endcode
   Python:
   @code{.py}
   import ctypes
   from grass.pygrass.utils import decode
   from grass.lib.gis import G_file_name_misc, GPATH_MAX

   path = ctypes.create_string_buffer(GPATH_MAX)
   path_str = decode(G_file_name_misc(path, "dir", "elem", "name", "mapset"))
   print(path_str)
   >>> /full/path/to/mapset/dir/name/elem
   @endcode

   \param[out] path allocated buffer to hold resultant full path to file
   \param dir misc directory (e.g., "cell_misc", "group")
   \param element database element (in this case – file to build path to e.g.,
   "history", "REF") \param name name of object (raster, group; fully qualified
   names allowed e.g., "my_raster@PERMANENT") \param mapset mapset name

<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
   \param[out] path allocated buffer to hold resultant full path to file
   \param dir misc directory (e.g., "cell_misc", "group")
   \param element database element (in this case – file to build path to e.g.,
   "history", "REF") \param name name of object (raster, group; fully qualified
   names allowed e.g., "my_raster@PERMANENT") \param mapset mapset name

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
   \return pointer to <i>path</i> buffer
 */
char *G_file_name_misc(char *path, const char *dir, const char *element,
                       const char *name, const char *mapset)
{
    return file_name(path, dir, element, name, mapset, NULL);
}

/*!
   \brief Builds full path names to GIS data files in temporary directory (for
   internal use only)

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
char *G_file_name_tmp(char *path, const char *element, const char *name,
                      const char *mapset)
{
    const char *env, *tmp_path;

    tmp_path = NULL;
    env = getenv("GRASS_VECTOR_TMPDIR_MAPSET");
    if (env && strcmp(env, "0") == 0) {
        tmp_path = getenv("TMPDIR");
    }

    return file_name(path, NULL, element, name, mapset, tmp_path);
}

/*!
   \brief Builds full path names to GIS data files in temporary directory (for
   internal use only)

   By default the GRASS temporary directory is located at
   $LOCATION/$MAPSET/.tmp/$HOSTNAME/. If basedir is provided, the
   temporary directory is located at <basedir>/.tmp/$HOSTNAME/.

   \param[out] path buffer to hold resultant full path to file
   \param element database element (eg, "cell", "cellhd", "vector", etc)
   \param name name of file to build path to (fully qualified names allowed)
   \param mapset mapset name

   \return pointer to <i>path</i> buffer
 */
char *G_file_name_basedir(char *path, const char *element, const char *name,
                          const char *mapset, const char *basedir)
{
    return file_name(path, NULL, element, name, mapset, basedir);
}

char *file_name(char *path, const char *dir, const char *element,
                const char *name, const char *mapset, const char *base)
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

void append_char(char *s, char c)
{
    int len = strlen(s);

    s[len] = c;
    s[len + 1] = '\0';
}
