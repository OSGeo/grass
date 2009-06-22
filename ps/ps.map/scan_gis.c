#include <unistd.h>
#include <string.h>
#include <grass/gis.h>
#include "local_proto.h"

int scan_gis(char *element, char *desc, char *key, char *data,
	     char *name, char *mapset, int gobble)
{
    const char *ms;


    *mapset = 0;
    if (sscanf(data, "%s %s", name, mapset) < 1) {
	error(key, data, "illegal request (scan_gis)");
	if (gobble)
	    gobble_input();
	return 0;
    }

    if (strcmp(name, "list") == 0) {
	if (isatty(0))
	    G_list_element(element, desc, mapset, (int (*)())NULL);
	reject();
	return 0;
    }

    ms = G_find_file2(element, name, mapset);
    if (ms == NULL) {
	error(key, data, "not found");
	if (gobble)
	    gobble_input();
	return 0;
    }
    strcpy(mapset, ms);
    return 1;
}
