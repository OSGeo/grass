
/****************************************************************/
/*                                                              */
/*      delete.c        in      ~/src/Glos                      */
/*                                                              */
/*      This function detaches a point data structure from      */
/*      the linked list and frees memory allocated for it.      */
/*                                                              */

/****************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/segment.h>
#include "point.h"

#define  PT_TO_DELETE_X		  PT_TO_DELETE->x
#define  PT_TO_DELETE_Y		  PT_TO_DELETE->y
#define  PT_NEXT_TO_DELETED	  PT_TO_DELETE->next
#define  PT_PREVIOUS_TO_DELETED	  PT_TO_DELETE->previous
#define  NEXT_PT_BACK_PTR	  PT_TO_DELETE->next->previous
#define	 PREVIOUS_PT_NEXT_PTR     PT_TO_DELETE->previous->next

struct point *delete(struct point *PT_TO_DELETE, struct point *head,
		     SEGMENT * seg_out_p, int row_viewpt, int col_viewpt)
{
    FCELL data;
    FCELL *value;

    /*      mark deleted points by light brownish color     */
    data = 1;
    value = &data;
    segment_put(seg_out_p, value,
		row_viewpt - PT_TO_DELETE_Y, PT_TO_DELETE_X + col_viewpt);

    if (PT_TO_DELETE == head) {	/*  first one ?  */
	NEXT_PT_BACK_PTR = NULL;
	head = PT_NEXT_TO_DELETED;
	G_free(PT_TO_DELETE);
	return (head);
    }

    /*  otherwise  */

    NEXT_PT_BACK_PTR = PT_PREVIOUS_TO_DELETED;
    PREVIOUS_PT_NEXT_PTR = PT_NEXT_TO_DELETED;
    G_free(PT_TO_DELETE);

    return (head);

}

/************* END OF FUNCTION "DELETE" *************************/
