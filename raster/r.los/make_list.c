
/****************************************************************
 *								*
 *	make_list.c	in	~/src/Glos			*
 *								*
 *	This function adds a new point to the point list	*
 *	for any segment of the map.				*
 *								*
 ****************************************************************/

#include <stdlib.h>
#include <math.h>
#include <grass/segment.h>
#include <grass/gis.h>
#include "point.h"
#include "local_proto.h"

#define		NEXT_PT		PRESENT_PT->next

struct point *make_list(struct point *head, int y, int x,
			SEGMENT * seg_in_p, int viewpt_elev,
			int quadrant, int row_viewpt, int col_viewpt,
			int docurv, double ellps_a)
{
    double del_x, del_y, dist, orientation, inclination;
    static struct point *PRESENT_PT;
    extern struct Cell_head window;
    extern double max_dist;

    del_x = abs(x);
    del_y = abs(y);

    dist = sqrt(del_x * del_x + del_y * del_y) * window.ns_res;

    /* if distance from viewpt is greater than the max      */
    /*   range specified, neglect that point                */
    if (dist > max_dist)
	return (head);

    /* otherwise find orientation and inclination           */
    orientation = find_orientation(x, y, quadrant);
    inclination = find_inclination(x, y, viewpt_elev, seg_in_p,
				   row_viewpt, col_viewpt, docurv, ellps_a);

    if (head == NULL) {		/*  first point ?           */
	head = make_point(orientation, inclination, y, x);
	PRESENT_PT = head;
    }
    else {			/*      add new point to tail of list           */
	NEXT_PT = make_point(orientation, inclination, y, x);
	PRESENT_PT = NEXT_PT;
    }

    return (head);

}
