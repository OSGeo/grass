#include <stdlib.h>
#include <string.h>
#include <grass/glocale.h>
#include "global.h"

int get_control_points(char *group, int order	/* THIS HAS BEEN ADDED WITH THE CRS MODIFICATIONS */
    )
{
    char msg[200];

    if (!I_get_control_points(group, &cp))
	exit(0);

    sprintf(msg, _("Control Point file for group <%s@%s> - "),
	    group, G_mapset());

    if (order == 0) {
	switch (I_compute_georef_equations_tps(&cp, &E12_t, &N12_t, &E21_t, &N21_t)) {
	case 0:
	    sprintf(&msg[strlen(msg)],
		    _("Not enough active control points for thin plate spline."));
	    break;
	case -1:
	    strcat(msg, _("Poorly placed control points."));
	    strcat(msg, _(" Can not generate the transformation equation."));
	    break;
	case -2:
	    strcat(msg, _("Not enough memory to solve for transformation equation"));
	    break;
	case -3:
	    strcat(msg, _("Invalid order"));
	    break;
	default:
	    /* COMMENTED OUT WHEN SUPPORT FOR 3rd ORDER WAS ADDED BY 'CRS'
	       E12a = E12[0]; E12b = E12[1]; E12c = E12[2];
	       N12a = N12[0]; N12b = N12[1]; N12c = N12[2];
	       E21a = E21[0]; E21b = E21[1]; E21c = E21[2];
	       N21a = N21[0]; N21b = N21[1]; N21c = N21[2];
	     */
	    return 1;
	}
    }
    else {
	switch (I_compute_georef_equations(&cp, E12, N12, E21, N21, order)) {
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
	    strcat(msg, _("Not enough memory to solve for transformation equation"));
	    break;
	case -3:
	    strcat(msg, _("Invalid order"));
	    break;
	default:
	    /* COMMENTED OUT WHEN SUPPORT FOR 3rd ORDER WAS ADDED BY 'CRS'
	       E12a = E12[0]; E12b = E12[1]; E12c = E12[2];
	       N12a = N12[0]; N12b = N12[1]; N12c = N12[2];
	       E21a = E21[0]; E21b = E21[1]; E21c = E21[2];
	       N21a = N21[0]; N21b = N21[1]; N21c = N21[2];
	     */
	    return 1;
	}
    }
    G_fatal_error(msg);

    return 0; /* G_fatal_error() calls exit() */
}
