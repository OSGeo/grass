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


#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

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

    gradient = (e->elev - vp->elev) * (e->elev - vp->elev) / sqdist;
    /*maintain sign */
    if (e->elev < vp->elev)
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
    e.angle = e.elev = 0;
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
    e.angle = e.elev = 0;
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

    /*M_PI is defined in math.h to represent 3.14159... */
    if (viewpointY == eventY && eventX > viewpointX) {
	return 0;		/*between 1st and 4th quadrant */
    }
    else if (eventX > viewpointX && eventY < viewpointY) {
	/*first quadrant */
	return atan((viewpointY - eventY) / (eventX - viewpointX));

    }
    else if (viewpointX == eventX && viewpointY > eventY) {
	/*between 1st and 2nd quadrant */
	return (M_PI) / 2;

    }
    else if (eventX < viewpointX && eventY < viewpointY) {
	/*second quadrant */
	return ((M_PI) / 2 +
		atan((viewpointX - eventX) / (viewpointY - eventY)));

    }
    else if (viewpointY == eventY && eventX < viewpointX) {
	/*between 1st and 3rd quadrant */
	return M_PI;

    }
    else if (eventY > viewpointY && eventX < viewpointX) {
	/*3rd quadrant */
	return (M_PI + atan((eventY - viewpointY) / (viewpointX - eventX)));

    }
    else if (viewpointX == eventX && viewpointY < eventY) {
	/*between 3rd and 4th quadrant */
	return (M_PI * 3.0 / 2.0);
    }
    else if (eventX > viewpointX && eventY > viewpointY) {
	/*4th quadrant */
	return (M_PI * 3.0 / 2.0 +
		atan((eventX - viewpointX) / (eventY - viewpointY)));
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
    /*if ((fabs(*x -e.col) >=1) || (fabs(*y -e.row) >=1)) {
       //printf("x-e.col=%f, y-e.row=%f ", fabs(*x -e.col), fabs(*y -e.row)); 
       //print_event(e); 
       //printf("vp=(%d, %d), x=%.3f, y=%.3f", viewpointRow, viewpointCol, *x, *y);
       //assert(0); 
       //exit(1);
       // } */
    return;
}


/* ------------------------------------------------------------ */
void print_event(AEvent a)
{
    char c = '0';

    if (a.eventType == ENTERING_EVENT)
	c = 'E';
    if (a.eventType == EXITING_EVENT)
	c = 'X';
    if (a.eventType == CENTER_EVENT)
	c = 'Q';
    printf("ev=[(%3d, %3d), e=%8.1f a=%4.2f t=%c] ",
	   a.row, a.col, a.elev, a.angle, c);
    return;
}


/* ------------------------------------------------------------ */
/*computes the distance from the event to the viewpoint. Note: all 3
   //events associate to a cell are considered at the same distance, from
   //the center of the cell to the viewpoint */
double
get_square_distance_from_viewpoint(const AEvent & a, const Viewpoint & vp)
{

    double eventy, eventx;

    calculate_event_position(a, vp.row, vp.col, &eventy, &eventx);

    double dist = (eventx - vp.col) * (eventx - vp.col) +
	(eventy - vp.row) * (eventy - vp.row);
    /*don't take sqrt, it is expensive; suffices for comparison */
    return dist;
}

/* ------------------------------------------------------------ */
/* a duplicate of get_square_distance_from_viewpoint() needed for debug */
double
get_square_distance_from_viewpoint_with_print(const AEvent & a,
					      const Viewpoint & vp)
{

    double eventy, eventx;

    calculate_event_position(a, vp.row, vp.col, &eventy, &eventx);
    double dist = (eventx - vp.col) * (eventx - vp.col) +
	(eventy - vp.row) * (eventy - vp.row);
    /*don't take sqrt, it is expensive; suffices for comparison */

    print_event(a);
    printf(" pos= (%.3f. %.3f) sqdist=%.3f\n", eventx, eventy, dist);

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

  double dif_x, dif_y, sqdist;
  dif_x = (vp.col - col);
  dif_y = (vp.row - row);
  
  /* expensive to take squareroots so use squares */
  sqdist = (dif_x * dif_x + dif_y * dif_y) * hd.cellsize * hd.cellsize;
  
  if (sqdist > maxDist *maxDist) {
	return 1;
  }
  else {
	return 0;
  }
}


void testeventlist(AEvent* elist, size_t n) {

  printf("testing event list..%lu ", n); fflush(stdout); 
  AEvent e = {0, 0,0};
  for (size_t i=0; i< n; i++) elist[i] = e; 
  printf("ok "); fflush(stdout); 
}

