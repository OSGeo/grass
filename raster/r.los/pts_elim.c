
/****************************************************************/
/*                                                              */
/*      hidden_pts_elimination.c        in      ~/src/Glos      */
/*                                                              */
/*      This function prunes a linked list of all points        */
/*      picked up from a map segment to leave only those        */
/*      points in the list that are visible from the viewpt.    */
/*                                                              */

/****************************************************************/

#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/segment.h>
#include <grass/glocale.h>
#include "point.h"
#include "radians.h"
#include "local_proto.h"

#define	  SECOND_PT			head->next

#define   NEXT_BLOCKING_PT		BLOCKING_PT->next
#define   BLOCKING_PT_X	 		BLOCKING_PT->x
#define   BLOCKING_PT_Y			BLOCKING_PT->y
#define   BLOCKING_PT_INCLINATION 	BLOCKING_PT->inclination
#define   BLOCKING_PT_ORIENTATION	BLOCKING_PT->orientation

#define   NEXT_CHECKED_PT		CHECKED_PT->next
#define	  CHECKED_PT_X			CHECKED_PT->x
#define	  CHECKED_PT_Y			CHECKED_PT->y
#define	  CHECKED_PT_INCLINATION	CHECKED_PT->inclination
#define   CHECKED_PT_ORIENTATION	CHECKED_PT->orientation


/****************************************************************/
/*                                                              */
/*      This function takes a linked list of points picked      */
/*      up from a segment of the map and deletes all the points */
/*      from the list that are not visible from the viewing pt. */
/*                                                              */

/****************************************************************/

struct point *hidden_point_elimination(struct point *head, int viewpt_elev,
				       SEGMENT * seg_in_p,
				       SEGMENT * seg_out_p,
				       SEGMENT * seg_patt_p, int quadrant,
				       int sign_on_y, int sign_on_x,
				       int row_viewpt, int col_viewpt,
				       int patt_flag, int docurv,
				       double ellps_a)
{
    struct point *CHECKED_PT, *BLOCKING_PT;
    double orientation_neighbor_1, orientation_neighbor_2,
	inclination_neighbor_1,
	inclination_neighbor_2, interpolated_inclination,
	correct_neighbor_inclination, correct_neighbor_orientation;
    int correct_neighbor_x, correct_neighbor_y, neighbor_1_y,
	neighbor_1_x, neighbor_2_x, neighbor_2_y, uu, vv;
    CELL mask;

    uu = (sign_on_y + sign_on_x) / 2;
    vv = (sign_on_y - sign_on_x) / 2;

    /* move blocking pt. from the 2nd pt till the end       */
    for (BLOCKING_PT = SECOND_PT;
	 BLOCKING_PT != NULL; BLOCKING_PT = NEXT_BLOCKING_PT) {
	/* calc coors of the two immediate neighbors on either  */
	/* side of the blocking point                           */

	if (BLOCKING_PT_X == 0 || BLOCKING_PT_Y == 0) {
	    neighbor_1_x = BLOCKING_PT_X - vv;
	    neighbor_1_y = BLOCKING_PT_Y + uu;

	    neighbor_2_x = BLOCKING_PT_X + uu;
	    neighbor_2_y = BLOCKING_PT_Y + vv;
	}
	else {
	    neighbor_1_x = BLOCKING_PT_X - uu;
	    neighbor_1_y = BLOCKING_PT_Y - vv;

	    neighbor_2_x = BLOCKING_PT_X + vv;
	    neighbor_2_y = BLOCKING_PT_Y - uu;
	}

	/* find orientation and inclination for both neighbors  */
	orientation_neighbor_1 =
	    find_orientation(neighbor_1_x, neighbor_1_y, quadrant);

	orientation_neighbor_2 =
	    find_orientation(neighbor_2_x, neighbor_2_y, quadrant);

	inclination_neighbor_1 =
	    find_inclination(neighbor_1_x, neighbor_1_y, viewpt_elev,
			     seg_in_p, row_viewpt, col_viewpt, docurv,
			     ellps_a);

	inclination_neighbor_2 =
	    find_inclination(neighbor_2_x, neighbor_2_y, viewpt_elev,
			     seg_in_p, row_viewpt, col_viewpt, docurv,
			     ellps_a);


	/* check all points behind the blocking point           */
	for (CHECKED_PT = head;
	     CHECKED_PT != BLOCKING_PT; CHECKED_PT = NEXT_CHECKED_PT) {

	    /* if pattern layer specified, check to see if checked  */
	    /* point is of interest. If not, delete it from list    */
	    if (patt_flag == 1) {
		segment_get(seg_patt_p, &mask,
			    row_viewpt - CHECKED_PT_Y,
			    col_viewpt + CHECKED_PT_X);

		if (mask == 0 || Rast_is_null_value(&mask, CELL_TYPE)) {
		    head = delete(CHECKED_PT, head, seg_out_p,
				  row_viewpt, col_viewpt);
		    goto next_iter;
		}
	    }


	    if (BLOCKING_PT_INCLINATION <= CHECKED_PT_INCLINATION) ;	/*      no need for checking for blocking       */
	    else {		/*      otherwise, proceed to check             */


		/* if checked point directly behind, delete it          */
		if (CHECKED_PT_ORIENTATION == BLOCKING_PT_ORIENTATION) {
		    head = delete(CHECKED_PT, head, seg_out_p,
				  row_viewpt, col_viewpt);
		}
		else {		/* if checked point not directly behind, check  */

		    /* find the coors of the actual neighbor that might be  */
		    /* required for interpolation.                          */
		    if (CHECKED_PT_ORIENTATION > BLOCKING_PT_ORIENTATION) {
			correct_neighbor_x = neighbor_1_x;
			correct_neighbor_y = neighbor_1_y;
			correct_neighbor_inclination = inclination_neighbor_1;
			correct_neighbor_orientation = orientation_neighbor_1;
		    }
		    else {
			correct_neighbor_x = neighbor_2_x;
			correct_neighbor_y = neighbor_2_y;
			correct_neighbor_inclination = inclination_neighbor_2;
			correct_neighbor_orientation = orientation_neighbor_2;
		    }

		    if (fabs(BLOCKING_PT_ORIENTATION - CHECKED_PT_ORIENTATION)
			<
			fabs(BLOCKING_PT_ORIENTATION -
			     correct_neighbor_orientation))
		    {		/* yes, the point neighboring the blocking point      */
			/* must be taken into consideration                     */

			if (CHECKED_PT_Y == correct_neighbor_y && CHECKED_PT_X == correct_neighbor_x) ;	/* same point   */

			else {	/*        CHECK !!                                */


			    /* if the checked point's inclination is even lower     */
			    /* than that of the blocking pt.'s neighbor, blocked    */
			    if (CHECKED_PT_INCLINATION <
				correct_neighbor_inclination) {
				head =
				    delete(CHECKED_PT, head, seg_out_p,
					   row_viewpt, col_viewpt);
			    }

			    else {	/*       INTERPOLATION                          */


				interpolated_inclination =
				    BLOCKING_PT_INCLINATION +
				    (CHECKED_PT_ORIENTATION -
				     BLOCKING_PT_ORIENTATION) /
				    (correct_neighbor_orientation -
				     BLOCKING_PT_ORIENTATION) *
				    (correct_neighbor_inclination -
				     BLOCKING_PT_INCLINATION);

				if (CHECKED_PT_INCLINATION < interpolated_inclination) {	/*      interpolated point blocks               */
				    /* code folded from here */
				    head = delete(CHECKED_PT, head, seg_out_p,
						  row_viewpt, col_viewpt);
				    /* unfolding */
				}
			    }
			}
		    }
		}
	    }
	  next_iter:
	    ;
	}			/* end of loop over points to be checked        */

	/* if pattern layer specified, check if blocking point  */
	/* itself is an area of interest. If not, of no use     */
	if (patt_flag == 1) {
	    segment_get(seg_patt_p, &mask, row_viewpt - BLOCKING_PT_Y,
			col_viewpt + BLOCKING_PT_X);
	    if (mask == 0 || Rast_is_null_value(&mask, CELL_TYPE)) {
	    
	      /* Commenting out the following fixes a bug in r.los.
		 In that program the 8 cells around the viewpoint
		 are marked as visible (when visible)
		 even if they fall outside the area of interest 
		 specified by the patt_map.  This occurs because
		 these cells are always the last blocking points
		 on the segment lists, and therefore don't get 
		 deleted when they should.  This fix allows them
		 to be deleted, but it required modifications
		 to delete.c.  MWL 25/6/99 */	    
	    
		/* if (NEXT_BLOCKING_PT != NULL) */
		
		    head = delete(BLOCKING_PT, head, seg_out_p,
				  row_viewpt, col_viewpt);
	    }
	}

    }				/* end of loop over blocking points             */

    return (head);

}

