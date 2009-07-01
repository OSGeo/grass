
/****************************************************************/
/*                                                              */
/*      mark_visible_points.c   in      ~/src/Glos              */
/*                                                              */
/*      This function marks all points that are visible in      */
/*      any one segment on the outputmap                        */
/*                                                              */

/****************************************************************/

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/segment.h>
#include "point.h"

#define		PT_TO_MARK_X		PT_TO_MARK->x
#define		PT_TO_MARK_Y		PT_TO_MARK->y
#define		NEXT_PT_TO_MARK		PT_TO_MARK->next
#define		PT_TO_MARK_INCL		PT_TO_MARK->inclination

int
mark_visible_points(struct point *head, SEGMENT * seg_out_p, int row_viewpt,
		    int col_viewpt, double color_factor, double COLOR_SHIFT)
{
    struct point *PT_TO_MARK;
    FCELL data;

    PT_TO_MARK = head;

    while (PT_TO_MARK != NULL) {	/*        loop till end of list   */
	segment_get(seg_out_p, &data,
		    row_viewpt - PT_TO_MARK_Y, PT_TO_MARK_X + col_viewpt);

	if (data != (FCELL) 1) {	/* point has not been deleted previously        */
	    /* old value    
	       data = (FCELL ) (PT_TO_MARK_INCL* 57.3 * color_factor 
	       + COLOR_SHIFT);
	       end of old data      */
	    data = (FCELL) (PT_TO_MARK_INCL * 57.325 + 90.0);
	    segment_put(seg_out_p, &data,
			row_viewpt - PT_TO_MARK_Y, PT_TO_MARK_X + col_viewpt);
	}

	PT_TO_MARK = NEXT_PT_TO_MARK;	/* next visible point   */
    }
    return 0;
}

/********* END OF FUNCTION "MARK_VISIBLE_POINTS" ****************/
