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
 * COPYRIGHT: (C) 2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 ****************************************************************************/


#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

extern "C"
{
#include "grass/gis.h"
#include "grass/glocale.h"
}

#include "viewshed.h"
#include "visibility.h"
#include "eventlist.h"
#include "statusstructure.h"
#include "grass.h"


#define VIEWSHEDDEBUG if(0)
#define INMEMORY_DEBUG if(0)



/* ------------------------------------------------------------ */
/* return the memory usage (in bytes) of viewshed */
long long get_viewshed_memory_usage(GridHeader * hd)
{


    assert(hd);
    /* the output  visibility grid */
    long long totalcells = (long long)hd->nrows * (long long)hd->ncols;

    G_verbose_message(_("rows=%d, cols=%d, total = %lld"), hd->nrows, hd->ncols,
	   totalcells);
    long long gridMemUsage = totalcells * sizeof(float);

    G_debug(1, "grid usage=%lld", gridMemUsage);

    /* the event array */
    long long eventListMemUsage = totalcells * 3 * sizeof(AEvent);

    G_debug(1, "memory_usage: eventList=%lld", eventListMemUsage);

    /* the double array <data> that stores all the cells in the same row
       as the viewpoint */
    long long dataMemUsage = (long long)(hd->ncols * sizeof(double));

    G_debug(1, "viewshed memory usage: size AEvent=%dB, nevents=%lld, \
            total=%lld B (%d MB)", (int)sizeof(AEvent), totalcells * 3,
            gridMemUsage + eventListMemUsage + dataMemUsage,
	    (int)((gridMemUsage + eventListMemUsage + dataMemUsage) >> 20));

    return (gridMemUsage + eventListMemUsage + dataMemUsage);

}


/* ------------------------------------------------------------ */
void
print_viewshed_timings(Rtimer initEventTime,
		       Rtimer sortEventTime, Rtimer sweepTime)
{

    char timeused[1000];

    G_verbose_message(_("Sweep timings:"));
    rt_sprint_safe(timeused, initEventTime);
    G_verbose_message("Init events: %s", timeused);

    rt_sprint_safe(timeused, sortEventTime);
    G_verbose_message("Sort events: %s", timeused);

    rt_sprint_safe(timeused, sweepTime);
    G_verbose_message("Process events: %s", timeused);

    return;
}


/* ------------------------------------------------------------ */
static void print_statusnode(StatusNode sn)
{
    G_debug(3, "processing (row=%d, col=%d, dist=%f, grad=%f)",
	   sn.row, sn.col, sn.dist2vp, sn.gradient[1]);
    return;
}



/* ------------------------------------------------------------ */
/* allocates the eventlist array used by kreveled_in_memory; it is
   possible that the amount of memory required is more than that
   supported by the platform; for e.g. on a 32-bt platform cannot
   allocate more than 4GB. Try to detect this situation.  */
AEvent *allocate_eventlist(GridHeader * hd)
{

    AEvent *eventList;

    long long totalsize = hd->ncols * hd->nrows * 3;

    totalsize *= sizeof(AEvent);
    G_debug(1, "total size of eventlist is %lld B (%d MB);  ",
	   totalsize, (int)(totalsize >> 20));

    /* what's the size of size_t on this machine? */
    int sizet_size;

    sizet_size = (int)sizeof(size_t);
    G_debug(1, "size_t is %d B", sizet_size);

    if (sizet_size >= 8) {
	G_debug(1, "64-bit platform, great.");
    }
    else {
	/* this is the max value of size_t */
	long long maxsizet = ((long long)1 << (sizeof(size_t) * 8)) - 1;

	G_debug(1, "max size_t is %lld", maxsizet);

	/* checking whether allocating totalsize causes an overflow */
	if (totalsize > maxsizet) {
	    G_fatal_error(_("Running the program in-memory mode requires " \
	                    "memory beyond the capability of the platform. " \
			    "Use external mode, or a 64-bit platform."));
	}
    }

    G_debug(1, "allocating eventList...");
    eventList = (AEvent *) G_malloc(totalsize);

    assert(eventList);
    G_debug(1, "...ok");

    return eventList;
}




/*///////////////////////////////////////////////////////////
   ------------------------------------------------------------ run
   Viewshed's sweep algorithm on the grid stored in the given file, and
   with the given viewpoint.  Create a visibility grid and return
   it. The computation runs in memory, which means the input grid, the
   status structure and the output grid are stored in arrays in
   memory. 


   The output: A cell x in the visibility grid is recorded as follows:

   if it is NODATA, then x  is set to NODATA
   if it is invisible, then x is set to INVISIBLE
   if it is visible,  then x is set to the vertical angle wrt to viewpoint

 */
