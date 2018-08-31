
/****************************************************************************
 *
 * MODULE:       r.viewshed
 *
 * AUTHOR(S):    Laura Toma, Bowdoin College - ltoma@bowdoin.edu
 *               Yi Zhuang - yzhuang@bowdoin.edu

 *               Ported to GRASS by William Richard -
 *               wkrichar@bowdoin.edu or willster3021@gmail.com
 *               Markus Metz: surface interpolation
 *
 * Date:         april 2011 
 * 
 * PURPOSE: To calculate the viewshed (the visible cells in the
 * raster) for the given viewpoint (observer) location.  The
 * visibility model is the following: Two points in the raster are
 * considered visible to each other if the cells where they belong are
 * visible to each other.  Two cells are visible to each other if the
 * line-of-sight that connects their centers does not intersect the
 * terrain. The terrain is NOT viewed as a tesselation of flat cells, 
 * i.e. if the line-of-sight does not pass through the cell center, 
 * elevation is determined using bilinear interpolation.
 * The viewshed algorithm is efficient both in
 * terms of CPU operations and I/O operations. It has worst-case
 * complexity O(n lg n) in the RAM model and O(sort(n)) in the
 * I/O-model.  For the algorithm and all the other details see the
 * paper: "Computing Visibility on * Terrains in External Memory" by
 * Herman Haverkort, Laura Toma and Yi Zhuang.
 *
 * COPYRIGHT: (C) 2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 *****************************************************************************/


#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

extern "C"
{
#include <grass/gis.h>
}

#include "eventlist.h"



/* forced to use this because DistanceCompare::compare has troubles if
   i put it inside the class */
Viewpoint globalVP;



/* ------------------------------------------------------------ 
   compute the gradient of the CENTER of this event wrt viewpoint. For
   efficiency it does not compute the gradient, but the square of the
   arctan of the gradient. Assuming all gradients are computed the same
   way, this is correct. */
double calculate_center_gradient(AEvent * e, Viewpoint * vp)
{

    assert(e && vp);
    double gradient, sqdist;

    /*square of the distance from the center of this event to vp */
    sqdist = (e->row - vp->row) * (e->row - vp->row) +
	(e->col - vp->col) * (e->col - vp->col);

    gradient = (e->elev[1] - vp->elev) * (e->elev[1] - vp->elev) / sqdist;
    /*maintain sign */
    if (e->elev[1] < vp->elev)
	gradient = -gradient;
    return gradient;
}





/* ------------------------------------------------------------ 
   //calculate the angle at which the event is. Return value is the angle.

   angle quadrants:
   2 1
   3 4 
   ----->x
   |
   |
   |
   V y

 */

/*/////////////////////////////////////////////////////////////////////
   //return the angle from this event wrt viewpoint; the type of the
   //event is taken into position to compute a different amngle for each
   //event associated with a cell */
double calculate_event_angle(AEvent * e, Viewpoint * vp)
{

    assert(e && vp);
    double ex, ey;

    calculate_event_position(*e, vp->row, vp->col, &ey, &ex);
    return calculate_angle(ex, ey, vp->col, vp->row);
}


/*/////////////////////////////////////////////////////////////////////
   //calculate the exit angle corresponding to this cell */
double
calculate_exit_angle(dimensionType row, dimensionType col, Viewpoint * vp)
{
    AEvent e;
    double x, y;

    e.eventType = EXITING_EVENT;
    e.angle = 0;
    e.elev[0] = e.elev[1] = e.elev[2] = 0;
    e.row = row;
    e.col = col;
    calculate_event_position(e, vp->row, vp->col, &y, &x);
    return calculate_angle(x, y, vp->col, vp->row);
}


/*/////////////////////////////////////////////////////////////////////
   //calculate the enter angle corresponding to this cell */
