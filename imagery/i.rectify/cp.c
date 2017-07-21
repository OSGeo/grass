#include <stdlib.h>
#include <string.h>
#include "global.h"

int get_control_points(struct Image_Group *group, int order)
{
    char msg[200];

    if (!I_get_control_points(group->name, &group->control_points))
	exit(0);

    sprintf(msg, _("Control Point file for group <%s@%s> - "),
	    group->name, G_mapset());

    G_verbose_message(_("Computing equations..."));

    if (order == 0) {
	switch (I_compute_georef_equations_tps(&group->control_points,
	                                       &group->E12_t,
					       &group->N12_t,
					       &group->E21_t,
					       &group->N21_t)) {
	case 0:
	    strcat(msg, _("Not enough active control points for thin plate spline."));
	    break;
	case -1:
	    strcat(msg, _("Poorly placed control points."));
	    strcat(msg, _(" Can not generate the transformation equation."));
	    break;
	case -2:
	    strcat(msg, _("Not enough memory to solve transformation equations."));
	    break;
	case -3:
	    strcat(msg, _("Invalid order."));
	    break;
	default:
	    return 1;
	}
    }
    else {
	switch (I_compute_georef_equations(&group->control_points,
	                                   group->E12, group->N12,
					   group->E21, group->N21,
					   order)) {
	case 0:
	    sprintf(&msg[strlen(msg)],
		    _("Not enough active control points for current order, %d are required."),
		    (order + 1) * (order + 2) / 2);
	    break;
	case -1:
	    strcat(msg, _("Poorly placed control points."));
	    strcat(msg, _(" Can not generate the transformation equation."));
	    break;
	case -2:
	    strcat(msg, _("Not enough memory to solve transformation equations."));
	    break;
	case -3:
	    strcat(msg, _("Invalid order"));
	    break;
	default:
	    return 1;
	}
    }
    G_fatal_error("%s", msg);

    return 0;
}
