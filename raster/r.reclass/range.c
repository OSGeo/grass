#include <grass/gis.h>

int new_range(char *name, struct Reclass *reclass)
{
    int i;
    struct Range range;

    G_init_range(&range);

    for (i = 0; i < reclass->num; i++)
	G_update_range(reclass->table[i], &range);
    G_write_range(name, &range);

    return 0;
}
