#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/imagery.h>
#include <grass/glocale.h>
#include "global.h"

int get_target(char *group)
{
    char location[GMAPSET_MAX];
    char mapset[GMAPSET_MAX];
    char buf[1024];
    int stat;

    if (group && *group) {
        if (!I_get_target(group, location, mapset)) {
            sprintf(buf, _("Target information for group <%s> missing"), group);
            goto error;
        }
    }
    else {
        sprintf(location, "%s", G_location());
        sprintf(mapset, "%s", G_mapset());
    }

    sprintf(buf, "%s/%s", G_gisdbase(), location);
    if (access(buf, 0) != 0) {
        sprintf(buf, _("Target project <%s> not found"), location);
        goto error;
    }
    select_target_env();
    G_setenv_nogisrc("LOCATION_NAME", location);
    stat = G_mapset_permissions(mapset);
    if (stat > 0) {
        G_setenv_nogisrc("MAPSET", mapset);
        select_current_env();
        return 1;
    }
    sprintf(buf, _("Mapset <%s> in target project <%s> - "), mapset, location);
    strcat(buf, stat == 0 ? _("permission denied") : _("not found"));
error:
    strcat(buf, _("Please run i.target for group."));
    strcat(buf, group);
    G_fatal_error("%s", buf);
    return 1; /* never reached */
}
