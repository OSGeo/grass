#include <stdlib.h>
#include <string.h>
#include "global.h"

int get_conz_points(struct Ortho_Image_Group *group)
{
    char msg[200];

    if (!I_get_con_points(group->name, &group->control_points))
	exit(0);

    sprintf(msg, _("Control Z Point file for group <%s@%s> - "),
	    group->name, G_mapset());

    G_verbose_message(_("Computing equations..."));

    Compute_ortho_equation(group);

    switch (group->con_equation_stat) {
    case -1:
	strcat(msg, _("Poorly placed control points."));
	strcat(msg, _(" Can not generate the transformation equation."));
	strcat(msg, _(" Run OPTION 7 of i.ortho.photo again!\n"));
	break;
    case 0:
	strcat(msg, _("No active Control Points!"));
	strcat(msg, _(" Can not generate the transformation equation."));
	strcat(msg, _(" Run OPTION 7 of i.ortho.photo!\n"));
	break;
    default:
	return 1;
    }
    G_fatal_error("%s", msg);

    return 0;
}

int get_ref_points(struct Ortho_Image_Group *group)
{
    char msg[200];

    if (!I_get_ref_points(group->name, &group->photo_points))
	exit(0);

    sprintf(msg, _("Reference Point file for group <%s@%s> - "),
	    group->name, G_mapset());

    Compute_ref_equation(group);
    switch (group->ref_equation_stat) {
    case -1:
	strcat(msg, _("Poorly placed reference points."));
	strcat(msg, _(" Can not generate the transformation equation."));
	strcat(msg, _(" Run OPTION 5 of i.ortho.photo again!"));
	break;

    case 0:
	strcat(msg, _("No active reference points."));
	strcat(msg, _(" Can not generate the transformation equation."));
	strcat(msg, _(" Run OPTION 5 of i.ortho.photo!"));
	break;
    default:
	return 1;
    }
    G_fatal_error("%s", msg);

    return 0;
}
