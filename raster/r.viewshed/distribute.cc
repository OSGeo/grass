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


#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifdef __GRASS__
extern "C"
{
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/glocale.h>
}

/* include IOSTREAM header */
#include <grass/iostream/ami.h>

#else 
/* if GRASS is not defined */
#include <ami.h>
#endif



/*note: MAX_STREAM_OPEN defined in IOStrea/include/ami_stream.h, which
   is included by ami.h  */

#include "distribute.h"
#include "viewshed.h"
#include "visibility.h"
#include "eventlist.h"
#include "statusstructure.h"
#include "print_message.h"
#ifdef __GRASS__
#include "grass.h"
#endif


#define DISTRIBDEBUG if(0)
#define SOLVEINMEMDEBUG if(0)
#define DEBUGINIT if(0)
#define PRINTWARNING if(0)
#define BND_DEBUG if(0)


#define ANGLE_FACTOR 1
#define EPSILON .00000001
#define PRINT_DISTRIBUTE if(0)



////////////////////////////////////////////////////////////////////////
/* distribution sweep: write results to visgrid.
 */
IOVisibilityGrid *distribute_and_sweep(char* inputfname,
									   GridHeader * hd,
									   Viewpoint *vp,
									   ViewOptions viewOptions) {
  
  assert(inputfname && hd && vp);
  print_message("Start distributed sweeping.\n");
  fflush(stdout);
  
  /* ------------------------------ */
  /*initialize the visibility grid */
  IOVisibilityGrid *visgrid;
  visgrid = init_io_visibilitygrid(*hd, *vp);
  printf("distribute_and_sweep: visgrid=%s\n", visgrid->visStr->name());
  
  
  /* ------------------------------ */
  /*construct event list corresp to the input file and viewpoint */
  Rtimer initEventTime;
  rt_start(initEventTime);
  AMI_STREAM < AEvent > *eventList;
#ifdef __GRASS__
  eventList = grass_init_event_list(inputfname, vp, hd, 
									viewOptions, NULL, visgrid);
#else
  eventList = init_event_list(inputfname, vp, hd, viewOptions,NULL, visgrid);
#endif
  assert(eventList);
  eventList->seek(0);
  rt_stop(initEventTime);
  printf("distribute_and_sweep: eventlist=%s\n", eventList->sprint());
  
  
  /* ------------------------------ */
  /*sort the events concentrically */
  Rtimer sortEventTime;
  rt_start(sortEventTime);
  PRINT_DISTRIBUTE {
	print_message("sorting events by distance from viewpoint..");
	fflush(stdout);
  }

  sort_event_list_by_distance(&eventList, *vp);
  PRINT_DISTRIBUTE {
	print_message("..sorting done.\n");
	fflush(stdout);
  }

  /* debugging */
  /*sortCheck(eventList, *vp); */
  eventList->seek(0);		/*this does not seem to be ensured by sort */
  rt_stop(sortEventTime);
  printf("distribute_and_sweep: eventlist=%s\n", eventList->sprint());
  
  
  
  /* ------------------------------ */
  /* start distribution */
  long nvis;	 /*number of cells visible. Returned by distribute_sector */
  
  Rtimer sweepTime;
  rt_start(sweepTime);
  /*distribute recursively the events and write results to visgrid.
	invariant: distribute_sector deletes its eventlist */
  nvis = distribute_sector(eventList, NULL, 0, ANGLE_FACTOR * 2 * M_PI,
						   visgrid, vp, viewOptions);
  rt_stop(sweepTime);
  
  
  /* ------------------------------ */
  /*cleanup */
  print_message("Distribution sweeping done.\n");
  fflush(stdout);
  
  printf("Total cells %ld, visible cells %ld (%.1f percent).\n",
		 (long)visgrid->hd->nrows * visgrid->hd->ncols,
		 nvis,
		 (float)((float)nvis * 100 /
				 (float)(visgrid->hd->nrows * visgrid->hd->ncols)));
  
  print_viewshed_timings(initEventTime, sortEventTime, sweepTime);
  
  return visgrid;
}






//***********************************************************************
/* distribute recursively the events and write results to
   visgrid. eventList is a list of events sorted by distance that fall
   within angle boundaries start_angle and end_angle; enterBndEvents
   is a stream that contains all the ENTER events that are not in this
   sector, but their corresponding Q or X events are in this sector.  

   When problem is small enough, solve it in memory and write results to
   visgrid.

   invariant: distribute_sector deletes eventList and enterBndEvents
 */
