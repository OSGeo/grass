#include <unistd.h>
#include <string.h>
#include "global.h"

int get_target(char *group)
{
    char location[GMAPSET_MAX];
    char mapset[GMAPSET_MAX];
    char buf[1024];
    int stat;

    if (!I_get_target(group, location, mapset)) {
	sprintf(buf, _("Target information for group <%s> missing"), group);
	goto error;
    }

    sprintf(buf, "%s/%s", G_gisdbase(), location);
    if (access(buf, 0) != 0) {
	sprintf(buf, _("Target location <%s> not found"), location);
	goto error;
    }
    select_target_env();
    G__setenv("LOCATION_NAME", location);
    stat = G__mapset_permissions(mapset);
    if (stat > 0) {
	G__setenv("MAPSET", mapset);
	G_get_window(&target_window);
	select_current_env();
	return 1;
    }
    sprintf(buf, _("Mapset <%s> in target location <%s> - "), mapset, location);
    strcat(buf, stat == 0 ? _("permission denied") : _("not found"));
  error:
    G_fatal_error(buf);
}
