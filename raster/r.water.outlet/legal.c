#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>

char *do_exist(char *file_name)
{
    char *buf, *file_mapset;

    file_mapset = G_find_cell2(file_name, "");
    if (file_mapset == NULL) {
	G_asprintf(&buf, _("Raster map <%s> not found"), file_name);
	G_fatal_error(buf);
    }
    return (file_mapset);
}