unsigned long distribute_sector(AMI_STREAM < AEvent > *eventList,
								AMI_STREAM < AEvent > *enterBndEvents,
								double start_angle, double end_angle,
								IOVisibilityGrid * visgrid, Viewpoint * vp, 
								ViewOptions viewOptions) {
  

    assert(eventList && visgrid && vp);
    /*enterBndEvents may be NULL first time */

    PRINT_DISTRIBUTE printf("***DISTRIBUTE sector [%.4f, %.4f]***\n",
			    start_angle, end_angle);
    printf("initial_gradient: %lf\n", SMALLEST_GRADIENT);
    printf("eventlist: %s\n", eventList->sprint());
    if (enterBndEvents)
	  printf("BndEvents: %s\n", enterBndEvents->sprint());
    PRINT_DISTRIBUTE LOG_avail_memo();
	
    unsigned long nvis = 0;
	
	/*******************************************************
	   BASE CASE
	*******************************************************/
    if (eventList->stream_len() * sizeof(AEvent) <
		MM_manager.memory_available()) {
	  if (enterBndEvents) {
	    nvis += solve_in_memory(eventList, enterBndEvents,
								start_angle,end_angle,visgrid,vp,viewOptions);
	    return nvis;
	  }
	  else {
	    /*we are here if the problem fits in memory, and
		//enterBNdEvents==NULL---this only happens the very first time;
		//technically at this point we should do one pass though the
		//data and collect the events that cross the 1st and 4th
		//quadrant; instead we force recursion, nect= 2 */
	  }
    }
	
    /*else, must recurse */
    PRINT_DISTRIBUTE print_message("in EXTERNAL memory\n");
	
    /*compute number of sectors */
    int nsect = compute_n_sectors();
	assert(nsect > 1);
	
    /*an array of streams, one for each sector; sector[i] will keep all
	  the cells that are completely inside sector i */
    AMI_STREAM < AEvent > *sector = new AMI_STREAM < AEvent >[nsect];
	
    /*the array of gradient values, one for each sector; the gradient is
	  the gradient of the center of a cell that spans the sector
	  completely */
    double *high = new double[nsect];
	
    for (int i = 0; i < nsect; i++)
	  high[i] = SMALLEST_GRADIENT;
	
    /*an array of streams, one for each stream boundary; sectorBnd[0]
	  will keep all the cells crossing into sector 0 from below; and so on. */
    AMI_STREAM < AEvent > *sectorBnd = new AMI_STREAM < AEvent >[nsect];
	
    /*keeps stats for each sector */
    long *total = new long[nsect];
    long *insert = new long[nsect];
    long *drop = new long[nsect];
    long *bndInsert = new long[nsect];
    long *bndDrop = new long[nsect];
	
    for (int i = 0; i < nsect; i++)
	  total[i] = insert[i] = drop[i] = bndInsert[i] = bndDrop[i] = 0;
    long longEvents = 0;
	
  /*******************************************************
  CONCENTRIC SWEEP
  *******************************************************/
    AEvent *e;
    AMI_err ae;
    double exit_angle, enter_angle;
    int exit_s, s, enter_s;
    long boundaryEvents = 0;
    off_t nbEvents = eventList->stream_len();
	
    eventList->seek(0);
    for (off_t i = 0; i < nbEvents; i++) {
	  
	  /*get out one event at a time and process it according to its type */
	  ae = eventList->read_item(&e);
	  assert(ae == AMI_ERROR_NO_ERROR);
	  assert(is_inside(e, start_angle, end_angle));
	  
	  /*compute  its sector  */
	  s = get_event_sector(e->angle, start_angle, end_angle, nsect);
	  /*detect boundary cases ==> precision issues */
	  if (is_almost_on_boundary(e->angle, s, start_angle, end_angle, nsect)) {
	    double ssize = (end_angle - start_angle) / nsect;
		
	    boundaryEvents++;
	    PRINTWARNING {
		  print_message("WARNING!event ");
		  print_event(*e);
		  print_message("CLOSE TO BOUNDARY\n");
		  printf("angle=%f close to  sector boundaries=[%f, %f]\n",
				 e->angle, s * ssize, (s + 1) * ssize);
	    }
	  }
	  
	  DISTRIBDEBUG printf("event %7lu: ", (unsigned long)i);
	  DISTRIBDEBUG print_event(*e);
	  DISTRIBDEBUG printf("d=%8.1f, ",
						  get_square_distance_from_viewpoint(*e, *vp));
	  DISTRIBDEBUG printf("s=%3d ", s);
	  
	  assert(is_inside(s, nsect));
	  total[s]++;
	  
	  /*insert event in sector if not occluded */
	  insert_event_in_sector(e, s, &sector[s], high[s], vp, insert, drop);
	  
	  switch (e->eventType) {
	  case CENTER_EVENT:
	    break;
		
	  case ENTERING_EVENT:
	    /*find its corresponding exit event and its sector */
	    exit_angle = calculate_exit_angle(e->row, e->col, vp);
	    exit_s =
		  get_event_sector(exit_angle, start_angle, end_angle, nsect);
	    DISTRIBDEBUG
		  printf(" ENTER (a=%.2f,s=%3d)---> EXIT (a=%.2f,s=%3d) ",
				 e->angle, s, exit_angle, exit_s);
	    /*note: exit_s can be -1 (outside) */
	    if (exit_s == s) {
		  /*short event, fit in sector s */
		  
	    }
	    else if (exit_s == (s + 1) % nsect || (exit_s + 1) % nsect == s) {
		  /*semi-short event; insert in sector s, and in sector boundary s+1
			NOTE: to avoid precision issues, the events are inserted
			when processing the EXIT_EVENT
			insertEventInSector(e, (s+1)%nsect, &sectorBnd[(s+1)%nsect],
			high[(s+1)%nsect], vp,bndInsert, bndDrop); */
		  
	    }
	    else {
		  /*long event; insert in sector s, and in sector boundary exit_s */
		  process_long_cell(s, exit_s, nsect, vp, e, high);
		  longEvents++;
		  /*insertEventInSector(e, exit_s, &sectorBnd[exit_s], high[exit_s],
		  //               vp, bndInsert, bndDrop); */
	    }
	    break;

	  case EXITING_EVENT:
	    /*find its corresponding enter event and its sector */
	    enter_angle = calculate_enter_angle(e->row, e->col, vp);
	    enter_s =
		  get_event_sector(enter_angle, start_angle, end_angle, nsect);
	    DISTRIBDEBUG
		  printf("  EXIT (a=%.2f,s=%3d)--->ENTER (a=%.2f,s=%3d) ",
				 e->angle, s, enter_angle, enter_s);
		
	    /*don't need to check spanned sectors because it is done on its
		//ENTER event; actually...you do, because its enter event may
		//not be in this sector==> enter_s = -1 (outside) */
	    if (enter_s == s) {
		  /*short event, fit in sector */
	    }
	    else if (enter_s == (s + 1) % nsect || (enter_s + 1) % nsect == s) {
		  /*semi-short event 
		  //the corresponding ENTER event must insert itself in sectorBnd[s] */
		  e->eventType = ENTERING_EVENT;
		  BND_DEBUG {
		    print_message("BND event ");
		    print_event(*e);
		    printf("in bndSector %d\n", s);
		    fflush(stdout);
		  }
		  insert_event_in_sector(e, s, &sectorBnd[s], high[s],
								 vp, bndInsert, bndDrop);
		  
	    }
	    else {
		  /*long event */
		  process_long_cell(enter_s, s, nsect, vp, e, high);
		  longEvents++;
		  /*the corresponding ENTER event must insert itself in sectorBnd[s] */
		  e->eventType = ENTERING_EVENT;
		  BND_DEBUG {
		    print_message("BND event ");
		    print_event(*e);
		    printf("in bndSector %d\n", s);
		    fflush(stdout);
		  }
		  insert_event_in_sector(e, s, &sectorBnd[s], high[s],
								 vp, bndInsert, bndDrop);
	    }
	    break;
		
	  }			/*switch event-type */
	  
	  DISTRIBDEBUG print_message("\n");
    }				/*for event i */
	
	
    /*distribute the enterBnd events to the boundary streams of the
	//sectors; note: the boundary streams are not sorted by distance. */
    if (enterBndEvents)
	  distribute_bnd_events(enterBndEvents, sectorBnd, nsect, vp,
							start_angle, end_angle, high, bndInsert,
							bndDrop);
	
    /*sanity checks */
    PRINT_DISTRIBUTE printf("boundary events in distribution: %ld\n",
							boundaryEvents);
    print_sector_stats(nbEvents, nsect, high, total, insert, drop, sector,
					   sectorBnd, bndInsert,
					   longEvents, start_angle, end_angle);
    /*cleanup after sector stats */
    delete[]total;
    delete[]insert;
    delete[]drop;
    delete[]high;
    delete[]bndInsert;
    delete[]bndDrop;
	
    /*we finished processing this sector, delete the event list */
    delete eventList;

    if (enterBndEvents)
	  delete enterBndEvents;
	
    /*save stream names of new sectors */
#ifdef __GRASS__
    char **sectorName = (char **)G_malloc(nsect * sizeof(char *));
    char **sectorBndName = (char **)G_malloc(nsect * sizeof(char *));
#else
    char **sectorName = (char **)malloc(nsect * sizeof(char *));
    char **sectorBndName = (char **)malloc(nsect * sizeof(char *));
#endif
    assert(sectorName && sectorBndName);
    for (int i = 0; i < nsect; i++) {
	sector[i].name(&sectorName[i]);
	PRINT_DISTRIBUTE printf("saving stream %d: %s\t", i, sectorName[i]);
	
	sector[i].persist(PERSIST_PERSISTENT);
	sectorBnd[i].name(&sectorBndName[i]);
	PRINT_DISTRIBUTE printf("saving BndStr %d: %s\n", i,
							sectorBndName[i]);
	sectorBnd[i].persist(PERSIST_PERSISTENT);
    }
	
    /*delete [] sector; 
	//does this call delete on every single stream? 
	//for (int i=0; i< nsect; i++ ) delete sector[i]; */
    delete[]sector;
    delete[]sectorBnd;
    /*LOG_avail_memo(); */
	

    /*solve recursively each sector */
    for (int i = 0; i < nsect; i++) {
	  
	  /*recover stream */
	  PRINT_DISTRIBUTE printf("\nopening sector stream %s ", sectorName[i]);
	  
	  AMI_STREAM < AEvent > *str =
	    new AMI_STREAM < AEvent > (sectorName[i]);
	  assert(str);
	  PRINT_DISTRIBUTE printf(" len=%lu\n",
							  (unsigned long)str->stream_len());
	  /*recover boundary stream */
	  PRINT_DISTRIBUTE printf("opening boundary sector stream %s ",
							  sectorBndName[i]);
	  AMI_STREAM < AEvent > *bndStr =
	    new AMI_STREAM < AEvent > (sectorBndName[i]);
	  assert(str);
	  PRINT_DISTRIBUTE printf(" len=%lu\n",
							  (unsigned long)bndStr->stream_len());
	  
	  
	  nvis += distribute_sector(str, bndStr,
								start_angle+i*((end_angle-start_angle)/nsect),
								start_angle+(i+1)*((end_angle-start_angle)/nsect),
								visgrid, vp, viewOptions);
    }

    /*cleanup */
#ifdef __GRASS__
    G_free(sectorName);
    G_free(sectorBndName);
#else
    free(sectorName);
    free(sectorBndName);
#endif
	
    PRINT_DISTRIBUTE printf("Distribute sector [ %.4f, %.4f] done.\n",
							start_angle, end_angle);
	
    return nvis;
}




