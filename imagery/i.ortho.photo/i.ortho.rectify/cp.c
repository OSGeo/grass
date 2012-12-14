#include <stdlib.h>
#include <string.h>
#include "global.h"

int get_conz_points(void)
{
    char msg[200];
    /* struct Ortho_Control_Points cpz; */

    if (!I_get_con_points(group.name, &group.control_points))
	exit(0);

    sprintf(msg, _("Control Z Point file for group [%s] in [%s] \n \n"),
	    group.name, G_mapset());

    G_verbose_message(_("Computing equations..."));

    Compute_ortho_equation();

    switch (group.con_equation_stat) {
    case -1:
	strcat(msg, _("Poorly placed Control Points!\n"));
	strcat(msg, _("Can not generate the transformation equation.\n"));
	strcat(msg, _("Run OPTION 7 of i.ortho.photo again!\n"));
	break;
    case 0:
	strcat(msg, _("No active Control Points!\n"));
	strcat(msg, _("Can not generate the transformation equation.\n"));
	strcat(msg, _("Run OPTION 7 of i.ortho.photo!\n"));
	break;
    default:
	return 1;
    }
    G_fatal_error(msg);
}

int get_ref_points(void)
{
    char msg[200];
    /* struct Ortho_Photo_Points cp; */

    if (!I_get_ref_points(group.name, &group.photo_points))
	exit(0);

    sprintf(msg, _("Reference Point file for group [%s] in [%s] \n \n"),
	    group.name, G_mapset());

    Compute_ref_equation();
    switch (group.ref_equation_stat) {
    case -1:
	strcat(msg, _("Poorly placed Reference Points!\n"));
	strcat(msg, _("Can not generate the transformation equation.\n"));
	strcat(msg, _("Run OPTION 5 of i.ortho.photo again!\n"));
	break;

    case 0:
	strcat(msg, _("No active Reference Points!\n"));
	strcat(msg, _("Can not generate the transformation equation.\n"));
	strcat(msg, _("Run OPTION 5 of i.ortho.photo!\n"));
	break;
    default:
	return 1;
    }
    G_fatal_error(msg);
    /* exit(1);   shouldn't get here */
}
