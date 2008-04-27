#include <grass/gis.h>

char *openvect(name)
     char *name;
{
    char *mapset;

    mapset = G_find_vector2(name, "");

    if (mapset == NULL)
	fprintf(stderr, "warning: %s - vector map not found\n", name);
    return mapset;
}
