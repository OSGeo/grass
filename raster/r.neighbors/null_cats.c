#include <grass/gis.h>
#include "ncb.h"

int null_cats(void)
{
    int ncats;

    ncats = G_number_of_cats(ncb.newcell, G_mapset());
    G_init_cats(ncats, ncb.title, &ncb.cats);

    return 0;
}
