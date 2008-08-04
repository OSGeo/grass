#include <grass/gis.h>
#include <grass/glocale.h>

char *openvect(char *name)
{
    char *mapset;

    mapset = G_find_vector2(name, "");

    if (mapset == NULL)
	fprintf(stderr, _("warning: %s - vector map not found\n"), name);
    return mapset;
}
