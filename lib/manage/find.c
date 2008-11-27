#include <string.h>
#include "list.h"

const char *find(int n, char *name, const char *mapsets)
{
    const char *mapset;

    mapset = G_find_file2(list[n].element[0], name, mapsets);
    if (mapset) {
	char temp[GNAME_MAX];

	sscanf(name, "%s", temp);
	strcpy(name, temp);
    }
    return mapset;
}