double
calculate_enter_angle(dimensionType row, dimensionType col, Viewpoint * vp)
{
    AEvent e;
    double x, y;

    e.eventType = ENTERING_EVENT;
    e.angle = 0;
    e.elev[0] = e.elev[1] = e.elev[2] = 0;
    e.row = row;
    e.col = col;
    calculate_event_position(e, vp->row, vp->col, &y, &x);
    return calculate_angle(x, y, vp->col, vp->row);
}

/*///////////////////////////////////////////////////////////////////// */
double
calculate_angle(double eventX, double eventY,
		double viewpointX, double viewpointY)
{
    double angle = atan(fabs(eventY - viewpointY) / fabs(eventX - viewpointX));
    
    /*M_PI is defined in math.h to represent 3.14159... */
    if (viewpointY == eventY && eventX > viewpointX) {
	return 0;		/*between 1st and 4th quadrant */
    }
    else if (eventX > viewpointX && eventY < viewpointY) {
	/*first quadrant */
	return angle;

    }
    else if (viewpointX == eventX && viewpointY > eventY) {
	/*between 1st and 2nd quadrant */
	return (M_PI) / 2;

    }
    else if (eventX < viewpointX && eventY < viewpointY) {
	/*second quadrant */
	return (M_PI - angle);

    }
    else if (viewpointY == eventY && eventX < viewpointX) {
	/*between 1st and 3rd quadrant */
	return M_PI;

    }
    else if (eventY > viewpointY && eventX < viewpointX) {
	/*3rd quadrant */
	return (M_PI + angle);

    }
    else if (viewpointX == eventX && viewpointY < eventY) {
	/*between 3rd and 4th quadrant */
	return (M_PI * 3.0 / 2.0);
    }
    else if (eventX > viewpointX && eventY > viewpointY) {
	/*4th quadrant */
	return (M_PI * 2.0 - angle);
    }
    assert(eventX == viewpointX && eventY == viewpointY);
    return 0;
}



/* ------------------------------------------------------------ */
/* calculate the exact position of the given event, and store them in x
   and y.
   quadrants:  1 2
   3 4
   ----->x
   |
   |
   |
   V y
 */