/*///////////////////////////////////////////////////////////
   ------------------------------------------------------------ 

   input: an array capable to hold the max number of events, an arcascii
   file, a grid header and a viewpoint; action: figure out all events
   in the input file, and write them to the event list. data is
   allocated and initialized with all the cells on the same row as the
   viewpoint.  it returns the number of events.
 */
size_t
init_event_list_in_memory(AEvent * eventList, char* inputfname, 
						  Viewpoint * vp,
						  GridHeader * hd, ViewOptions viewOptions,
						  double **data, MemoryVisibilityGrid *visgrid ) {
  printf("computing events..");
  fflush(stdout);
  assert(eventList && inputfname && hd && vp && visgrid);

  
  /* data is used to store all the cells on the same row as the
	 viewpoint. */
  *data = (double *)malloc(hd->ncols * sizeof(double));
  assert(*data);
  
  /* open input raster */
  FILE * grid_fp; 
  grid_fp = fopen(inputfname, "r"); 
  assert(grid_fp); 
  /* we do this just to position the pointer after header for reading
	 the data */
  read_header_from_arcascii_file(grid_fp);
  
  
  /* scan throught the arcascii file */
  size_t nevents = 0;
  dimensionType i, j;
  double ax, ay;
  AEvent e;
  
  e.angle = -1;
  for (i = 0;  i< hd->nrows; i++) {
	for (j = 0; j < hd->ncols; j++) {
	  
	  e.row = i;
	  e.col = j;
	  fscanf(grid_fp, "%f", &(e.elev));
	  //printf("(i=%3d, j=%3d): e=%.1f\n", i,j,e.elev); fflush(stdout);
	  
		/*write the row of data going through the viewpoint */
		if (i == vp->row) {
		  (*data)[j] = e.elev;
		  if (j == vp->col) {
			set_viewpoint_elev(vp, e.elev + viewOptions.obsElev);
			/*what to do when viewpoint is NODATA ? */
			if (is_nodata(hd, e.elev)) {
			  printf("WARNING: viewpoint is NODATA. ");
			  printf("Will assume its elevation is %.f\n", e.elev);
			}
		  }
		}
		
		/*don't insert the viewpoint events in the list */
		if (i == vp->row && j == vp->col)
		  continue;
		
		/*don't insert the nodata cell events */
		if (is_nodata(hd, e.elev)) {
		  /* record this cell as being NODATA; this is necessary so
			 that we can distingush invisible events, from nodata
			 events in the output */
		  add_result_to_inmem_visibilitygrid(visgrid,i,j,hd->nodata_value);
		  continue;
		}

		/* if point is outside maxDist, do NOT include it as an
		   event */
		if (is_point_outside_max_dist(*vp, *hd, i, j, viewOptions.maxDist))
		  continue;

		e.eventType = ENTERING_EVENT;
		calculate_event_position(e, vp->row, vp->col, &ay, &ax);
		e.angle = calculate_angle(ax, ay, vp->col, vp->row);
		eventList[nevents] = e;
		nevents++;
		
		e.eventType = CENTER_EVENT;
		calculate_event_position(e, vp->row, vp->col, &ay, &ax);
		e.angle = calculate_angle(ax, ay, vp->col, vp->row);
		eventList[nevents] = e;
		nevents++;
		
		e.eventType = EXITING_EVENT;
		calculate_event_position(e, vp->row, vp->col, &ay, &ax);
		e.angle = calculate_angle(ax, ay, vp->col, vp->row);
		eventList[nevents] = e;
		nevents++;
		
      } /* for j  */
    } /* for i */
  fclose(grid_fp); 
  
  printf("..done\n"); fflush(stdout);
  printf("Event array size: %lu x %dB (%lu MB)\n",
		 (unsigned long)nevents, (int)sizeof(AEvent),
		 (unsigned long)(((long long)(nevents * sizeof(AEvent))) >> 20));
  fflush(stdout);
  
  return nevents;
}







/*///////////////////////////////////////////////////////////
   ------------------------------------------------------------ 
   input: an arcascii file, a grid header and a viewpoint; action:
   figure out all events in the input file, and write them to the
   stream.  It assumes the file pointer is positioned rigth after the
   grid header so that this function can read all grid elements.

   if data is not NULL, it creates an array that stores all events on
   the same row as the viewpoint. 
 */