MemoryVisibilityGrid *viewshed_in_memory(char *inputfname, GridHeader * hd,
					 Viewpoint * vp,
					 ViewOptions viewOptions)
{

    assert(inputfname && hd && vp);
    G_verbose_message(_("Start sweeping."));

    /* ------------------------------ */
    /* create the visibility grid  */
    MemoryVisibilityGrid *visgrid;

    visgrid = create_inmem_visibilitygrid(*hd, *vp);
    /* set everything initially invisible */
    set_inmem_visibilitygrid(visgrid, INVISIBLE);
    assert(visgrid);
    G_debug(1, "visibility grid size:  %d x %d x %d B (%d MB)",
	       hd->nrows, hd->ncols, (int)sizeof(float),
	       (int)(((long long)(hd->nrows * hd->ncols *
				  sizeof(float))) >> 20));


    /* ------------------------------ */
    /* construct the event list corresponding to the given input file
       and viewpoint; this creates an array of all the cells on the
       same row as the viewpoint */
    surface_type **data;
    size_t nevents;

    Rtimer initEventTime;

    rt_start(initEventTime);

    AEvent *eventList = allocate_eventlist(hd);

    nevents = init_event_list_in_memory(eventList, inputfname, vp, hd,
					      viewOptions, &data, visgrid);

    assert(data);
    rt_stop(initEventTime);
    G_debug(1, "actual nb events is %lu", (long unsigned int)nevents);

    /* ------------------------------ */
    /*sort the events radially by angle */
    Rtimer sortEventTime;

    rt_start(sortEventTime);
    G_verbose_message(_("Sorting events..."));
    fflush(stdout);

    /*this is recursive and seg faults for large arrays
       //qsort(eventList, nevents, sizeof(AEvent), radial_compare_events);

       //this is too slow...
       //heapsort(eventList, nevents, sizeof(AEvent), radial_compare_events);

       //iostream quicksort */
    RadialCompare cmpObj;

    quicksort(eventList, nevents, cmpObj);
    G_verbose_message(_("Done."));
    fflush(stdout);
    rt_stop(sortEventTime);


    /* ------------------------------ */
    /*create the status structure */
    StatusList *status_struct = create_status_struct();

    /*Put cells that are initially on the sweepline into status structure */
    Rtimer sweepTime;
    StatusNode sn;

    rt_start(sweepTime);
    for (dimensionType i = vp->col + 1; i < hd->ncols; i++) {
	AEvent e;
	double ax, ay;

	sn.col = i;
	sn.row = vp->row;
	e.col = i;
	e.row = vp->row;
	e.elev[0] = data[0][i];
	e.elev[1] = data[1][i];
	e.elev[2] = data[2][i];
	
	if (!is_nodata(visgrid->grid->hd, data[1][i]) &&
	    !is_point_outside_max_dist(*vp, *hd, sn.row, sn.col,
				       viewOptions.maxDist)) {
	    /*calculate Distance to VP and Gradient, store them into sn */
	    /* need either 3 elevation values or 
	     * 3 gradients calculated from 3 elevation values */
	    /* need also 3 angles */
	    e.eventType = ENTERING_EVENT;
	    calculate_event_position(e, vp->row, vp->col, &ay, &ax);
	    sn.angle[0] = calculate_angle(ax, ay, vp->col, vp->row);
	    calculate_event_gradient(&sn, 0, ay, ax, e.elev[0], vp, *hd);

	    e.eventType = CENTER_EVENT;
	    calculate_event_position(e, vp->row, vp->col, &ay, &ax);
	    sn.angle[1] = calculate_angle(ax, ay, vp->col, vp->row);
	    calculate_dist_n_gradient(&sn, e.elev[1], vp, *hd);

	    e.eventType = EXITING_EVENT;
	    calculate_event_position(e, vp->row, vp->col, &ay, &ax);
	    sn.angle[2] = calculate_angle(ax, ay, vp->col, vp->row);
	    calculate_event_gradient(&sn, 2, ay, ax, e.elev[2], vp, *hd);
	    
	    assert(sn.angle[1] == 0);

	    if (sn.angle[0] > sn.angle[1])
		sn.angle[0] -= 2 * M_PI;

	    G_debug(2, "inserting: ");
	    print_statusnode(sn);
	    /*insert sn into the status structure */
	    insert_into_status_struct(sn, status_struct);
	}
    }
    G_free(data[0]);
    G_free(data);



    /* ------------------------------ */
    /*sweep the event list */
    long nvis = 0;		/*number of visible cells */
    AEvent *e;

    G_important_message(_("Computing visibility..."));
    G_percent(0, 100, 2);

    for (size_t i = 0; i < nevents; i++) {

	int perc = (int)(1000000 * i / nevents);
	if (perc > 0 && perc < 1000000)
	    G_percent(perc, 1000000, 1);

	/*get out one event at a time and process it according to its type */
	e = &(eventList[i]);

	sn.col = e->col;
	sn.row = e->row;
	//sn.elev = e->elev;

	/*calculate Distance to VP and Gradient */
	calculate_dist_n_gradient(&sn, e->elev[1] + vp->target_offset, vp, *hd);
	G_debug(3, "event: ");
	print_event(*e, 3);
	G_debug(3, "sn.dist=%f, sn.gradient=%f", sn.dist2vp, sn.gradient[1]);

	switch (e->eventType) {
	case ENTERING_EVENT:
	    double ax, ay;
	    /*insert node into structure */
	    G_debug(3, "..ENTER-EVENT: insert");

	    /* need either 3 elevation values or 
	     * 3 gradients calculated from 3 elevation values */
	    /* need also 3 angles */
	    calculate_event_position(*e, vp->row, vp->col, &ay, &ax);
	    //sn.angle[0] = calculate_angle(ax, ay, vp->col, vp->row);
	    sn.angle[0] = e->angle;
	    calculate_event_gradient(&sn, 0, ay, ax, e->elev[0], vp, *hd);

	    e->eventType = CENTER_EVENT;
	    calculate_event_position(*e, vp->row, vp->col, &ay, &ax);
	    sn.angle[1] = calculate_angle(ax, ay, vp->col, vp->row);
	    calculate_dist_n_gradient(&sn, e->elev[1], vp, *hd);

	    e->eventType = EXITING_EVENT;
	    calculate_event_position(*e, vp->row, vp->col, &ay, &ax);
	    sn.angle[2] = calculate_angle(ax, ay, vp->col, vp->row);
	    calculate_event_gradient(&sn, 2, ay, ax, e->elev[2], vp, *hd);

	    e->eventType = ENTERING_EVENT;

	    if (e->angle < M_PI) {
		if (sn.angle[0] > sn.angle[1])
		    sn.angle[0] -= 2 * M_PI;
	    }
	    else {
		if (sn.angle[0] > sn.angle[1]) {
		    sn.angle[1] += 2 * M_PI;
		    sn.angle[2] += 2 * M_PI;
		}
	    }

	    insert_into_status_struct(sn, status_struct);
	    break;

	case EXITING_EVENT:
	    /*delete node out of status structure */
	    G_debug(3, "..EXIT-EVENT: delete");
	    /* need only distance */
	    delete_from_status_struct(status_struct, sn.dist2vp);
	    break;

	case CENTER_EVENT:
	    G_debug(3, "..QUERY-EVENT: query");
	    /*calculate visibility */
	    double max;

	    /* consider current angle and gradient */
	    max =
		find_max_gradient_in_status_struct(status_struct, sn.dist2vp,
		                          e->angle, sn.gradient[1]);

	    /*the point is visible: store its vertical angle  */
	    if (max <= sn.gradient[1]) {
		float vert_angle = get_vertical_angle(*vp, sn, e->elev[1] + vp->target_offset,
		                                      viewOptions.doCurv);

		add_result_to_inmem_visibilitygrid(visgrid, sn.row, sn.col,
						   vert_angle);
		assert(vert_angle >= 0);
		/* when you write the visibility grid you assume that
		   visible values are positive */
		nvis++;
	    }
	    //else {
	    /* cell is invisible */
	    /*  the visibility grid is initialized all invisible */
	    //visgrid->grid->grid_data[sn.row][sn.col] = INVISIBLE;
	    //}
	    break;
	}
    }
    rt_stop(sweepTime);
    G_percent(1, 1, 1);

    G_verbose_message(_("Sweeping done."));
    G_verbose_message(_("Total cells %ld, visible cells %ld (%.1f percent)."),
	   (long)visgrid->grid->hd->nrows * visgrid->grid->hd->ncols,
	   nvis,
	   (float)((float)nvis * 100 /
		   (float)(visgrid->grid->hd->nrows *
			   visgrid->grid->hd->ncols)));

    print_viewshed_timings(initEventTime, sortEventTime, sweepTime);

    /*cleanup */
    G_free(eventList);

    return visgrid;
}