/***********************************************************************
 enterBndEvents is a stream of events that cross into the sector's
   (first) boundary; they must be distributed to the boundary streams
   of the sub-sectors of this sector. Note: the boundary streams of
   the sub-sectors may not be empty; as a result, events get appended
   at the end, and they will not be sorted by distance from the
   vp.  
 */
void
distribute_bnd_events(AMI_STREAM < AEvent > *bndEvents,
		      AMI_STREAM < AEvent > *sectorBnd, int nsect,
		      Viewpoint * vp, double start_angle, double end_angle,
		      double *high, long *insert, long *drop)
{

    PRINT_DISTRIBUTE printf("Distribute boundary of sector [ %.4f, %.4f] ",
							start_angle, end_angle);
    assert(bndEvents && sectorBnd && vp && high && insert && drop);
    AEvent *e;
    AMI_err ae;
    double exit_angle;
    int exit_s;
    off_t nbEvents = bndEvents->stream_len();
	
    bndEvents->seek(0);
    for (off_t i = 0; i < nbEvents; i++) {
	  
	  /*get out one event at a time */
	  ae = bndEvents->read_item(&e);
	  assert(ae == AMI_ERROR_NO_ERROR);
	  /*must be outside, but better not check to avoid precision issues 
	  //assert(!is_inside(e, start_angle, end_angle)); 
	  
	  //each event must be an ENTER event that falls in a diff sector
	  //than its EXIT */
	  assert(e->eventType == ENTERING_EVENT);
	  
	  /*find its corresponding exit event and its sector */
	exit_angle = calculate_exit_angle(e->row, e->col, vp);
	exit_s = get_event_sector(exit_angle, start_angle, end_angle, nsect);
	
	/*exit_s cannot be outside sector; though we have to be careful
	//with precision */
	assert(is_inside(exit_s, nsect));
	
	/*insert this event in the boundary stream of this sub-sector */
	insert_event_in_sector(e, exit_s, &sectorBnd[exit_s], high[exit_s],
						   vp, insert, drop);
	
    }				/*for i */
	
    PRINT_DISTRIBUTE
	  printf("Distribute boundary of sector [ %.4f, %.4f] done.\n",
			 start_angle, end_angle);
	
    return;
}




