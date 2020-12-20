#include <unistd.h>
#include <string.h>
#include "global.h"

int get_target(char *group)
{
    char project[GMAPSET_MAX];
    char subproject[GMAPSET_MAX];
    char buf[1024];
    int stat;

    if (!I_get_target(group, project, subproject)) {
	sprintf(buf, _("Target information for group <%s> missing"), group);
	goto error;
    }

    sprintf(buf, "%s/%s", G_gisdbase(), project);
    if (access(buf, 0) != 0) {
	sprintf(buf, _("Target project <%s> not found"), project);
	goto error;
    }
    select_target_env();
    G_setenv_nogisrc("LOCATION_NAME", project);
    stat = G_subproject_permissions(subproject);
    if (stat > 0) {
	G_setenv_nogisrc("MAPSET", subproject);
	G_get_window(&target_window);
	select_current_env();
	return 1;
    }
    sprintf(buf, _("Subproject <%s> in target project <%s> - "), subproject, project);
    strcat(buf, stat == 0 ? _("permission denied") : _("not found"));
  error:
    strcat(buf, "\n");
    strcat(buf, _("Please run i.target for group "));
    strcat(buf, group);
    G_fatal_error("%s", buf);
    return 1;			/* never reached */
}
