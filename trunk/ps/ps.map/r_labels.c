#include <string.h>
#include <grass/gis.h>
#include "labels.h"
#include "local_proto.h"

#define KEY(x) (strcmp(key,x)==0)

static char *help[] = {
    "font fontname",
    ""
};

int read_labels(char *name, char *mapset)
{
    char fullname[GNAME_MAX + 2 * GMAPSET_MAX + 4];
    char buf[1024];
    char *key, *data;

    sprintf(fullname, "%s in %s", name, mapset);

    if (labels.count >= MAXLABELS) {
	error(fullname, "", "no more label files allowed");
	return 0;
    }

    labels.name[labels.count] = G_store(name);
    labels.mapset[labels.count] = G_store(mapset);

    while (input(2, buf, help)) {
	if (!key_data(buf, &key, &data))
	    continue;

	if (KEY("font")) {
	    get_font(data);
	    labels.font[labels.count] = G_store(data);
	    continue;
	}
	error(key, "", "illegal request (labels)");
    }

    labels.count++;
    return 1;
}