void
calculate_event_position(AEvent e, dimensionType viewpointRow,
			 dimensionType viewpointCol, double *y, double *x)
{
    assert(x && y);
    *x = 0;
    *y = 0;

    if (e.eventType == CENTER_EVENT) {
	/*FOR CENTER_EVENTS */
	*y = e.row;
	*x = e.col;
	return;
    }

    if (e.row < viewpointRow && e.col < viewpointCol) {
	/*first quadrant */
	if (e.eventType == ENTERING_EVENT) {
	    /*if it is ENTERING_EVENT */
	    *y = e.row - 0.5;
	    *x = e.col + 0.5;
	}
	else {
	    /*otherwise it is EXITING_EVENT */
	    *y = e.row + 0.5;
	    *x = e.col - 0.5;
	}

    }
    else if (e.col == viewpointCol && e.row < viewpointRow) {
	/*between the first and second quadrant */
	if (e.eventType == ENTERING_EVENT) {
	    /*if it is ENTERING_EVENT */
	    *y = e.row + 0.5;
	    *x = e.col + 0.5;
	}
	else {
	    /*otherwise it is EXITING_EVENT */
	    *y = e.row + 0.5;
	    *x = e.col - 0.5;
	}

    }
    else if (e.col > viewpointCol && e.row < viewpointRow) {
	/*second quadrant */
	if (e.eventType == ENTERING_EVENT) {
	    /*if it is ENTERING_EVENT */
	    *y = e.row + 0.5;
	    *x = e.col + 0.5;
	}
	else {			/*otherwise it is EXITING_EVENT */
	    *y = e.row - 0.5;
	    *x = e.col - 0.5;
	}

    }
    else if (e.row == viewpointRow && e.col > viewpointCol) {
	/*between the second and the fourth quadrant */
	if (e.eventType == ENTERING_EVENT) {
	    /*if it is ENTERING_EVENT */
	    *y = e.row + 0.5;
	    *x = e.col - 0.5;
	}
	else {
	    /*otherwise it is EXITING_EVENT */
	    *y = e.row - 0.5;
	    *x = e.col - 0.5;
	}

    }
    else if (e.col > viewpointCol && e.row > viewpointRow) {
	/*fourth quadrant */
	if (e.eventType == ENTERING_EVENT) {
	    /*if it is ENTERING_EVENT */
	    *y = e.row + 0.5;
	    *x = e.col - 0.5;
	}
	else {
	    /*otherwise it is EXITING_EVENT */
	    *y = e.row - 0.5;
	    *x = e.col + 0.5;
	}

    }
    else if (e.col == viewpointCol && e.row > viewpointRow) {
	/*between the third and fourth quadrant */
	if (e.eventType == ENTERING_EVENT) {
	    /*if it is ENTERING_EVENT */
	    *y = e.row - 0.5;
	    *x = e.col - 0.5;
	}
	else {
	    /*otherwise it is EXITING_EVENT */
	    *y = e.row - 0.5;
	    *x = e.col + 0.5;
	}

    }
    else if (e.col < viewpointCol && e.row > viewpointRow) {
	/*third quadrant */
	if (e.eventType == ENTERING_EVENT) {
	    /*if it is ENTERING_EVENT */
	    *y = e.row - 0.5;
	    *x = e.col - 0.5;
	}
	else {
	    /*otherwise it is EXITING_EVENT */
	    *y = e.row + 0.5;
	    *x = e.col + 0.5;
	}

    }
    else if (e.row == viewpointRow && e.col < viewpointCol) {
	/*between first and third quadrant */
	if (e.eventType == ENTERING_EVENT) {	/*if it is ENTERING_EVENT */
	    *y = e.row - 0.5;
	    *x = e.col + 0.5;
	}
	else {
	    /*otherwise it is EXITING_EVENT */
	    *y = e.row + 0.5;
	    *x = e.col + 0.5;
	}
    }
    else {
	/*must be the viewpoint cell itself */
	assert(e.row == viewpointRow && e.col == viewpointCol);
	*x = e.col;
	*y = e.row;
    }

    assert(fabs(*x - e.col) < 1 && fabs(*y - e.row) < 1);
    /*
    if ((fabs(*x -e.col) >=1) || (fabs(*y -e.row) >=1)) {
       G_warning("x-e.col=%f, y-e.row=%f ", fabs(*x -e.col), fabs(*y -e.row)); 
       print_event(e, 0); 
       G_warning("vp=(%d, %d), x=%.3f, y=%.3f", viewpointRow, viewpointCol, *x, *y);
       exit(1);
       }
    */
    return;
}