//***********************************************************************
/* Solves a segment it inemory. it is called by distribute() when
   sector fits in memory.  eventList is the list of events in
   increasing order of distance from the viewpoint; enterBndEvents is
   the list of ENTER events that are outside the sector, whose
   corresponding EXIT events are inside the sector.  start_angle and
   end_angle are the boundaries of the sector, and visgrid is the grid
   to which the visible/invisible cells must be written. The sector is
   solved by switching to radial sweep.  */
unsigned long solve_in_memory(AMI_STREAM < AEvent > *eventList,
			      AMI_STREAM < AEvent > *enterBndEvents,
			      double start_angle,
			      double end_angle, IOVisibilityGrid * visgrid,
			      Viewpoint * vp, ViewOptions viewOptions)
{

    assert(eventList && visgrid && vp);
    PRINT_DISTRIBUTE print_message("solve INTERNAL memory\n");

    unsigned long nvis = 0;	/*number of visible cells */

    printf("solve_in_memory: eventlist: %s\n", eventList->sprint());
    if (enterBndEvents)
	  printf("BndEvents: %s\n", enterBndEvents->sprint());
	
    if (eventList->stream_len() == 0) {
	  delete eventList;
	  if (enterBndEvents) delete enterBndEvents;
	  return nvis;
    }

    /*sort the events radially */
    sort_event_list(&eventList);
	
    /*create the status structure */
    StatusList *status_struct = create_status_struct();
	
    /*initialize status structure with all ENTER events whose EXIT
       //events is inside the sector */
    AEvent *e;
    AMI_err ae;
    StatusNode sn;
    int inevents = 0;
	
    /* 
       eventList->seek(0);
       double enter_angle;
       ae = eventList->read_item(&e); 
       while (ae == AMI_ERROR_NO_ERROR) {
       if (e->eventType == EXITING_EVENT) { 
       enter_angle = calculateEnterAngle(e->row, e->col, vp); 
       //to get around precision problems, insert cell in active
       //structure when close to the boundary --- these may cause a
       //double insert, but subsequent ENTER events close to the
       //boundary will be careful not to insert
       if (!is_inside(enter_angle, start_angle, end_angle) || 
       is_almost_on_boundary(enter_angle, start_angle))  {
       DEBUGINIT {printf("inserting "); print_event(*e); printf("\n");}
       //this must span the first boundary of this sector; insert it
       //in status structure
       sn.col = e->col; 
       sn.row = e->row; 
       sn.elev = e->elev; 
       calculateDistNGradient(&sn, vp); 
       insertIntoStatusStruct(sn, status_struct);
       inevents++;
       }
       }
       ae = eventList->read_item(&e); 
       }
       assert(ae == AMI_ERROR_END_OF_STREAM); 
     */
    if (enterBndEvents) {
	  enterBndEvents->seek(0);
	  inevents = enterBndEvents->stream_len();
	  for (off_t i = 0; i < inevents; i++) {
	    ae = enterBndEvents->read_item(&e);
	    assert(ae == AMI_ERROR_NO_ERROR);
	    DEBUGINIT {
		  print_message("INMEM init: initializing boundary ");
		  print_event(*e);
		  print_message("\n");
	    }
	    /*this must span the first boundary of this sector; insert it
		//in status structure */
	    sn.col = e->col;
	    sn.row = e->row;
	    sn.elev = e->elev;
	    calculate_dist_n_gradient(&sn, vp);
	    insert_into_status_struct(sn, status_struct);
	  }
    }
    PRINT_DISTRIBUTE {
	  printf("initialized active structure with %d events\n", inevents);
	  fflush(stdout);
    }
	
	
    /*sweep the event list */
    VisCell viscell;
    off_t nbEvents = eventList->stream_len();
	
    /*printf("nbEvents = %ld\n", (long) nbEvents); */
    eventList->seek(0);
    for (off_t i = 0; i < nbEvents; i++) {
	  
	  /*get out one event at a time and process it according to its type */
	  ae = eventList->read_item(&e);
	  assert(ae == AMI_ERROR_NO_ERROR);
	  SOLVEINMEMDEBUG {
	    print_message("INMEM sweep: next event: ");
	    print_event(*e);
	  }
	  
	  sn.col = e->col;
	  sn.row = e->row;
	  sn.elev = e->elev;
	  /*calculate Distance to VP and Gradient */
	  calculate_dist_n_gradient(&sn, vp);
	  
	  switch (e->eventType) {
	  case ENTERING_EVENT:
	    /*insert node into structure */
	    SOLVEINMEMDEBUG {
		  print_message("..ENTER-EVENT: insert\n");
	    }
	    /*don't insert if its close to the boundary---the segment was
		//already inserted in initialization above
		//if (!is_almost_on_boundary(e->angle, start_angle)) 
		//insertIntoStatusStruct(sn,status_struct); */
	    insert_into_status_struct(sn, status_struct);
	    break;
		
	  case EXITING_EVENT:
	    /*delete node out of status structure */
	    SOLVEINMEMDEBUG {
		  print_message("..EXIT-EVENT: delete\n");
		  /*find its corresponding enter event and its sector */
		  double enter_angle =
		    calculate_enter_angle(e->row, e->col, vp);
		  printf("  EXIT (a=%f)--->ENTER (a=%f) ", e->angle,
				 enter_angle);
	    }
	    delete_from_status_struct(status_struct, sn.dist2vp);
	    break;
		
	  case CENTER_EVENT:
	    SOLVEINMEMDEBUG {
		  print_message("..QUERY-EVENT: query\n");
	    }
	    /*calculate visibility
		  
		//note: if there is nothing in the status structure, it means
		//there is no prior event to occlude it, so this cell is
		//VISIBLE. this is taken care of in the status structure --- if
		//a query event comes when the structure is empty, the query
		//returns minimum gradient */
	    double max;
		
	    max = find_max_gradient_in_status_struct(status_struct, sn.dist2vp);
		
	    viscell.row = sn.row;
	    viscell.col = sn.col;
		
		if (max <= sn.gradient) {
		  /*the point is visible */
		  viscell.angle = get_vertical_angle(*vp, sn, viewOptions.doCurv);
		  assert(viscell.angle >0);
	      /* viscell.vis = VISIBLE; */
		  add_result_to_io_visibilitygrid(visgrid, &viscell);
	      /*make sure nvis is correct*/
	      nvis++;
	    }
		else {
		/* else the point is invisible; we do not write it to the
	    //visibility stream, because we only record visible and nodata
	    //values to the stream */
		  /* viscell.vis = INVISIBLE; */
		  /* add_result_to_io_visibilitygrid(visgrid, &viscell); */
		}
	    break;
	  }
    } /* for each event */


    PRINT_DISTRIBUTE print_message("in memory sweeping done.\n");

    PRINT_DISTRIBUTE
	printf("Total cells %lu, visible cells %lu (%.1f percent).\n",
	       (unsigned long)eventList->stream_len(), nvis,
	       (float)((float)nvis * 100 / (float)(eventList->stream_len())));

    /*cleanup */
    delete_status_structure(status_struct);



    /*invariant: must delete its eventList */
    delete eventList;

    if (enterBndEvents)
	delete enterBndEvents;

    return nvis;
}



