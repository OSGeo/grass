#include <grass/gis.h>
#include <grass/raster.h>
#include "ncb.h"

int intr_cats(void)
{
    Rast_set_cats_fmt("$1% dispersion", 1.0, -1.0, 0.0, 0.0, &ncb.cats);
    
    return 0;
}