void
calculate_event_row_col(AEvent e, dimensionType viewpointRow,
			 dimensionType viewpointCol, int *y, int *x)
{
    assert(x && y);
    *x = 0;
    *y = 0;

    if (e.eventType == CENTER_EVENT) {
	G_fatal_error("calculate_event_row_col() must not be called for CENTER events");
    }

    if (e.row < viewpointRow && e.col < viewpointCol) {
	/*first quadrant */
	if (e.eventType == ENTERING_EVENT) {
	    /*if it is ENTERING_EVENT */
	    *y = e.row - 1;
	    *x = e.col + 1;
	}
	else {
	    /*otherwise it is EXITING_EVENT */
	    *y = e.row + 1;
	    *x = e.col - 1;
	}

    }
    else if (e.col == viewpointCol && e.row < viewpointRow) {
	/*between the first and second quadrant */
	if (e.eventType == ENTERING_EVENT) {
	    /*if it is ENTERING_EVENT */
	    *y = e.row + 1;
	    *x = e.col + 1;
	}
	else {
	    /*otherwise it is EXITING_EVENT */
	    *y = e.row + 1;
	    *x = e.col - 1;
	}

    }
    else if (e.col > viewpointCol && e.row < viewpointRow) {
	/*second quadrant */
	if (e.eventType == ENTERING_EVENT) {
	    /*if it is ENTERING_EVENT */
	    *y = e.row + 1;
	    *x = e.col + 1;
	}
	else {			/*otherwise it is EXITING_EVENT */
	    *y = e.row - 1;
	    *x = e.col - 1;
	}

    }
    else if (e.row == viewpointRow && e.col > viewpointCol) {
	/*between the second and the fourth quadrant */
	if (e.eventType == ENTERING_EVENT) {
	    /*if it is ENTERING_EVENT */
	    *y = e.row + 1;
	    *x = e.col - 1;
	}
	else {
	    /*otherwise it is EXITING_EVENT */
	    *y = e.row - 1;
	    *x = e.col - 1;
	}

    }
    else if (e.col > viewpointCol && e.row > viewpointRow) {
	/*fourth quadrant */
	if (e.eventType == ENTERING_EVENT) {
	    /*if it is ENTERING_EVENT */
	    *y = e.row + 1;
	    *x = e.col - 1;
	}
	else {
	    /*otherwise it is EXITING_EVENT */
	    *y = e.row - 1;
	    *x = e.col + 1;
	}

    }
    else if (e.col == viewpointCol && e.row > viewpointRow) {
	/*between the third and fourth quadrant */
	if (e.eventType == ENTERING_EVENT) {
	    /*if it is ENTERING_EVENT */
	    *y = e.row - 1;
	    *x = e.col - 1;
	}
	else {
	    /*otherwise it is EXITING_EVENT */
	    *y = e.row - 1;
	    *x = e.col + 1;
	}

    }
    else if (e.col < viewpointCol && e.row > viewpointRow) {
	/*third quadrant */
	if (e.eventType == ENTERING_EVENT) {
	    /*if it is ENTERING_EVENT */
	    *y = e.row - 1;
	    *x = e.col - 1;
	}
	else {
	    /*otherwise it is EXITING_EVENT */
	    *y = e.row + 1;
	    *x = e.col + 1;
	}

    }
    else if (e.row == viewpointRow && e.col < viewpointCol) {
	/*between first and third quadrant */
	if (e.eventType == ENTERING_EVENT) {	/*if it is ENTERING_EVENT */
	    *y = e.row - 1;
	    *x = e.col + 1;
	}
	else {
	    /*otherwise it is EXITING_EVENT */
	    *y = e.row + 1;
	    *x = e.col + 1;
	}
    }
    else {
	/*must be the viewpoint cell itself */
	G_debug(1, "calculate_event_row_col() called for viewpoint cell itself");
	assert(e.row == viewpointRow && e.col == viewpointCol);
	*x = e.col;
	*y = e.row;
    }

    /* assert(fabs(*x - e.col) <= 1 && fabs(*y - e.row) <= 1); */

    if ((abs(*x - e.col) > 1) || (abs(*y - e.row) > 1)) {
	G_warning("calculate_event_row_col() :");
        G_warning("x-e.col=%d, y-e.row=%d", abs(*x - e.col), abs(*y - e.row)); 
        print_event(e, 0); 
        G_warning("vp=(%d, %d), x=%d, y=%d", viewpointRow, viewpointCol, *x, *y);
        exit(1);
    }

    return;
}

/* ------------------------------------------------------------ */
void print_event(AEvent a, int debug_level)
{
    char c = '0';

    if (a.eventType == ENTERING_EVENT)
	c = 'E';
    if (a.eventType == EXITING_EVENT)
	c = 'X';
    if (a.eventType == CENTER_EVENT)
	c = 'Q';
    
    if (debug_level < 1)
	G_warning("ev=[(%3d, %3d), e=%8.1f a=%4.2f t=%c] ",
	   a.row, a.col, a.elev[1], a.angle, c);
    else
	G_debug(debug_level, "ev=[(%3d, %3d), e=%8.1f a=%4.2f t=%c] ",
	   a.row, a.col, a.elev[1], a.angle, c);
    return;
}


/* ------------------------------------------------------------ */
/*computes the distance from the event to the viewpoint. Note: all 3
   //events associate to a cell are considered at the same distance, from
   //the center of the cell to the viewpoint */
