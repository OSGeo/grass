#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>


int opennew(const char *name, RASTER_MAP_TYPE wr_type)
{
    if (wr_type < 0)		/* default fp type */
	return Rast_open_fp_new(name);
    else
	return Rast_open_new(name, wr_type);
}
