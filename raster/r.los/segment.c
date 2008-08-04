
/****************************************************************
 *								*
 *	segment.c	in	~/src/Glos			*
 *								*
 *	This function picks up all the points in one segment	*
 *	and performs los analysis on them.			*
 *								*
 ****************************************************************/

#include <grass/segment.h>
#include "point.h"

#define	  NEXT_PT		PRESENT_PT->next
#define	  NEXT_PT_BACK_PTR	PRESENT_PT->next->previous
#define	  HEAD_BACK_PTR		head->previous

struct point *segment(int segment_no, int xmax, int ymax,
		      double slope_1, double slope_2, int flip,
		      int sign_on_y, int sign_on_x, int viewpt_elev,
		      SEGMENT * seg_in_p, SEGMENT * seg_out_p,
		      SEGMENT * seg_patt_p, int row_viewpt, int col_viewpt,
		      int patt_flag, int docurv, double ellps_a)
{
    int lower_limit_y, upper_limit_y, less, x, y,
	x_actual, y_actual, x_flip, y_flip;
    struct point *head = (struct point *)NULL, *PRESENT_PT;
    int quadrant;

    /*      decide which one of the four quadrants          */
    quadrant = 1 + (segment_no - 1) / 4;

    if (slope_1 != 0) {
	less = ymax / slope_1 + 0.99;
	xmax = (xmax <= less) ? xmax : less;
    }

    /*      outer loop over x coors for picking up points   */
    for (x = xmax; x > 0; x--) {

	/*      calculate limits for range of y for a single x  */
	lower_limit_y = x * slope_1 + 0.9;
	upper_limit_y = x * slope_2;
	upper_limit_y = (upper_limit_y <= ymax) ? upper_limit_y : ymax;

	/*      loop over y range to pick up correct points     */
	for (y = upper_limit_y; y >= lower_limit_y; y--) {
	    /*  calculate actual x, y that lie in current segment   */

	    if (flip == 0) {
		x_flip = x;
		y_flip = y;
	    }
	    else {
		y_flip = x;
		x_flip = y;
	    }

	    x_actual = sign_on_x * x_flip;
	    y_actual = sign_on_y * y_flip;

	    /*      add chosen point to the point list              */
	    head = make_list(head, y_actual, x_actual, seg_in_p,
			     viewpt_elev, quadrant, row_viewpt, col_viewpt,
			     docurv, ellps_a);

	}
    }				/* end of outer loop */


    if (head != NULL) {
	/*      assign back pointers in linked list             */
	HEAD_BACK_PTR = NULL;
	PRESENT_PT = head;

	while (NEXT_PT != NULL) {
	    NEXT_PT_BACK_PTR = PRESENT_PT;
	    PRESENT_PT = NEXT_PT;
	}

	head = hidden_point_elimination(head, viewpt_elev,
					seg_in_p, seg_out_p, seg_patt_p,
					quadrant, sign_on_y, sign_on_x,
					row_viewpt, col_viewpt, patt_flag,
					docurv, ellps_a);
    }

    return (head);

}
