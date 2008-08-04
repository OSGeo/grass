#include <unistd.h>
#include <string.h>
#include <grass/gis.h>
#include "globals.h"

/* read the target for the group and cast it into the alternate GRASS env */

static int which_env;

int get_target(void)
{
    char location[GNAME_MAX];
    char mapset[GMAPSET_MAX];
    char buf[1024];
    int stat;

    if (!I_get_target(group.name, location, mapset)) {
	sprintf(buf, "Target information for group [%s] missing\n",
		group.name);
	goto error;
    }

    sprintf(buf, "%s/%s", G_gisdbase(), location);
    if (access(buf, 0) != 0) {
	sprintf(buf, "Target location [%s] not found\n", location);
	goto error;
    }
    G__create_alt_env();
    G__setenv("LOCATION_NAME", location);
    stat = G__mapset_permissions(mapset);
    if (stat > 0) {
	G__setenv("MAPSET", mapset);
	G__create_alt_search_path();
	G__switch_env();
	G__switch_search_path();
	which_env = 0;
	return 1;
    }
    sprintf(buf, "Mapset [%s] in target location [%s] - ", mapset, location);
    strcat(buf, stat == 0 ? "permission denied\n" : "not found\n");
  error:
    strcat(buf, "Please run i.target for group ");
    strcat(buf, group.name);
    G_fatal_error(buf);

    return -1;
}

int select_current_env(void)
{
    if (which_env != 0) {
	G__switch_env();
	G__switch_search_path();
	which_env = 0;
    }

    return 0;
}

int select_target_env(void)
{
    if (which_env != 1) {
	G__switch_env();
	G__switch_search_path();
	which_env = 1;
    }

    return 0;
}