AMI_STREAM < AEvent > *
init_event_list(char* inputfname, Viewpoint * vp, GridHeader * hd, 
				ViewOptions viewOptions, double **data, 
				IOVisibilityGrid *visgrid)
{
    printf("computing events..");
    fflush(stdout);
    assert(inputfname && hd && vp && visgrid);
	
    /*create the event stream that will hold the events */
    AMI_STREAM < AEvent > *eventList = new AMI_STREAM < AEvent > ();
    assert(eventList);

	if (data != NULL) {
	  /*data is used to store all the cells on the same row as the
	  //viewpoint. */
	  *data = (double *)malloc(hd->ncols * sizeof(double));
	  assert(*data);
	}

	FILE * grid_fp = fopen(inputfname, "r"); 
	assert(grid_fp); 
    /*we do this just to position the pointer after header for reading
	//the data
	//GridHeader* foo = */
    read_header_from_arcascii_file(grid_fp);
	
    /*scan throught the arcascii file */
    dimensionType i, j;
    double ax, ay;
    AEvent e;

    e.angle = -1;
    for (i = 0; i < hd->nrows; i++) {
	for (j = 0; j < hd->ncols; j++) {
	  
	  e.row = i;
	  e.col = j;
	  fscanf(grid_fp, "%f", &(e.elev));

	  if (data  != NULL) {	  
		/*write the row of data going through the viewpoint */
		if (i == vp->row) {
		  (*data)[j] = e.elev;
		}
	  }

	  if (i == vp-> row && j == vp->col) {
		set_viewpoint_elev(vp, e.elev + viewOptions.obsElev);
		/*what to do when viewpoint is NODATA */
		if (is_nodata(hd, e.elev)) {
		  printf("WARNING: viewpoint is NODATA. ");
		  printf("Will assume its elevation is %.f\n", e.elev);
		};
	  }
	  
	  /*don't insert the viewpoint events in the list */
	  if (i == vp->row && j == vp->col)
		continue;
	  

	  /*don't insert the nodata cell events */
	  if (is_nodata(hd, e.elev)) {
		/* record this cell as being NODATA. ; this is necessary so
			 that we can distingush invisible events, from nodata
			 events in the output */
		VisCell visCell = {i, j, hd->nodata_value};
		add_result_to_io_visibilitygrid(visgrid, &visCell);
		continue;
	  }

	  /* if point is outside maxDist, do NOT include it as an
		 event */
	  if(is_point_outside_max_dist(*vp, *hd, i, j, viewOptions.maxDist))
		continue;


	    e.eventType = ENTERING_EVENT;
	    calculate_event_position(e, vp->row, vp->col, &ay, &ax);
	    e.angle = calculate_angle(ax, ay, vp->col, vp->row);
	    eventList->write_item(e);

	    e.eventType = CENTER_EVENT;
	    calculate_event_position(e, vp->row, vp->col, &ay, &ax);
	    e.angle = calculate_angle(ax, ay, vp->col, vp->row);
	    eventList->write_item(e);

	    e.eventType = EXITING_EVENT;
	    calculate_event_position(e, vp->row, vp->col, &ay, &ax);
	    e.angle = calculate_angle(ax, ay, vp->col, vp->row);
	    eventList->write_item(e);
	} /* for j  */
    } /* for i */

	fclose(grid_fp); 

    printf("..done\n");
    printf("nbEvents = %lu\n", (unsigned long)eventList->stream_len());
    printf("Event stream length: %lu x %dB (%lu MB)\n",
	   (unsigned long)eventList->stream_len(), (int)sizeof(AEvent),
	   (unsigned
	    long)(((long long)(eventList->stream_len() *
			       sizeof(AEvent))) >> 20));
    fflush(stdout);
	
    return eventList;
}







// /*///////////////////////////////////////////////////////////
//    ------------------------------------------------------------ 
//    input: an arcascii file, a grid header and a viewpoint; action:
//    figure out all events in the input file, and write them to the
//    stream. data is allocated and initialized with all the cells on the
//    same row as the viewpoint. It assumes the file pointer is positioned
//    rigth after the grid header so that this function can read all grid
//    elements.
//  */
// AMI_STREAM < AEvent > *
// init_event_list(FILE * grid_fp, Viewpoint * vp,
// 				GridHeader * hd, ViewOptions viewOptions, 
// 				IOVisibilityGrid *visgrid) {

//     printf("computing events..");
//     fflush(stdout);
//     assert(grid_fp && hd && vp && visgrid);

//     /*create the event stream that will hold the events */
//     AMI_STREAM < AEvent > *eventList = new AMI_STREAM < AEvent > ();
//     assert(eventList);


//     /*we do this just to position the pointer after header for reading
// 	//the data GridHeader* foo = */
//     read_header_from_arcascii_file(grid_fp);

//     /*scan throught the arcascii file */
//     dimensionType i, j;
//     double ax, ay;
//     AEvent e;
	
//     e.angle = -1;
//     for (i = 0; i < hd->nrows; i++) {
// 	  for (j = 0; j < hd->ncols; j++) {
		
// 	    e.row = i;
// 	    e.col = j;
// 	    fscanf(grid_fp, "%f", &(e.elev));

