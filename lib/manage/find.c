#include <string.h>
#include "list.h"

char *find(int n, char *name, char *mapsets)
{
    char *mapset;

    mapset = G_find_file(list[n].element[0], name, mapsets);
    if (mapset) {
	char temp[GNAME_MAX];

	sscanf(name, "%s", temp);
	strcpy(name, temp);
    }
    return mapset;
}