/*********** END OF FUNCTION "HIDDEN_POINT_ELIMINATION" *********/




/****************************************************************/
/*                                                              */
/*      This function finds the orientation of a point if       */
/*      provided with the number of the quadrant and the        */
/*      coordinates of that point.                              */
/*                                                              */

/****************************************************************/

double find_orientation(int x, int y, int quadrant)
{
    double del_x, del_y, atan(), angle;
    int abs();

    del_x = abs(x);
    del_y = abs(y);

    if (del_x == 0.0)
	angle = PIBYTWO;
    else
	angle = atan(del_y / del_x);

    switch (quadrant) {
    case 1:
	break;
    case 2:
	angle = PI - angle;
	break;
    case 3:
	angle = PI + angle;
	break;
    case 4:
	angle = TWOPI - angle;
	break;
    default:
	break;
    }

    return (angle);

}				/* END OF FUNCTION ANGLE */

/************* END OF FUNCTION "FIND_ORIENTATION" ***************/




/****************************************************************/
/*                                                              */
/*      This function calculates the vertical angle of a point  */
/*      with respect to the viewing pt.                         */
/*                                                              */

/****************************************************************/

double
find_inclination(int x, int y, int viewpt_elev, SEGMENT * seg_in_p,
		 int row_viewpt, int col_viewpt, int docurv, double ellps_a)

{
    double del_x, del_y, dist;
    int abs();
    FCELL picked_pt_elev;
    extern struct Cell_head window;

    del_x = abs(x);
    del_y = abs(y);

    dist = sqrt(del_x * del_x + del_y * del_y) * window.ns_res;

    segment_get(seg_in_p, &picked_pt_elev, row_viewpt - y, x + col_viewpt);

    if (docurv)			/* decrease height of target point */
	picked_pt_elev = picked_pt_elev - ((dist * dist) / (2 * ellps_a));

    return (atan((picked_pt_elev - viewpt_elev) / dist));
}

/************ END OF FUNCTION "FIND_INCLINATION"*****************/
