#include <grass/gis.h>
#include <grass/glocale.h>
#include "globals.h"

#define PT Region.point
#define SPT Region.saved_point


int save_region(void)
{
    int i;

    if (!Region.area.completed || Region.view == NULL || Region.npoints <= 0)
	G_warning(_("Region is not complete, can not save."));
    else {
	for (i = 0; i < Region.npoints; i++) {
	    SPT[i].x = PT[i].x;
	    SPT[i].y = PT[i].y;
	}
	Region.saved_npoints = Region.npoints;
	Region.saved_view = Region.view;
	Region.area.saved = 1;
    }
    return (0);
}