/***********************************************************************
 //returns 1 if enter angle is within epsilon from boundary angle*/
int is_almost_on_boundary(double angle, double boundary_angle)
{
    /*printf("is_almost_on_boundary: %f (%f) %f\n", angle, angle-2*M_PI, boundary_angle); */
    return (fabs(angle - boundary_angle) < EPSILON) ||
	(fabs(angle - 2 * M_PI - boundary_angle) < EPSILON);
}


/***********************************************************************
 // returns 1 if angle is within epsilon the boundaries of sector s*/
int
is_almost_on_boundary(double angle, int s, double start_angle,
		      double end_angle, int nsect)
{
    /*the boundaries of sector s */
    double ssize = (end_angle - start_angle) / nsect;

    return is_almost_on_boundary(angle, s * ssize) ||
	is_almost_on_boundary(angle, (s + 1) * ssize);
}


/***********************************************************************
 returns true if the event is inside the given sector */
int is_inside(AEvent * e, double start_angle, double end_angle)
{
    assert(e);
    return (e->angle >= (start_angle - EPSILON) &&
	    e->angle <= (end_angle + EPSILON));
}

/***********************************************************************
 returns true if this angle is inside the given sector */
int is_inside(double angle, double start_angle, double end_angle)
{
    return (angle >= (start_angle - EPSILON) &&
	    angle <= (end_angle + EPSILON));
}



