#include <grass/gis.h>
#include <grass/raster.h>
#include "ncb.h"

int null_cats(const char *title)
{
    Rast_init_cats(title, &ncb.cats);

    return 0;
}
