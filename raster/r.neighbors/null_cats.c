#include <grass/gis.h>
#include <grass/Rast.h>
#include "ncb.h"

int null_cats(void)
{
    int ncats;

    ncats = Rast_number_of_cats(ncb.newcell, G_mapset());
    Rast_init_cats(ncats, ncb.title, &ncb.cats);

    return 0;
}
