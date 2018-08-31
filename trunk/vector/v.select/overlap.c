#include <stdlib.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>

#include "proto.h"

/* Add all elements of area A to the list */
void add_aarea(struct Map_info *In, int aarea, int *ALines, int *AAreas)
{
    int i, j, aline, naisles, aisle, acentroid;
    static struct ilist *BoundList = NULL;

    if (!BoundList)
	BoundList = Vect_new_list();

    acentroid = Vect_get_area_centroid(In, aarea);
    ALines[acentroid] = 1;

    Vect_get_area_boundaries(In, aarea, BoundList);
    for (i = 0; i < BoundList->n_values; i++) {
	aline = abs(BoundList->value[i]);
	ALines[aline] = 1;
    }

    naisles = Vect_get_area_num_isles(In, aarea);

    for (j = 0; j < naisles; j++) {
	aisle = Vect_get_area_isle(In, aarea, j);

	Vect_get_isle_boundaries(In, aisle, BoundList);
	for (i = 0; i < BoundList->n_values; i++) {
	    aline = abs(BoundList->value[i]);
	    ALines[aline] = 1;
	}
    }
    if (AAreas)
	AAreas[aarea] = 1;
}

/* Returns 1 if line1 from Map1 overlaps area2 from Map2,
 *         0 otherwise */
int line_overlap_area(struct line_pnts *LPoints, struct line_pnts *OPoints,
		      struct line_pnts **IPoints, int nisles)
{
    int i, isle;

    G_debug(4, "line_overlap_area area");

    /* Try if any of line vertices is within area */
    for (i = 0; i < LPoints->n_points; i++) {

	if (Vect_point_in_poly(LPoints->x[i], LPoints->y[i], OPoints)) {
	    int inside = 1;
	    
	    for (isle = 0; isle < nisles; isle++) {
		if (Vect_point_in_poly(LPoints->x[i], LPoints->y[i], IPoints[isle])) {
		    inside = 0;
		    break;
		}
	    }
	    if (inside) {
		G_debug(4, "  -> line vertex inside area");
		return 1;
	    }
	}
    }

    /* Skip points */
    if (LPoints->n_points < 2)
	return 0;

    /* Try intersections of line with area/isles boundary */
    /* Outer boundary */

    if (Vect_line_check_intersection2(LPoints, OPoints, 0)) {
	G_debug(4, "  -> line intersects outer area boundary");
	return 1;
    }

    for (i = 0; i < nisles; i++) {
	if (Vect_line_check_intersection2(LPoints, IPoints[i], 0)) {
	    G_debug(4, "  -> line intersects area island boundary");
	    return 1;
	}
    }
    return 0;
}
