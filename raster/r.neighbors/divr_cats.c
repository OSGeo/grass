#include <grass/gis.h>
#include <grass/raster.h>
#include "ncb.h"
int divr_cats(void)
{
    Rast_set_cats_fmt("$1 $?different categories$category$", 1.0, 0.0, 0.0, 0.0,
		      &ncb.cats);

    return 0;
}
