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
 ****************************************************************************/


#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "viewshed.h"
#include "visibility.h"
#include "eventlist.h"
#include "statusstructure.h"

#ifdef __GRASS
#include "grass.h"
#endif


#define VIEWSHEDDEBUG if(0)




/* ------------------------------------------------------------ */
/* return the memory usage (in bytes) of viewshed */
long long get_viewshed_memory_usage(GridHeader* hd) {


  assert(hd); 
  /* the output  visibility grid */
  long long totalcells = (long long)hd->nrows * (long long)hd->ncols; 
  VIEWSHEDDEBUG {printf("rows=%d, cols=%d, total = %lld\n", hd->nrows, hd->ncols, totalcells);}
  long long gridMemUsage =  totalcells * sizeof(float);
  VIEWSHEDDEBUG {printf("grid usage=%lld\n", gridMemUsage);}
  
  /* the event array */
  long long eventListMemUsage = totalcells * 3 * sizeof(AEvent);
  VIEWSHEDDEBUG {printf("memory_usage: eventList=%lld\n", eventListMemUsage);}
  
  /* the double array <data> that stores all the cells in the same row
	 as the viewpoint */
  long long dataMemUsage = (long long)(hd->ncols * sizeof(double));
  
  printf("get_viewshed_memory usage: size AEvent=%dB, nevents=%lld, \
 total=%lld B (%d MB)\n", 
		 (int)sizeof(AEvent),  totalcells*3, 
		 gridMemUsage + eventListMemUsage + dataMemUsage, 
		 (int)((gridMemUsage + eventListMemUsage + dataMemUsage)>>20));
  
  return (gridMemUsage + eventListMemUsage + dataMemUsage);
  
}


/* ------------------------------------------------------------ */
void
print_viewshed_timings(Rtimer initEventTime,
		      Rtimer sortEventTime, Rtimer sweepTime)
{

    char timeused[1000];

    printf("sweep timings:\n");
    rt_sprint_safe(timeused, initEventTime);
    printf("\t%30s", "init events: ");
    printf(timeused);
    printf("\n");

    rt_sprint_safe(timeused, sortEventTime);
    printf("\t%30s", "sort events: ");
    printf(timeused);
    printf("\n");

    rt_sprint_safe(timeused, sweepTime);
    printf("\t%30s", "process events: ");
    printf(timeused);
    printf("\n");
    fflush(stdout);
    return;
}


/* ------------------------------------------------------------ */
static void print_event(StatusNode sn)
{
    printf("processing (row=%d, col=%d, elev=%f, dist=%f, grad=%f)",
	   sn.row, sn.col, sn.elev, sn.dist2vp, sn.gradient);
    return;
}



