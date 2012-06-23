#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

int opencell(char *fullname, char *name, char *mapset)
{
    char *m;
    int fd;

    strcpy(name, fullname);
    m = G_find_raster2(name, "");
    if (m == NULL) {
	G_warning(_("Raster map <%s> not found"), name);
	return -1;
    }

    if (strlen(m) == 0)
	strcpy(mapset, G_mapset());
    else
	strcpy(mapset, m);

    fd = Rast_open_old(name, mapset);

    return fd;
}
