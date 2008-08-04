#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/Vect.h>

int main(int argc, char *argv[])
{
    int lister();

    G_gisinit(argv[0]);

    G_list_element("vector", "vector", argc > 1 ? argv[1] : "", lister);
    exit(0);
}

int lister(char *name, char *mapset, char *title)
{
    struct Map_info Map;

    *title = 0;
    if (*name) {
	Vect_open_old_head(&Map, name, mapset);
	strcpy(title, Vect_get_map_name(&Map));
	Vect_close(&Map);
    }

    return 0;
}
