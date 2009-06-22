#include <grass/gis.h>
#include <grass/raster.h>

int new_range(char *name, struct Reclass *reclass)
{
    int i;
    struct Range range;

    Rast_init_range(&range);

    for (i = 0; i < reclass->num; i++)
	Rast_update_range(reclass->table[i], &range);
    Rast_write_range(name, &range);

    return 0;
}
