
/**************************************************************
 * char *G_get_cell_title (name, mapset)
 *   char *name        name of map file
 *   char *mapset      mapset containing name
 *
 *   returns pointer to string containing cell title. (from cats file)
 *************************************************************/

#include <stdio.h>
#include <grass/gis.h>

char *G_get_dig_title(const char *name, const char *mapset)
{
    FILE *fd;
    int stat = -1;
    char title[100];

    fd = G_fopen_old("dig_cats", name, mapset);
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
