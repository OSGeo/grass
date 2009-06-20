#include <stdlib.h>
#include <grass/gis.h>
#include <grass/Rast.h>
#include <grass/glocale.h>


int opennew(const char *name, RASTER_MAP_TYPE wr_type)
{
    int fd;

    if (wr_type < 0)		/* default fp type */
	fd = Rast_open_fp_cell_new(name);
    else
	fd = Rast_open_raster_new(name, wr_type);

    if (fd < 0)
	G_fatal_error(_("Unable to create raster map <%s>"), name);

    return fd;
}
