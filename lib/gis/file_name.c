/**********************************************************************
 *  char *
 *  G__file_name (path, element, name, maps)
 *      char path[]          buffer to hold resultant full path to file.
 *      const char *element  database element (eg, "cell", "cellhd", etc)
 *      const char *name     name of file to build path to
 *      const char *mapset   mapset name
 *   
 *      builds full path names to GIS data files
 *
 *  returns:
 *         pointer to 'path'
 *
 *   note:
 *      if name is of the form nnn@ppp then path is set
 *      as if name had been nnn and mapset had been ppp
 *      (mapset parameter itself is ignored in this case)
 *********************************************************************/

#include <string.h>
#include <grass/gis.h>

char *G__file_name ( 
	char *path,
	const char *element,
	const char *name,
	const char *mapset)
{
	char xname[GNAME_MAX];
	char xmapset[GMAPSET_MAX];
	const char *pname = name;
	char *location = G__location_path();

/*
 * if a name is given, build a file name
 * must split the name into name, mapset if it is
 * in the name@mapset format
 */
	if (name && *name && G__name_is_fully_qualified(name, xname, xmapset))
	{
		pname = xname;
		sprintf(path,"%s/%s", location, xmapset);
	}
	else if (mapset && *mapset)
		sprintf(path,"%s/%s", location, mapset);
	else
		sprintf(path,"%s/%s", location, G_mapset());

	G_free (location);
	
	if (element && *element)
	{
		strcat (path, "/");
		strcat (path, element);
	}

	if (pname && *pname)
	{
		strcat (path, "/");
		strcat (path, pname);
	}

	return path;
}

char *G__file_name_misc ( 
	char *path,
	const char *dir,
	const char *element,
	const char *name,
	const char *mapset)
{
	char xname[GNAME_MAX];
	char xmapset[GMAPSET_MAX];
	const char *pname = name;
	char *location = G__location_path();

/*
 * if a name is given, build a file name
 * must split the name into name, mapset if it is
 * in the name@mapset format
 */
	if (name && *name && G__name_is_fully_qualified(name, xname, xmapset))
	{
		pname = xname;
		sprintf(path,"%s/%s", location, xmapset);
	}
	else if (mapset && *mapset)
		sprintf(path,"%s/%s", location, mapset);
	else
		sprintf(path,"%s/%s", location, G_mapset());

	G_free (location);
	
	if (dir && *dir)
	{
		strcat (path, "/");
		strcat (path, dir);
	}

	if (pname && *pname)
	{
		strcat (path, "/");
		strcat (path, pname);
	}

	if (element && *element)
	{
		strcat (path, "/");
		strcat (path, element);
	}

	return path;
}
