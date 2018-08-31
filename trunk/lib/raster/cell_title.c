
/**************************************************************
 * char *Rast_get_cell_title (name, mapset)
 *   char *name        name of map file
 *   char *mapset      mapset containing name
 *
 *   returns pointer to string containing cell title. (from cats file)
 *************************************************************/

#include <grass/gis.h>


/*!
 * \brief get raster map title
 *
 * If only the map layer title is needed, it is not necessary to read the
 * entire category file into memory. This routine gets the title for raster map
 * <b>name</b> in <b>mapset</b> directly from the category file, and returns
 * a pointer to the title. A legal pointer is always returned. If the map layer
 * does not have a title, then a pointer to the empty string "" is returned.
 *
 *  \param name
 *  \param mapset
 *  \return char * 
 */

char *Rast_get_cell_title(const char *name, const char *mapset)
{
    FILE *fd;
    int stat;
    char title[1024];

    stat = -1;
    fd = G_fopen_old("cats", name, mapset);
    if (fd) {
	stat = 1;
	if (!fgets(title, sizeof title, fd))	/* skip number of cats */
	    stat = -1;
	else if (!G_getl(title, sizeof title, fd))	/* read title */
	    stat = -1;

	fclose(fd);
    }

    if (stat < 0)
	*title = 0;
    else
	G_strip(title);
    return G_store(title);
}