double
get_square_distance_from_viewpoint(const AEvent & a, const Viewpoint & vp)
{

    double eventy, eventx, dist;

    calculate_event_position(a, vp.row, vp.col, &eventy, &eventx);

    if (G_projection() == PROJECTION_LL) {
	struct Cell_head window;
	
	Rast_get_window(&window);
	
	dist = G_distance(Rast_col_to_easting(vp.col + 0.5, &window),
			  Rast_row_to_northing(vp.row + 0.5, &window),
			  Rast_col_to_easting(eventx + 0.5, &window),
			  Rast_row_to_northing(eventy + 0.5, &window));

	dist = dist * dist;
    }
    else {
	/*don't take sqrt, it is expensive; suffices for comparison */
	dist = (eventx - vp.col) * (eventx - vp.col) +
	    (eventy - vp.row) * (eventy - vp.row);
    }

    return dist;
}

/* ------------------------------------------------------------ */
/* a duplicate of get_square_distance_from_viewpoint() needed for debug */
double
get_square_distance_from_viewpoint_with_print(const AEvent & a,
					      const Viewpoint & vp)
{

    double eventy, eventx, dist;

    calculate_event_position(a, vp.row, vp.col, &eventy, &eventx);

    if (G_projection() == PROJECTION_LL) {
	struct Cell_head window;
	
	Rast_get_window(&window);
	
	dist = G_distance(Rast_col_to_easting(vp.col + 0.5, &window),
			  Rast_row_to_northing(vp.row + 0.5, &window),
			  Rast_col_to_easting(eventx + 0.5, &window),
			  Rast_row_to_northing(eventy + 0.5, &window));

	dist = dist * dist;
    }
    else {
	/*don't take sqrt, it is expensive; suffices for comparison */
	dist = (eventx - vp.col) * (eventx - vp.col) +
	    (eventy - vp.row) * (eventy - vp.row);
    }

    print_event(a, 2);
    G_debug(2, " pos= (%.3f. %.3f) sqdist=%.3f", eventx, eventy, dist);

    return dist;
}


/* ------------------------------------------------------------ */
/*determines if the point at row,col is outside the maximum distance
   limit.  Return 1 if the point is outside limit, 0 if point is inside
   limit. */
int is_point_outside_max_dist(Viewpoint vp, GridHeader hd,
			      dimensionType row, dimensionType col,
			      float maxDist)
{
    /* it is not too smart to compare floats */
    if ((int)maxDist == INFINITY_DISTANCE)
	return 0;
	
    if (maxDist < G_distance(Rast_col_to_easting(vp.col + 0.5, &hd.window),
                             Rast_row_to_northing(vp.row + 0.5, &hd.window),
	                     Rast_col_to_easting(col + 0.5, &hd.window),
			     Rast_row_to_northing(row + 0.5, &hd.window))) {
	return 1;
    }

    return 0;
}



/* ------------------------------------------------------------ 
   //note: this is expensive because distance is not stored in the event
   //and must be computed on the fly */