/***********************************************************************
 return the start angle of the i-th sector. Assuming that
[start..end] is split into nsectors */
double
get_sector_start(int i, double start_angle, double end_angle, int nsect)
{
    assert(is_inside(i, nsect));
    return start_angle + i * ((end_angle - start_angle) / nsect);
}



/***********************************************************************
 return the start angle of the i-th sector. Assuming that
[start..end] is split into nsectors */
double get_sector_end(int i, double start_angle, double end_angle, int nsect)
{
    assert(is_inside(i, nsect));
    return start_angle + (i + 1) * ((end_angle - start_angle) / nsect);
}



/***********************************************************************
 return 1 is s is inside sector; that is, if it is not -1 */
int is_inside(int s, int nsect)
{
    return (s >= 0 && s < nsect);
}


/***********************************************************************
 the event e spans sectors from start_s to end_s; Action: update
   high[] for each spanned sector. start_s and both_s can be -1, which
   means outside given sector---in that case long cell spans to the
   boundary of the sector.
 */
void
process_long_cell(int start_s, int end_s, int nsect,
		  Viewpoint * vp, AEvent * e, double *high)
{

    DISTRIBDEBUG printf("LONG CELL: spans [%3d, %3d] ", start_s, end_s);
    double ctrgrad = calculate_center_gradient(e, vp);

    /*ENTER event is outside */
    if (start_s == -1) {
	assert(e->eventType == EXITING_EVENT);
	assert(is_inside(end_s, nsect));
	/*span from 0 to end_s */
	for (int j = 0; j < end_s; j++) {
	    if (high[j] < ctrgrad) {
		/*printf("update high[%d] from %.2f to %.2f ", j, high[j], ctrgrad); */
		high[j] = ctrgrad;
	    }
	}
	return;
    }

    /*EXIT event is outside */
    if (end_s == -1) {
	assert(e->eventType == ENTERING_EVENT);
	assert(is_inside(start_s, nsect));
	/*span from start_s to nsect */
	for (int j = start_s + 1; j < nsect; j++) {
	    if (high[j] < ctrgrad) {
		/*printf("update high[%d] from %.2f to %.2f ", j, high[j], ctrgrad); */
		high[j] = ctrgrad;
	    }
	}
	return;
    }

    /*the usual scenario, both inside sector */
    if (start_s < end_s) {
	/*we must update high[] in start_s+1..end_s-1 */
	for (int j = start_s + 1; j < end_s; j++) {
	    if (high[j] < ctrgrad) {
		/*printf("update high[%d] from %.2f to %.2f ", j, high[j], ctrgrad); */
		high[j] = ctrgrad;
	    }
	}
	return;
    }
    else {
	/*start_s > end_s: we must insert in [start_s..nsect] and [0, end_s] */
	for (int j = start_s + 1; j < nsect; j++) {
	    if (high[j] < ctrgrad) {
		/*printf("update high[%d] from %.2f to %.2f ", j, high[j], ctrgrad); */
		high[j] = ctrgrad;
	    }
	}
	for (int j = 0; j < end_s; j++) {
	    if (high[j] < ctrgrad) {
		/*printf("update high[%d] from %.2f to %.2f ", j, high[j], ctrgrad); */
		high[j] = ctrgrad;
	    }
	}
    }
    return;
}

