#include <grass/gis.h>
#include <grass/raster.h>
#include "ncb.h"

int null_cats(void)
{
    Rast_init_cats(ncb.title, &ncb.cats);

    return 0;
}
