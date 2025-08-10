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
 * terrain. The terrain is NOT viewed as a tessellation of flat cells,
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

#ifndef __DISTRIBUTE_H
#define __DISTRIBUTE_H

#include "visibility.h"
#include "grid.h"
#include "eventlist.h"

/* distribution sweep: write results to visgrid.
 */
IOVisibilityGrid *distribute_and_sweep(char *inputfname, GridHeader *hd,
                                       Viewpoint *vp,
                                       const ViewOptions &viewOptions);

/* distribute recursively the events and write results to
   visgrid. eventList is a list of events sorted by distance that fall
   within angle boundaries start_angle and end_angle;  enterBndEvents
   is a stream that contains all the ENTER events that are not in this
   sector, but their corresponding Q or X events are in this sector.

   when problem is small enough, solve it in memory and write results
   to visgrid.

   invariant: distribute_sector deletes eventList and enterBndEvents

   returns the number of visible cells.
 */
unsigned long distribute_sector(AMI_STREAM<AEvent> *eventList,
                                AMI_STREAM<AEvent> *enterBndEvents,
                                double start_angle, double end_angle,
                                IOVisibilityGrid *visgrid, Viewpoint *vp,
                                GridHeader *hd, const ViewOptions &viewOptions);

/* bndEvents is a stream of events that cross into the sector's
   (first) boundary; they must be distributed to the boundary streams
   of the sub-sectors of this sector. Note: the boundary streams of
   the sub-sectors may not be empty; as a result, events get appended
   at the end, and they will not be sorted by distance from the
   vp.
 */
void distribute_bnd_events(AMI_STREAM<AEvent> *bndEvents,
                           AMI_STREAM<AEvent> *SectorBnd, int nsect,
                           Viewpoint *vp, double start_angle, double end_angle,
                           double *high, long *insert, long *drop);

/* same as above, but does it inemory. it is called when sector fits
   in memory. eventList is the list of events in increasing order of
   distance from the viewpoint; enterBndEvents is the list of ENTER
   events that are outside the sector, whose corresponding EXIT events
   are inside the sector.  start_angle and end_angle are the
   boundaries of the sector, and visgrid is the grid to which the
   visible/invisible cells must be written. The sector is solved by
   switching to radial sweep.  Returns the number of visible cells. */
unsigned long solve_in_memory(AMI_STREAM<AEvent> *eventList,
                              AMI_STREAM<AEvent> *enterBndEvents,
                              double start_angle, double end_angle,
                              IOVisibilityGrid *visgrid, GridHeader *hd,
                              Viewpoint *vp, const ViewOptions &viewOptions);

/*returns 1 if enter angle is within epsilon from boundary angle */
int is_almost_on_boundary(double angle, double boundary_angle);

/* returns 1 if angle is within epsilon the boundaries of sector s */
int is_almost_on_boundary(double angle, int s, double start_angle,
                          double end_angle, int nsect);

/* computes the number of sector for the distribution sweep;
   technically M/B */
int compute_n_sectors();

/* return 1 is s is inside sector; that is, if it is not -1 */
int is_inside(int s, int nsect);

/* compute the sector that contains this angle; there are nsect
   sectors that span the angle interval [sstartAngle, sendAngle] */
int get_event_sector(double angle, double sstartAngle, double sendAngle,
                     int nsect);

/* insert event in this sector */
void insert_event_in_sector(AMI_STREAM<AEvent> *str, AEvent *e);

/* insert event e into sector if it is not occluded by high_s */
void insert_event_in_sector(AEvent *e, int s, AMI_STREAM<AEvent> *str,
                            double high_s, Viewpoint *vp, long *insert,
                            long *drop);

/**********************************************************************
 insert event e into sector, no occlusion check */
void insert_event_in_sector_no_drop(AEvent *e, int s, AMI_STREAM<AEvent> *str,
                                    long *insert);

/* returns 1 if the center of event is occluded by the gradient, which
   is assumed to be in line with the event  */
int is_center_gradient_occluded(AEvent *e, double gradient, Viewpoint *vp);
/* called when dropping an event e, high is the highest gradient value
   in its sector */
void print_dropped(AEvent *e, Viewpoint *vp, double high);

/* prints how many events were inserted and dropped in each sector */
void print_sector_stats(off_t nevents, int nsect, double *high, long *total,
                        long *insert, long *drop, AMI_STREAM<AEvent> *sector,
                        AMI_STREAM<AEvent> *bndSector, long *bndInsert,
                        long longEvents, double start_angle, double end_angle);

/* the event e spans sectors from start_s to end_s; Action: update
   high[] for each spanned sector.
 */
void process_long_cell(int start_s, int end_s, int nsect, Viewpoint *vp,
                       AEvent *e, double *high);

/* return the start angle of the i-th sector. Assuming that
   [start..end] is split into nsectors */
double get_sector_start(int i, double start_angle, double end_angle, int nsect);

/* return the start angle of the i-th sector. Assuming that
   [start..end] is split into nsectors */
double get_sector_end(int i, double start_angle, double end_angle, int nsect);

/* returns true if the event is inside the given sector */
int is_inside(AEvent *e, double start_angle, double end_angle);

/* returns true if this angle is inside the given sector */
int is_inside(double angle, double start_angle, double end_angle);

#endif
