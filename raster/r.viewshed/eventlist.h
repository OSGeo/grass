
/****************************************************************************
 *
 * MODULE:       r.viewshed
 *
 * AUTHOR(S):    Laura Toma, Bowdoin College - ltoma@bowdoin.edu
 *               Yi Zhuang - yzhuang@bowdoin.edu
 *
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
 * COPYRIGHT: (C) 2008 - 2011 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 *****************************************************************************/


#ifndef _EVENTLIST_H
#define _EVENTLIST_H

#include "grid.h"
#include "visibility.h"

#include <grass/iostream/ami.h>


#define ENTERING_EVENT 1
#define EXITING_EVENT -1
#define CENTER_EVENT 0

typedef struct event_
{
    dimensionType row, col;	//location of the center of cell
    // 3 elevation values:
    // elev[0]: entering
    // elev[1]: center
    // elev[2]: exiting
    surface_type elev[3];			//elevation here
    double angle;
    char eventType;

    //type of the event: ENTERING_EVENT,  EXITING_EVENT, CENTER_EVENT
} AEvent;



/* ------------------------------------------------------------ */
/*determines if the point at row,col is outside the maximum distance
   limit wrt viewpoint.   Return 1 if the point is outside
   limit, 0 if point is inside limit. */
int
is_point_outside_max_dist(Viewpoint vp, GridHeader hd,
			  dimensionType row, dimensionType col,
			  float maxDist);



/*sort the event list by the angle around the viewpoint) */
void sort_event_list(AMI_STREAM < AEvent > **eventList);

class RadialCompare
{
  public:int compare(const AEvent &, const AEvent &);
};
int radial_compare_events(const void *a, const void *b);


    /*sort the event list by the distance from the viewpoint */
class DistanceCompare
{
  public:int compare(const AEvent &, const AEvent &);
};
void print_event(AEvent a, int debug_level);


    /*computes the distance from the event to the viewpoint. Note: all 3
       //events associate to a cell are considered at the same distance, from
       //the center of the cell to the viewpoint */
double get_square_distance_from_viewpoint(const AEvent & a,
					  const Viewpoint & vp);

    /*sort the event list in distance order */
void sort_event_list_by_distance(AMI_STREAM < AEvent > **eventList,
				 Viewpoint vp);


    /*return the angle from this event wrt viewpoint; the type of the
       //event is taken into position to compute a different amngle for each
       //event associated with a cell */
double calculate_event_angle(AEvent * e, Viewpoint * vp);


    /*compute the gradient of the CENTER of this event wrt viewpoint. For
       //efficiency it does not compute the gradient, but the square of the
       //tan of the gradient. Assuming all gradients are computed the same
       //way, this is correct. */
double calculate_center_gradient(AEvent * e, Viewpoint * vp);


    /*calculate the exit angle corresponding to this cell */
double calculate_exit_angle(dimensionType row, dimensionType col,
			    Viewpoint * vp);

    /*calculate the enter angle corresponding to this cell */
double calculate_enter_angle(dimensionType row, dimensionType col,
			     Viewpoint * vp);

    /*calculate the exact position of the given event, and store them in x
       //and y. */
void calculate_event_position(AEvent e, dimensionType viewpointRow,
			      dimensionType viewpointCol, double *y,
			      double *x);
    /* calculate the neighbouring row, col of the given event, and store them in x
       //and y. */
void
calculate_event_row_col(AEvent e, dimensionType viewpointRow,
			 dimensionType viewpointCol, int *y, int *x);


double calculate_angle(double eventX, double eventY, double viewpointX,
		       double viewpointY);

#endif
