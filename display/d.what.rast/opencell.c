#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

int opencell(char *fullname, char *name, char *mapset)
{
    char *m;
    int fd;

    strcpy(name, fullname);
    m = G_find_cell2(name, "");
    if (m == NULL) {
	G_warning(_("Raster map <%s> not found"), name);
	return -1;
    }

    if (strlen(m) == 0)
	strcpy(mapset, G_mapset());
    else
	strcpy(mapset, m);

    fd = G_open_cell_old(name, mapset);
    if (fd < 0)
	G_warning(_("Unable to open raster map <%s>"), name);

    return fd;
}
