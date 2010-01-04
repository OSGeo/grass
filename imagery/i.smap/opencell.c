#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>


int open_cell_old(const char *name, const char *mapset)
{
    return Rast_open_old(name, mapset);
}

int open_cell_new(const char *name)
{
    return Rast_open_c_new(name);
}