// 	    if (i == vp->row && j == vp->col) {
// 		  set_viewpoint_elev(vp, e.elev+viewOptions.obsElev);
// 		  /*what to do when viewpoint is NODATA */
// 		  if (is_nodata(hd, e.elev)) {
// 		    printf("WARNING: viewpoint is NODATA. ");
// 		    printf("Will assume its elevation is %.f\n", e.elev);
// 		  };
// 	    }
		
// 	    /*don't insert the viewpoint events in the list */
// 	    if (i == vp->row && j == vp->col)
// 		  continue;

	
// 	    /*don't insert the nodata cell events */
// 	    if (is_nodata(hd, e.elev)) {
// 		/*printf("(%d, %d) dropping\n", i, j); */
// 		  /* record this cell as being NODATA; this is necessary so
// 			 that we can distingush invisible events, from nodata
// 			 events in the output */
// 		  VisCell viscell = {i,j, hd->nodata_value};
// 		  add_result_to_io_visibilitygrid(visgrid, &viscell);
// 			continue;
// 	    }
	
// 		/* if point is outside maxDist, do NOT include it as an
// 		   event */	
// 		if(is_point_outside_max_dist(*vp, *hd, i, j, viewOptions.maxDist))
// 		  continue;


// 	    e.eventType = ENTERING_EVENT;
// 	    calculate_event_position(e, vp->row, vp->col, &ay, &ax);
// 	    e.angle = calculate_angle(ax, ay, vp->col, vp->row);
// 	    eventList->write_item(e);
// 	    /*d = get_square_distance_from_viewpoint(e, *vp); 
// 		//printf("(%d, %d) insert ENTER (%.2f,%.2f) d=%.2f\n", i, j, ay, ax, d); */
		
// 	    e.eventType = CENTER_EVENT;
// 	    calculate_event_position(e, vp->row, vp->col, &ay, &ax);
// 	    e.angle = calculate_angle(ax, ay, vp->col, vp->row);
// 	    eventList->write_item(e);
// 	    /*d = get_square_distance_from_viewpoint(e, *vp); 
// 		//printf("(%d, %d) insert CENTER (%.2f,%.2f) d=%.2f\n", i,j, ay, ax, d); */

// 	    e.eventType = EXITING_EVENT;
// 	    calculate_event_position(e, vp->row, vp->col, &ay, &ax);
// 	    e.angle = calculate_angle(ax, ay, vp->col, vp->row);
// 	    eventList->write_item(e);
// 	    /*d = get_square_distance_from_viewpoint(e, *vp); 
// 		//printf("(%d, %d) insert EXIT (%.2f,%.2f) d=%.2f\n", i, j, ay, ax, d); */
// 	}
//     }
//     printf("..done\n");
//     printf("nbEvents = %lu\n", (unsigned long)eventList->stream_len());
//     printf("Event stream length: %lu x %dB (%lu MB)\n",
// 	   (unsigned long)eventList->stream_len(), (int)sizeof(AEvent),
// 	   (unsigned
// 	    long)(((long long)(eventList->stream_len() *
// 			       sizeof(AEvent))) >> 20));
//     fflush(stdout);
//     return eventList;
// }









/* ------------------------------------------------------------ 
   //note: this is expensive because distance is not storedin the event
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
    da = (eventx - globalVP.col) * (eventx - globalVP.col) +
	  (eventy - globalVP.row) * (eventy - globalVP.row);
    calculate_event_position(b, globalVP.row, globalVP.col, &eventy, &eventx);
    db = (eventx - globalVP.col) * (eventx - globalVP.col) +
	  (eventy - globalVP.row) * (eventy - globalVP.row);

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
	else if (a.eventType == ENTERING_EVENT)
	    return 1;
	return 0;
    }
}

/* ------------------------------------------------------------ */
/* a copy of the function above is needed by qsort, when teh
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




void sort_check(AMI_STREAM < AEvent > *eventList, Viewpoint vp)
{
    printf("checking sort..");
    fflush(stdout);
    assert(eventList);

    size_t i, nbe = eventList->stream_len();

    eventList->seek(0);
    AEvent *crt, *next;
    double crtd, nextd;
    AMI_err ae;

    ae = eventList->read_item(&crt);
    assert(ae == AMI_ERROR_NO_ERROR);
    crtd = get_square_distance_from_viewpoint(*crt, vp);

    for (i = 0; i < nbe - 1; i++) {
	ae = eventList->read_item(&next);
	assert(ae == AMI_ERROR_NO_ERROR);
	nextd = get_square_distance_from_viewpoint(*next, vp);
	if (crtd > nextd)
	    assert(0);
	crtd = nextd;
    }
    printf("..sort test passed\n");
    return;
}
