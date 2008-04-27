#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>


CELL *
open_cell (char *name, char *mapset, int *fd)
{
    if (mapset == NULL) mapset = G_find_cell (name, "");
    *fd = G_open_cell_old (name, mapset);
    if (*fd < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), name);

    return G_allocate_cell_buf();
}