/*///////////////////////////////////////////////////////////
   ------------------------------------------------------------ 
   run Viewshed's algorithm on the grid stored in the given file, and
   with the given viewpoint.  Create a visibility grid and return it. It
   runs in external memory, i.e. the input grid and the outpt grid are
   stored as streams
 */

IOVisibilityGrid *viewshed_external(char *inputfname, GridHeader * hd,
				    Viewpoint * vp, ViewOptions viewOptions)
{

    assert(inputfname && hd && vp);
    G_message(_("Start sweeping."));


    /* ------------------------------ */
    /*initialize the visibility grid */
    IOVisibilityGrid *visgrid;

    visgrid = init_io_visibilitygrid(*hd, *vp);


    /* ------------------------------ */
    /* construct the event list corresponding to the give input file and
       viewpoint; this creates an array of all the cells on
       the same row as the viewpoint  */
    Rtimer initEventTime, sortEventTime, sweepTime;

    AMI_STREAM < AEvent > *eventList;
    surface_type **data;

    rt_start(initEventTime);

    eventList = init_event_list(inputfname, vp, hd, viewOptions,
				      &data, visgrid);

    assert(eventList && data);
    eventList->seek(0);
    rt_stop(initEventTime);
    /*printf("Event stream length: %lu\n", (unsigned long)eventList->stream_len()); */


    /* ------------------------------ */
    /*sort the events radially by angle */
    G_verbose_message(_("Sorting events..."));

    rt_start(sortEventTime);
    sort_event_list(&eventList);
    eventList->seek(0);		/*this does not seem to be ensured by sort?? */
    rt_stop(sortEventTime);


    /* ------------------------------ */
    /*create the status structure */
    StatusList *status_struct = create_status_struct();

    /* Put cells that are initially on the sweepline into status
       structure */
    StatusNode sn;

    G_message(_("Initialize sweepline..."));

    rt_start(sweepTime);
    for (dimensionType i = vp->col + 1; i < hd->ncols; i++) {
	AEvent e;
	double ax, ay;

	G_percent(i, hd->ncols, 2);

	sn.col = i;
	sn.row = vp->row;
	e.col = i;
	e.row = vp->row;
	e.elev[0] = data[0][i];
	e.elev[1] = data[1][i];
	e.elev[2] = data[2][i];
	if (!is_nodata(visgrid->hd, data[1][i]) &&
	    !is_point_outside_max_dist(*vp, *hd, sn.row, sn.col,
				       viewOptions.maxDist)) {
	    /*calculate Distance to VP and Gradient, store them into sn */
	    /* need either 3 elevation values or 
	     * 3 gradients calculated from 3 elevation values */
	    /* need also 3 angles */
	    e.eventType = ENTERING_EVENT;
	    calculate_event_position(e, vp->row, vp->col, &ay, &ax);
	    sn.angle[0] = calculate_angle(ax, ay, vp->col, vp->row);
	    calculate_event_gradient(&sn, 0, ay, ax, e.elev[0], vp, *hd);

	    e.eventType = CENTER_EVENT;
	    calculate_event_position(e, vp->row, vp->col, &ay, &ax);
	    sn.angle[1] = calculate_angle(ax, ay, vp->col, vp->row);
	    calculate_dist_n_gradient(&sn, e.elev[1], vp, *hd);

	    e.eventType = EXITING_EVENT;
	    calculate_event_position(e, vp->row, vp->col, &ay, &ax);
	    sn.angle[2] = calculate_angle(ax, ay, vp->col, vp->row);
	    calculate_event_gradient(&sn, 2, ay, ax, e.elev[2], vp, *hd);
	    
	    assert(sn.angle[1] == 0);

	    if (sn.angle[0] > sn.angle[1])
		sn.angle[0] -= 2 * M_PI;

	    G_debug(3, "inserting: ");
	    print_statusnode(sn);

	    /*insert sn into the status structure */
	    insert_into_status_struct(sn, status_struct);
	}
    }
    G_percent(hd->ncols, hd->ncols, 2);
    G_free(data[0]);
    G_free(data);


    /* ------------------------------ */
    /*sweep the event list */
    long nvis = 0;		/*number of visible cells */
    VisCell viscell;
    AEvent *e;
    AMI_err ae;
    off_t nbEvents = eventList->stream_len();

    /*printf("nbEvents = %ld\n", (long) nbEvents); */

    G_message(_("Determine visibility..."));
    G_percent(0, 100, 2);

    for (off_t i = 0; i < nbEvents; i++) {

	int perc = (int)(1000000 * i / nbEvents);
	if (perc > 0)
	    G_percent(perc, 1000000, 1);

	/*get out one event at a time and process it according to its type */
	ae = eventList->read_item(&e);
	assert(ae == AMI_ERROR_NO_ERROR);

	sn.col = e->col;
	sn.row = e->row;
	//sn.elev = e->elev;
	/*calculate Distance to VP and Gradient */
	calculate_dist_n_gradient(&sn, e->elev[1] + vp->target_offset, vp, *hd);

	G_debug(3, "next event: ");
	print_statusnode(sn);

	switch (e->eventType) {
	case ENTERING_EVENT:
	    double ax, ay;

	    /*insert node into structure */
	    /* need either 3 elevation values or 
	     * 3 gradients calculated from 3 elevation values */
	    /* need also 3 angles */
	    calculate_event_position(*e, vp->row, vp->col, &ay, &ax);
	    //sn.angle[0] = calculate_angle(ax, ay, vp->col, vp->row);
	    sn.angle[0] = e->angle;
	    calculate_event_gradient(&sn, 0, ay, ax, e->elev[0], vp, *hd);

	    e->eventType = CENTER_EVENT;
	    calculate_event_position(*e, vp->row, vp->col, &ay, &ax);
	    sn.angle[1] = calculate_angle(ax, ay, vp->col, vp->row);
	    calculate_dist_n_gradient(&sn, e->elev[1], vp, *hd);

	    e->eventType = EXITING_EVENT;
	    calculate_event_position(*e, vp->row, vp->col, &ay, &ax);
	    sn.angle[2] = calculate_angle(ax, ay, vp->col, vp->row);
	    calculate_event_gradient(&sn, 2, ay, ax, e->elev[2], vp, *hd);

	    e->eventType = ENTERING_EVENT;

	    if (e->angle < M_PI) {
		if (sn.angle[0] > sn.angle[1])
		    sn.angle[0] -= 2 * M_PI;
	    }
	    else {
		if (sn.angle[0] > sn.angle[1]) {
		    sn.angle[1] += 2 * M_PI;
		    sn.angle[2] += 2 * M_PI;
		}
	    }

	    G_debug(3, "..ENTER-EVENT: insert");

	    insert_into_status_struct(sn, status_struct);
	    break;

	case EXITING_EVENT:
	    /*delete node out of status structure */

	    G_debug(3, "..EXIT-EVENT: delete");

	    delete_from_status_struct(status_struct, sn.dist2vp);
	    break;

	case CENTER_EVENT:
	    G_debug(3, "..QUERY-EVENT: query");

	    /*calculate visibility */
	    viscell.row = sn.row;
	    viscell.col = sn.col;
	    double max;

	    max =
		find_max_gradient_in_status_struct(status_struct, sn.dist2vp,
		                          e->angle, sn.gradient[1]);

	    /*the point is visible */
	    if (max <= sn.gradient[1]) {
		viscell.angle =
		    get_vertical_angle(*vp, sn, e->elev[1] + vp->target_offset, viewOptions.doCurv);
		assert(viscell.angle >= 0);
		/* viscell.vis = VISIBLE; */
		add_result_to_io_visibilitygrid(visgrid, &viscell);
		nvis++;
	    }
	    else {
		/* else the cell is invisible; we do not write it to the
		   visibility stream because we only record visible cells, and
		   nodata cells; */
		/* viscell.vis = INVISIBLE; */
		/* add_result_to_io_visibilitygrid(visgrid, &viscell);  */
	    }
	    break;
	}
    }				/* for each event  */
    rt_stop(sweepTime);
    G_percent(1, 1, 1);

    G_message(_("Sweeping done."));
    G_verbose_message(_("Total cells %ld, visible cells %ld (%.1f percent)."),
	   (long)visgrid->hd->nrows * visgrid->hd->ncols,
	   nvis,
	   (float)((float)nvis * 100 /
		   (float)(visgrid->hd->nrows * visgrid->hd->ncols)));

    print_viewshed_timings(initEventTime, sortEventTime, sweepTime);

    /*cleanup */
    delete eventList;

    return visgrid;
}