int DistanceCompare::compare(const AEvent & a, const AEvent & b)
{

    /*calculate distance from viewpoint
       //don't take sqrt, it is expensive; suffices for comparison */
    double da, db;

    /*da = get_square_distance_from_viewpoint(a, globalVP); 
       //db = get_square_distance_from_viewpoint(b, globalVP); */

    /*in the event these are not inlined */
    double eventy, eventx;

    calculate_event_position(a, globalVP.row, globalVP.col, &eventy, &eventx);
    if (G_projection() == PROJECTION_LL) {
	struct Cell_head window;
	
	Rast_get_window(&window);
	
	da = G_distance(Rast_col_to_easting(globalVP.col + 0.5, &window),
			  Rast_row_to_northing(globalVP.row + 0.5, &window),
			  Rast_col_to_easting(eventx + 0.5, &window),
			  Rast_row_to_northing(eventy + 0.5, &window));

	da = da * da;
    }
    else {
	/*don't take sqrt, it is expensive; suffices for comparison */
	da = (eventx - globalVP.col) * (eventx - globalVP.col) +
	    (eventy - globalVP.row) * (eventy - globalVP.row);
    }


    calculate_event_position(b, globalVP.row, globalVP.col, &eventy, &eventx);
    if (G_projection() == PROJECTION_LL) {
	struct Cell_head window;
	
	Rast_get_window(&window);
	
	db = G_distance(Rast_col_to_easting(globalVP.col + 0.5, &window),
			  Rast_row_to_northing(globalVP.row + 0.5, &window),
			  Rast_col_to_easting(eventx + 0.5, &window),
			  Rast_row_to_northing(eventy + 0.5, &window));

	db = db * db;
    }
    else {
	/*don't take sqrt, it is expensive; suffices for comparison */
	db = (eventx - globalVP.col) * (eventx - globalVP.col) +
	    (eventy - globalVP.row) * (eventy - globalVP.row);
    }

    if (da > db) {
	return 1;
    }
    else if (da < db) {
	return -1;
    }
    else {
	return 0;
    }
    return 0;
}


/* ------------------------------------------------------------ */
int RadialCompare::compare(const AEvent & a, const AEvent & b)
{

    if (a.row == b.row && a.col == b.col && a.eventType == b.eventType)
	return 0;

    assert(a.angle >= 0 && b.angle >= 0);

    if (a.angle > b.angle) {
	return 1;
    }
    else if (a.angle < b.angle) {
	return -1;
    }
    else {
	/*a.angle == b.angle */
	if (a.eventType == EXITING_EVENT)
	    return -1;
	if (b.eventType == EXITING_EVENT)
	    return 1;
	if (a.eventType == ENTERING_EVENT)
	    return 1;
	if (b.eventType == ENTERING_EVENT)
	    return -1;
	return 0;
    }
}

/* ------------------------------------------------------------ */
/* a copy of the function above is needed by qsort, when the
   computation runs in memory */

int radial_compare_events(const void *x, const void *y)
{

    AEvent *a, *b;

    a = (AEvent *) x;
    b = (AEvent *) y;
    if (a->row == b->row && a->col == b->col && a->eventType == b->eventType)
	return 0;

    assert(a->angle >= 0 && b->angle >= 0);

    if (a->angle > b->angle) {
	return 1;
    }
    else if (a->angle < b->angle) {
	return -1;
    }
    else {
	/*a->angle == b->angle */
	if (a->eventType == EXITING_EVENT)
	    return -1;
	else if (a->eventType == ENTERING_EVENT)
	    return 1;
	return 0;
    }
}



/* ------------------------------------------------------------ */
/*sort the event list in radial order */
void sort_event_list(AMI_STREAM < AEvent > **eventList)
{

    /*printf("sorting events.."); fflush(stdout); */
    assert(*eventList);

    AMI_STREAM < AEvent > *sortedStr;
    RadialCompare cmpObj;
    AMI_err ae;

    ae = AMI_sort(*eventList, &sortedStr, &cmpObj, 1);
    assert(ae == AMI_ERROR_NO_ERROR);
    *eventList = sortedStr;
    /*printf("..done.\n"); fflush(stdout); */
    return;
}


/* ------------------------------------------------------------ */
/*sort the event list in distance order */
void
sort_event_list_by_distance(AMI_STREAM < AEvent > **eventList, Viewpoint vp)
{

    /*printf("sorting events by distance from viewpoint.."); fflush(stdout); */
    assert(*eventList);

    AMI_STREAM < AEvent > *sortedStr;
    DistanceCompare cmpObj;

    globalVP.row = vp.row;
    globalVP.col = vp.col;
    /*printViewpoint(globalVP); */
    AMI_err ae;

    ae = AMI_sort(*eventList, &sortedStr, &cmpObj, 1);
    assert(ae == AMI_ERROR_NO_ERROR);
    *eventList = sortedStr;
    /*printf("..sorting done.\n"); fflush(stdout); */
    return;
}