/* ------------------------------------------------------------ */
/* allocates the eventlist array used by kreveled_in_memory */
AEvent*
allocate_eventlist(GridHeader* hd) {
  
  AEvent* eventList; 

  long long totalsize = hd->ncols * hd->nrows * 3; 
  totalsize *=  sizeof(AEvent); 
  printf("total size of eventlist is %lld B (%d MB);  ", 
		 totalsize, (int)(totalsize>>20));
  printf("size_t is %lu B\n", sizeof(size_t));
  
  /* checking whether allocating totalsize causes an overflow */
  long long maxsizet = ((long long)1<<(sizeof(size_t)*8)) -1; 
  printf("max size_t is %lld\n", maxsizet); 
  if (totalsize > maxsizet) {
	printf("running the program in-memory mode requires memory beyond the capability of the platform. Use external mode, or 64-bit platform.\n");
	exit(1); 
  }
  
  printf("allocating.."); 
#ifdef __GRASS__
  eventList =  (AEvent *) G_malloc(totalsize);
#else
  eventList = (AEvent *) malloc(totalsize*sizeof(char));
#endif
  assert(eventList);
  printf("..ok\n");

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
MemoryVisibilityGrid *viewshed_in_memory(char* inputfname, GridHeader * hd,
										 Viewpoint*vp,ViewOptions viewOptions){
  
    assert(inputfname && hd && vp);
    printf("Start sweeping.\n");
    fflush(stdout);

	/* ------------------------------ */
    /* create the visibility grid  */
    MemoryVisibilityGrid *visgrid; 
	visgrid = create_inmem_visibilitygrid(*hd, *vp);
	/* set everything initially invisible */
	set_inmem_visibilitygrid(visgrid, INVISIBLE); 
    assert(visgrid);
    VIEWSHEDDEBUG { 
	  printf("visibility grid size:  %d x %d x %d B (%d MB)\n",
			 hd->nrows, hd->ncols, (int)sizeof(float), 
			 (int)(((long long)(hd->nrows*hd->ncols* sizeof(float))) >> 20));
	}



	/* ------------------------------ */
    /* construct the event list corresponding to the given input file
       and viewpoint; this creates an array of all the cells on the
       same row as the viewpoint */
    double *data;
    size_t nevents;

    Rtimer initEventTime;
    rt_start(initEventTime);

	AEvent *eventList = allocate_eventlist(hd);
#ifdef __GRASS__
    nevents = grass_init_event_list_in_memory(eventList, inputfname, vp, hd, 
											  viewOptions, &data, visgrid);
#else
    nevents = init_event_list_in_memory(eventList, inputfname, vp, hd, 
										viewOptions, &data, visgrid);
#endif
    assert(data);
	rt_stop(initEventTime);
	printf("actual nb events is %lu\n", nevents);

	/* ------------------------------ */
    /*sort the events radially by angle */
    Rtimer sortEventTime;
    rt_start(sortEventTime);
    printf("sorting events..");
    fflush(stdout);

    /*this is recursive and seg faults for large arrays
	//qsort(eventList, nevents, sizeof(AEvent), radial_compare_events);
	
	//this is too slow...
	//heapsort(eventList, nevents, sizeof(AEvent), radial_compare_events);
	
	//iostream quicksort */
    RadialCompare cmpObj;
    quicksort(eventList, nevents, cmpObj);
    printf("done\n"); fflush(stdout);
    rt_stop(sortEventTime);
	



	/* ------------------------------ */
    /*create the status structure */
    StatusList *status_struct = create_status_struct();

    /*Put cells that are initially on the sweepline into status structure */
    Rtimer sweepTime;
    StatusNode sn;

    rt_start(sweepTime);
    for (dimensionType i = vp->col + 1; i < hd->ncols; i++) {
	  sn.col = i;
	  sn.row = vp->row;
	  sn.elev = data[i];
	  if (!is_nodata(visgrid->grid->hd, sn.elev) && 
		  !is_point_outside_max_dist(*vp, *hd, sn.row, sn.col, 
									 viewOptions.maxDist)) {
	    /*calculate Distance to VP and Gradient, store them into sn */
	    calculate_dist_n_gradient(&sn, vp);
	    VIEWSHEDDEBUG {
		  printf("inserting: ");
		  print_event(sn);
		  printf("\n");
	    }
	    /*insert sn into the status structure */
	    insert_into_status_struct(sn, status_struct);
	  }
    }
#ifdef __GRASS__
    G_free(data);
#else
    free(data);
#endif



	/* ------------------------------ */
    /*sweep the event list */
    long nvis = 0;		/*number of visible cells */
    AEvent *e;

    for (size_t i = 0; i < nevents; i++) {
	  
	  /*get out one event at a time and process it according to its type */
	  e = &(eventList[i]);
	  
	  sn.col = e->col;
	  sn.row = e->row;
	  sn.elev = e->elev;

	  /*calculate Distance to VP and Gradient */
	  calculate_dist_n_gradient(&sn, vp);
	  VIEWSHEDDEBUG {
	    printf("next event: ");
	    print_event(sn);
	  }
	  
	  switch (e->eventType) {
	  case ENTERING_EVENT:
	    /*insert node into structure */
	    VIEWSHEDDEBUG {
		  printf("..ENTER-EVENT: insert\n");
	    }
	    insert_into_status_struct(sn, status_struct);
	    break;
		
	  case EXITING_EVENT:
	    /*delete node out of status structure */
	    VIEWSHEDDEBUG {
		  printf("..EXIT-EVENT: delete\n");
	    }
	    delete_from_status_struct(status_struct, sn.dist2vp);
	    break;
		
	  case CENTER_EVENT:
	    VIEWSHEDDEBUG {
		  printf("..QUERY-EVENT: query\n");
	    }
	    /*calculate visibility */
	    double max;
		max=find_max_gradient_in_status_struct(status_struct, sn.dist2vp);
	    
	    /*the point is visible: store its vertical angle  */
	    if (max <= sn.gradient) {
		  add_result_to_inmem_visibilitygrid(visgrid, sn.row, sn.col, 
							  get_vertical_angle(*vp, sn, viewOptions.doCurv));
		  assert(get_vertical_angle(*vp, sn, viewOptions.doCurv) >= 0); 
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
	
    printf("Sweeping done.\n");
    printf("Total cells %ld, visible cells %ld (%.1f percent).\n",
		   (long)visgrid->grid->hd->nrows * visgrid->grid->hd->ncols,
		   nvis,
		   (float)((float)nvis * 100 /
				   (float)(visgrid->grid->hd->nrows *
			   visgrid->grid->hd->ncols)));

    print_viewshed_timings(initEventTime, sortEventTime, sweepTime);

    /*cleanup */
#ifdef __GRASS__
    G_free(eventList);
#else
    free(eventList);
#endif
	
    return visgrid;

}








/*///////////////////////////////////////////////////////////
   ------------------------------------------------------------ 
   run Viewshed's algorithm on the grid stored in the given file, and
   with the given viewpoint.  Create a visibility grid and return it. It
   runs in external memory, i.e. the input grid and the outpt grid are
   stored as streams
 */

IOVisibilityGrid *viewshed_external(char* inputfname, GridHeader * hd,
									Viewpoint* vp, ViewOptions viewOptions){

  assert(inputfname && hd && vp);
  printf("Start sweeping.\n");
  fflush(stdout);


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
  double *data;
  rt_start(initEventTime);
#ifdef __GRASS__
  eventList = grass_init_event_list(inputfname,vp, hd, viewOptions, 
									&data, visgrid);
#else
  eventList = init_event_list(inputfname, vp,hd,viewOptions,&data,visgrid);
#endif
  assert(eventList && data);
  eventList->seek(0);
  rt_stop(initEventTime);
  /*printf("Event stream length: %lu\n", (unsigned long)eventList->stream_len()); */
  
  
  /* ------------------------------ */
  /*sort the events radially by angle */
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
  rt_start(sweepTime);
  for (dimensionType i = vp->col + 1; i < hd->ncols; i++) {
	sn.col = i;
	sn.row = vp->row;
	sn.elev = data[i];
	if (!is_nodata(visgrid->hd, sn.elev)  && 
		!is_point_outside_max_dist(*vp, *hd, sn.row, sn.col, 
								   viewOptions.maxDist)) {
	  /*calculate Distance to VP and Gradient, store them into sn */
	  calculate_dist_n_gradient(&sn, vp);
	  VIEWSHEDDEBUG {
		printf("inserting: ");
		print_event(sn);
		printf("\n");
	  }
	  /*insert sn into the status structure */
	  insert_into_status_struct(sn, status_struct);
	}
  }
#ifdef __GRASS__
  G_free(data);
#else
  free(data);
#endif
  
  
  /* ------------------------------ */
    /*sweep the event list */
    long nvis = 0;		/*number of visible cells */
    VisCell viscell;
    AEvent *e;
    AMI_err ae;
    off_t nbEvents = eventList->stream_len();

    /*printf("nbEvents = %ld\n", (long) nbEvents); */

    for (off_t i = 0; i < nbEvents; i++) {

	/*get out one event at a time and process it according to its type */
	ae = eventList->read_item(&e);
	assert(ae == AMI_ERROR_NO_ERROR);
	
	sn.col = e->col;
	sn.row = e->row;
	sn.elev = e->elev;
	/*calculate Distance to VP and Gradient */
	calculate_dist_n_gradient(&sn, vp);
	VIEWSHEDDEBUG {
	  printf("next event: ");
	  print_event(sn);
	}
	
	switch (e->eventType) {
	case ENTERING_EVENT:
	  /*insert node into structure */
	  VIEWSHEDDEBUG {
		printf("..ENTER-EVENT: insert\n");
	  }
	  insert_into_status_struct(sn, status_struct);
	  break;
	  
	case EXITING_EVENT:
	  /*delete node out of status structure */
	  VIEWSHEDDEBUG {
		printf("..EXIT-EVENT: delete\n");
	  }
	  delete_from_status_struct(status_struct, sn.dist2vp);
	  break;
	  
	case CENTER_EVENT:
	  VIEWSHEDDEBUG {
		printf("..QUERY-EVENT: query\n");
	  }
	  /*calculate visibility */
	  viscell.row = sn.row;
	  viscell.col = sn.col;
	  double max;
	  max=find_max_gradient_in_status_struct(status_struct, sn.dist2vp);
	  
	  /*the point is visible */
	  if (max <= sn.gradient) {
		viscell.angle = get_vertical_angle(*vp, sn, viewOptions.doCurv);
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
	} /* for each event  */
    rt_stop(sweepTime);
	
    printf("Sweeping done.\n");
    printf("Total cells %ld, visible cells %ld (%.1f percent).\n",
		   (long)visgrid->hd->nrows * visgrid->hd->ncols,
		   nvis,
		   (float)((float)nvis * 100 /
				   (float)(visgrid->hd->nrows * visgrid->hd->ncols)));
	
    print_viewshed_timings(initEventTime, sortEventTime, sweepTime);
	
    /*cleanup */
    delete eventList;
	
    return visgrid;
}