/***********************************************************************
 prints how many events were inserted and dropped in each sector */
void
print_sector_stats(off_t nevents, int nsect, double *high,
		   long *total,
		   long *insert, long *drop, AMI_STREAM < AEvent > *sector,
		   AMI_STREAM < AEvent > *bndSector, long *bndInsert,
		   long longEvents, double start_angle, double end_angle)
{


    unsigned long totalSector = 0, totalDrop = 0, totalInsert = 0;

    for (int i = 0; i < nsect; i++) {
	assert(total[i] == insert[i] + drop[i]);
	assert(insert[i] == sector[i].stream_len());

	assert(bndInsert[i] == bndSector[i].stream_len());
	
	totalSector += total[i];
	totalDrop += drop[i];
	totalInsert += insert[i];
	
    }
    assert(totalSector == nevents);
	
#ifdef __GRASS__
    PRINT_DISTRIBUTE {
	  G_message("-----nsectors=%d\n", nsect);
	  for (int i = 0; i < nsect; i++) {
	    G_message("\ts=%3d  ", i);
	    G_message("[%.4f, %.4f] ",
				  get_sector_start(i, start_angle, end_angle, nsect),
				  get_sector_end(i, start_angle, end_angle, nsect));
	    G_message("high = %9.1f, ", high[i]);
	    G_message("total = %10ld, ", total[i]);
	    G_message("inserted = %10ld, ", insert[i]);
	    G_message("dropped = %10ld, ", drop[i]);
	    G_message("BOUNDARY = %5ld", bndInsert[i]);
	    G_message("\n");
	  }
    }
    G_message("Distribute [%.4f, %.4f]: nsect=%d, ", start_angle, end_angle,
			  nsect);
    G_message
	  ("total events %lu, inserted %lu, dropped %lu, long events=%ld\n",
	   totalSector, totalInsert, totalDrop, longEvents);
#else
    PRINT_DISTRIBUTE {
	  printf("-----nsectors=%d\n", nsect);
	  for (int i = 0; i < nsect; i++) {
	    printf("\ts=%3d  ", i);
	    printf("[%.4f, %.4f] ",
			   get_sector_start(i, start_angle, end_angle, nsect),
			   get_sector_end(i, start_angle, end_angle, nsect));
	    printf("high = %9.1f, ", high[i]);
	    printf("total = %10ld, ", total[i]);
	    printf("inserted = %10ld, ", insert[i]);
	    printf("dropped = %10ld, ", drop[i]);
	    printf("BOUNDARY = %5ld", bndInsert[i]);
	    printf("\n");
	  }
    }
    printf("Distribute [%.4f, %.4f]: nsect=%d, ", start_angle, end_angle,
		   nsect);
    printf("total events %lu, inserted %lu, dropped %lu, long events=%ld\n",
		   totalSector, totalInsert, totalDrop, longEvents);
    fflush(stdout);
#endif
    return;
}



/***********************************************************************
 computes the number of sector for the distribution sweep; technically
   M/2B because you need 2 streams per sector */
