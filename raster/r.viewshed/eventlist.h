
/****************************************************************************
 *
 * MODULE:       r.viewshed
 *
 * AUTHOR(S):    Laura Toma, Bowdoin College - ltoma@bowdoin.edu
 *               Yi Zhuang - yzhuang@bowdoin.edu

 *               Ported to GRASS by William Richard -
 *               wkrichar@bowdoin.edu or willster3021@gmail.com
 *
 * Date:         july 2008 
 * 
 * PURPOSE: To calculate the viewshed (the visible cells in the
 * raster) for the given viewpoint (observer) location.  The
 * visibility model is the following: Two points in the raster are
 * considered visible to each other if the cells where they belong are
 * visible to each other.  Two cells are visible to each other if the
 * line-of-sight that connects their centers does not intersect the
 * terrain. The height of a cell is assumed to be constant, and the
 * terrain is viewed as a tesselation of flat cells.  This model is
 * suitable for high resolution rasters; it may not be accurate for
 * low resolution rasters, where it may be better to interpolate the
 * height at a point based on the neighbors, rather than assuming
 * cells are "flat".  The viewshed algorithm is efficient both in
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


#ifndef _EVENTLIST_H
#define _EVENTLIST_H

#include "grid.h"
#include "visibility.h"

#ifdef __GRASS__
#include <grass/iostream/ami.h>
#else 
#include <ami.h>
#endif


#define ENTERING_EVENT 1
#define EXITING_EVENT -1
#define CENTER_EVENT 0

typedef struct event_
{
    dimensionType row, col;	//location of the center of cell
    float elev;			//elevation here
    double angle;
    char eventType;

    //type of the event: ENTERING_EVENT,  EXITING_EVENT, CENTER_EVENT
} AEvent;



/* ------------------------------------------------------------ */
/*determines if the point at row,col is outside the maximum distance
  limit wrt viewpoint.   Return 1 if the point is outside
  limit, 0 if point is inside limit. */
int 
is_point_outside_max_dist(Viewpoint  vp, GridHeader hd,
						  dimensionType row, dimensionType col,
						  float maxDist);



/* ------------------------------------------------------------ */
/* input: an array capable to hold the max number of events, an
   arcascii file, a grid header and a viewpoint; action: figure out
   all events in the input file, and write them to the event
   list. data is allocated and initialized with all the cells on the
   same row as the viewpoint. it returns the number of events.
*/
size_t
init_event_list_in_memory(AEvent * eventList, char* inputfname, 
						  Viewpoint * vp,
						  GridHeader * hd, ViewOptions viewOptions,
						  double **data, MemoryVisibilityGrid* visgrid);





/* ------------------------------------------------------------ */
/* input: an arcascii file, an eventlist stream, and a viewpoint.
   action: figure out all events in the input file, and write them to
   the stream. hd is initialized from file. if data is not NULL, data
   is allocated and initialized with all the cells on the same row as
   the viewpoint.
*/
AMI_STREAM <AEvent>*
init_event_list(char* inputfname, Viewpoint * vp, GridHeader * hd, 
				ViewOptions viewOptions, double **data, 
				IOVisibilityGrid* visgrid);



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
void print_event(AEvent a);


    /*computes the distance from the event to the viewpoint. Note: all 3
       //events associate to a cell are considered at the same distance, from
       //the center of teh cell to the viewpoint */
double get_square_distance_from_viewpoint(const AEvent & a,
					  const Viewpoint & vp);

    /*sort the event list in distance order */
void sort_event_list_by_distance(AMI_STREAM < AEvent > **eventList,
				 Viewpoint vp);
void sort_check(AMI_STREAM < AEvent > *eventList, Viewpoint vp);


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
double calculate_angle(double eventX, double eventY, double viewpointX,
		       double viewpointY);

#endif