int compute_n_sectors()
{

    long memSizeBytes = MM_manager.memory_available();
    unsigned int blockSizeBytes = UntypedStream::get_block_length();
	
    /*printf("computeNSect: block=%d, mem=%d\n", blockSizeBytes, (int)memSizeBytes); */
    int nsect = (int)(memSizeBytes / (2 * blockSizeBytes));

    /*be safe */
    if (nsect > 4)
	nsect = nsect / 2;

    /*if it happens that we are at the end of memory, set nsect=2;
       //technically, if there is not enough memory to hold two
       //blocks, the function should enter solve_in_memory; so there
       //is not enough memory to solve in memory nor to
       //distribute...this shoudl happen only under tests with very
       //very little memory. just set nsect=2 and hope that it
       //works */

    if (nsect == 0 || nsect == 1)
	nsect = 2;
    else {
	  /*we'll have 2 streams for each sector open; subtract 10 to be safe */
	  if (2 * nsect > MAX_STREAMS_OPEN - 10)
	    nsect = (MAX_STREAMS_OPEN - 10) / 2;
    }
    printf("nsectors set to %d\n", nsect);
    fflush(stdout);
    return nsect;
}



/***********************************************************************
 compute the sector that contains this angle; there are nsect
   sectors that span the angle interval [sstartAngle, sendAngle]. if
   angle falls outside, return -1*/
int
get_event_sector(double angle, double sstartAngle, double sendAngle,
		 int nsect)
{

    int s = -1;

    /*first protect against rounding errors
       //in the last sector */
    if (fabs(angle - sendAngle) < EPSILON)
	return nsect - 1;

    /*in the first sector */
    if (fabs(angle - sstartAngle) < EPSILON)
	return 0;

    double ssize = fabs(sstartAngle - sendAngle) / nsect;

    s = (int)((angle - sstartAngle) / ssize);
    /*printf("getsector: fit %.2f in (%.2f. %.2f)", angle, sstartAngle, sendAngle); 
       //printf("ssize = %.2f, s=%d", ssize, s);
       //assert (s >= 0 && s < nsect); */
    if (s < 0 || s >= nsect) {
	/*falls outside --- this can happen when finding sector of pair
	   //event; e.g. ENTER is inside sector, and its pair EXIT event
	   //falls outside sector. */
	s = -1;
    }
    return s;
}



/***********************************************************************
 insert event in this sector */
void insert_event_in_sector(AMI_STREAM < AEvent > *str, AEvent * e)
{

    assert(str && e);
    AMI_err ae;

    ae = str->write_item(*e);
    assert(ae == AMI_ERROR_NO_ERROR);

    return;
}


/**********************************************************************
 insert event e into sector */
void
insert_event_in_sector_no_drop(AEvent * e, int s, AMI_STREAM < AEvent > *str,
			       long *insert)
{

    /*note: if on boundary, PRECISION ISSUES??  should insert both sectors? */
    DISTRIBDEBUG {
	print_event(*e);
	printf(" insert in sector %3d\n", s);
    }
    AMI_err ae;

    ae = str->write_item(*e);
    assert(ae == AMI_ERROR_NO_ERROR);
    insert[s]++;

    return;
}

/**********************************************************************
 insert event e into sector if it is not occluded by high_s */
void
insert_event_in_sector(AEvent * e, int s, AMI_STREAM < AEvent > *str,
		       double high_s, Viewpoint * vp, long *insert,
		       long *drop)
{

    /*note: if on boundary, PRECISION ISSUES??  should insert both sectors?

       //if not occluded by high_s insert it in its sector */
    if (!is_center_gradient_occluded(e, high_s, vp)) {
	insert[s]++;
	DISTRIBDEBUG {
	    print_event(*e);
	    printf(" insert in sector %3d\n", s);
	}
	AMI_err ae;

	ae = str->write_item(*e);
	assert(ae == AMI_ERROR_NO_ERROR);

    }
    else {
	/*assert(calculateCenterGradient(e, vp) <= high[s]);
	   //technically, if its a QUERY we should write it as invisible in
	   //vis stream; but as an optimization, our vis stream only
	   //records visible stuff. */
	DISTRIBDEBUG print_dropped(e, vp, high_s);

	drop[s]++;
    }

    return;
}


/***********************************************************************
 returns 1 if the center of event is occluded by the gradient, which
   is assumed to be in line with the event  */
int is_center_gradient_occluded(AEvent * e, double gradient, Viewpoint * vp)
{

    assert(e && vp);
    double eg = calculate_center_gradient(e, vp);

    return (eg < gradient);
}

/***********************************************************************
called when dropping an event e, high is the highest gradiant value
//in its sector*/
void print_dropped(AEvent * e, Viewpoint * vp, double high)
{

    assert(e && vp);
    double eg = calculate_center_gradient(e, vp);

    printf(" dropping grad=%.2f, high=%.2f\n", eg, high);

    return;
}
