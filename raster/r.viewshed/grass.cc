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


#include <stdlib.h>
#include <stdio.h>

#ifdef __GRASS__

extern "C"
{
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/glocale.h>
}

#include "grass.h"
#include "visibility.h"





/* ------------------------------------------------------------ */
/* if viewOptions.doCurv is on then adjust the passed height for
   curvature of the earth; otherwise return the passed height
   unchanged. 
*/
float  adjust_for_curvature(Viewpoint vp, dimensionType row,
							dimensionType col, float h,
							ViewOptions viewOptions) {
  
  if (!viewOptions.doCurv) return h;

  assert(viewOptions.ellps_a != 0);  
  double dif_x, dif_y, sqdist; 
  dif_x = (vp.col - col);
  dif_y = (vp.row - row);
  sqdist = (dif_x * dif_x + dif_y * dif_y) * viewOptions.cellsize * viewOptions.cellsize;
  
  return h - (sqdist / (2 * viewOptions.ellps_a));
}





/* ************************************************************ */
/*return a GridHeader that has all the relevant data filled in from
  GRASS */
GridHeader *read_header_from_GRASS(char *rastName, Cell_head *region) {

    assert(rastName);

    /*allocate the grid header to fill */
    GridHeader *hd = (GridHeader *) G_malloc(sizeof(GridHeader));
    assert(hd);

    /*get the num rows and cols from GRASS */
    int nrows, ncols;
    nrows = G_window_rows();
    ncols = G_window_cols();
    /*check for loss of prescion */
    if (nrows <= maxDimension && ncols <= maxDimension) {
	  hd->nrows = (dimensionType) nrows;
	  hd->ncols = (dimensionType) ncols;
    }
    else {
	  G_fatal_error(_("grid dimension too big for current precision"));
	  exit(EXIT_FAILURE);
    }

    /*fill in rest of header */
    hd->xllcorner = G_col_to_easting(0, region);
    hd->yllcorner = G_row_to_northing(0, region);
    /*Cell_head stores 2 resolutions, while GridHeader only stores 1
	//make sure the two Cell_head resolutions are equal */
	if (fabs(region->ew_res - region->ns_res) > .001) {
	  G_warning(_("east-west resolution does not equal north-south resolutio. The viewshed computation assumes the cells are square, so in this case this may result in innacuracies."));
	  // 	exit(EXIT_FAILURE);
	  //     
	}
	hd->cellsize = (float) region->ew_res;
	//store the null value of the map
	G_set_f_null_value(& (hd->nodata_value), 1);
	printf("nodata value set to %f\n", hd->nodata_value);
  return hd;
}








/*  ************************************************************ */
/* input: an array capable to hold the max number of events, a raster
 name, a viewpoint and the viewOptions; action: figure out all events
 in the input file, and write them to the event list. data is
 allocated and initialized with all the cells on the same row as the
 viewpoint. it returns the number of events. initialize and fill
 AEvent* with all the events for the map.  Used when solving in
 memory, so the AEvent* should fit in memory.  */
size_t
grass_init_event_list_in_memory(AEvent * eventList, char *rastName,
								Viewpoint * vp, GridHeader* hd,  
								ViewOptions viewOptions, double **data,  
								MemoryVisibilityGrid* visgrid ) {
  
  G_verbose_message(_("computing events ..."));
  assert(eventList && vp && visgrid);
  //GRASS should be defined 

  /*alloc data ; data is used to store all the cells on the same row
	as the viewpoint. */ 
  *data = (double *)malloc(G_window_cols() * sizeof(double));
  assert(*data);
  
  /*get the mapset name */
  char *mapset;
  mapset = G_find_cell(rastName, "");
  if (mapset == NULL) {
	G_fatal_error(_("raster map [%s] not found"), rastName);
	exit(EXIT_FAILURE);
  }
  
  /*open map */
  int infd;
  if ((infd = G_open_cell_old(rastName, mapset)) < 0) {
	G_fatal_error(_("Cannot open raster file [%s]"), rastName);
	exit(EXIT_FAILURE);
  }
  
  /*get the data_type */
  RASTER_MAP_TYPE data_type;
  data_type = G_raster_map_type(rastName, mapset);
  
  /*buffer to hold a row */
  void *inrast;
  inrast = G_allocate_raster_buf(data_type);
  assert(inrast); 

  /*DCELL to check for loss of prescion- haven't gotten that to work
	yet though */
  DCELL d;
  int isnull = 0;
	
  /*keep track of the number of events added, to be returned later */
  size_t nevents = 0;
  
  /*scan through the raster data */
  dimensionType i, j, k;
  double ax, ay;
  AEvent e;
  
  e.angle = -1;
  for (i = 0; i < G_window_rows(); i++) {
	/*read in the raster row */
	int rasterRowResult = G_get_raster_row(infd, inrast, i, data_type);
	if (rasterRowResult <= 0) {
	  G_fatal_error(_("coord not read from row %d of %s"), i, rastName);
	  exit(EXIT_FAILURE);
	}
	
	/*fill event list with events from this row */
	for (j = 0; j < G_window_cols(); j++) {
	  e.row = i;
	  e.col = j;
	  
	  /*read the elevation value into the event, depending on data_type */
	  switch (data_type) {
	  case CELL_TYPE:
		isnull = G_is_c_null_value(&(((CELL *) inrast)[j]));
		e.elev = (float)(((CELL *) inrast)[j]);
		break;
	    case FCELL_TYPE:
		isnull = G_is_f_null_value(&(((FCELL *) inrast)[j]));
		e.elev = (float)(((FCELL *) inrast)[j]);
		break;
	    case DCELL_TYPE:
		isnull = G_is_d_null_value(&(((DCELL *) inrast)[j]));
		e.elev = (float)(((DCELL *) inrast)[j]);
		break;
	  }

	  /* adjust for curvature */
	  e.elev = adjust_for_curvature(*vp, i, j, e.elev, viewOptions);
	  
	  /*write it into the row of data going through the viewpoint */
	  if (i == vp->row) {
		(*data)[j] = e.elev;
	  }
	  
	  /* set the viewpoint, and don't insert it into eventlist */
	  if (i == vp->row && j == vp->col) {
		set_viewpoint_elev(vp, e.elev + viewOptions.obsElev);
		if (isnull) {
		  	/*what to do when viewpoint is NODATA ? */
		  G_message(_("WARNING: viewpoint is NODATA. "));
		  G_message(_("Will assume its elevation is = %f"), vp->elev);
		}
		continue;	
	  }
	  
	  /*don't insert in eventlist nodata cell events */
	  if (isnull) {
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
	  
	  /* if it got here it is not the viewpoint, not NODATA, and
		 within max distance from viewpoint; generate its 3 events
		 and insert them */
	  /*put event into event list */
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
	  
	}
  }

  G_message(_("...done creating event list"));
  G_close_cell(infd);
  return nevents;
}





/* ************************************************************ */
/* input: an arcascii file, a grid header and a viewpoint; action:
   figure out all events in the input file, and write them to the
   stream.  It assumes the file pointer is positioned rigth after the
   grid header so that this function can read all grid elements.

   if data is not NULL, it creates an array that stores all events on
   the same row as the viewpoint. 
 */
AMI_STREAM < AEvent > *
grass_init_event_list(char *rastName, Viewpoint* vp, GridHeader* hd, 
					  ViewOptions viewOptions, double **data, 
					  IOVisibilityGrid* visgrid) {
  
  G_message(_("computing events ..."));
  assert(rastName && vp && hd && visgrid);

  if (data != NULL) {
	/*data is used to store all the cells on the same row as the
	//viewpoint. */  
	*data = (double *)G_malloc(G_window_cols() * sizeof(double));
	assert(*data);
  }
  
  /*create the event stream that will hold the events */
  AMI_STREAM < AEvent > *eventList = new AMI_STREAM < AEvent > ();
  assert(eventList);
  
  /*determine which mapset we are in */
  char *mapset;
  mapset = G_find_cell(rastName, "");
  if (mapset == NULL) {
	G_fatal_error(_("raster map [%s] not found"), rastName);
	exit(EXIT_FAILURE);
  }
  
  /*open map */
  int infd;
  if ((infd = G_open_cell_old(rastName, mapset)) < 0) {
	G_fatal_error(_("Cannot open raster file [%s]"), rastName);
	exit(EXIT_FAILURE);
  }
  RASTER_MAP_TYPE data_type;
  data_type = G_raster_map_type(rastName, mapset);
  void *inrast;
  inrast = G_allocate_raster_buf(data_type);
  assert(inrast); 

  /*scan through the raster data */
  DCELL d;
  int isnull = 0;
  dimensionType i, j, k;
  double ax, ay;
  AEvent e;
  e.angle = -1;
  
  /*start scanning through the grid */
  for (i = 0; i < G_window_rows(); i++) {
	
	/*read in the raster row */
	if (G_get_raster_row(infd, inrast, i, data_type) <= 0) {
	  G_fatal_error(_("coord not read from row %d of %s"), i, rastName);
	  exit(EXIT_FAILURE);
	}
	/*fill event list with events from this row */
	for (j = 0; j < G_window_cols(); j++) {
	  
	  e.row = i;
	  e.col = j;

	  /*read the elevation value into the event, depending on data_type */
	  switch (data_type) {
	  case CELL_TYPE:
		isnull = G_is_c_null_value(&(((CELL *) inrast)[j]));
		e.elev = (float)(((CELL *) inrast)[j]);
		break;
	  case FCELL_TYPE:
		isnull = G_is_f_null_value(&(((FCELL *) inrast)[j]));
		e.elev = (float)(((FCELL *) inrast)[j]);
		break;
	  case DCELL_TYPE:
		isnull = G_is_d_null_value(&(((DCELL *) inrast)[j]));
		e.elev = (float)(((DCELL *) inrast)[j]);
		break;
	  }
	  
	  /* adjust for curvature */
	  e.elev = adjust_for_curvature(*vp, i, j, e.elev, viewOptions);

	  if (data!= NULL) {
		/**write the row of data going through the viewpoint */
		if (i == vp->row) {
		  (*data)[j] = e.elev;
		}
	  }
	  
	  /* set the viewpoint */
	  if (i == vp-> row && j == vp->col) {
		set_viewpoint_elev(vp, e.elev + viewOptions.obsElev);
		/*what to do when viewpoint is NODATA */
		if (is_nodata(hd, e.elev)) {
		  printf("WARNING: viewpoint is NODATA. ");
		  printf("Will assume its elevation is %.f\n", e.elev);
		};
	  }
	  
	  /*don't insert viewpoint into eventlist */
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
	  if (is_point_outside_max_dist(*vp, *hd, i, j, viewOptions.maxDist))
		continue;
	  
	  /*put event into event list */
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
	} /* for j */
	
  } /* for i */
  
  G_message(_("...done creating event list\n"));
  G_close_cell(infd);
  printf("nbEvents = %lu\n", (unsigned long)eventList->stream_len());
  printf("Event stream length: %lu x %dB (%lu MB)\n",
		 (unsigned long)eventList->stream_len(), (int)sizeof(AEvent),
	   (unsigned
	    long)(((long long)(eventList->stream_len() *
						   sizeof(AEvent))) >> 20));
  fflush(stdout);
  return eventList;
}






/* ************************************************************ */
/*  saves the grid into a GRASS raster.  Loops through all elements x
	in row-column order and writes fun(x) to file.*/
void
save_grid_to_GRASS(Grid* grid, char* filename, RASTER_MAP_TYPE type, 
				   float(*fun)(float)) {
  
  G_message(_("saving grid to %s"), filename);
  assert(grid && filename);

  /*open the new raster  */
  int outfd;
  outfd = G_open_raster_new(filename, type);
  
  /*get the buffer to store values to read and write to each row */
  void *outrast;
  outrast = G_allocate_raster_buf(type);
  assert(outrast); 

  dimensionType i, j;
  for (i = 0; i < G_window_rows(); i++) {
	for (j = 0; j < G_window_cols(); j++) {
	  
	  switch (type) {
	  case CELL_TYPE:
	    ((CELL *) outrast)[j] = (CELL) fun(grid->grid_data[i][j]); 
		break;
	  case FCELL_TYPE:
		((FCELL *) outrast)[j] = (FCELL) fun(grid->grid_data[i][j]); 
		break;
	  case DCELL_TYPE:
		((DCELL *) outrast)[j] = (DCELL) fun(grid->grid_data[i][j]); 
		break;
	  }
	} /* for j */
	G_put_raster_row(outfd, outrast, type);
  } /* for i */

  G_close_cell(outfd);
  return;
}





/* ************************************************************ */
/*  using the visibility information recorded in visgrid, it creates an
	output viewshed raster with name outfname; for every point p that
	is visible in the grid, the corresponding value in the output
	raster is elevation(p) - viewpoint_elevation(p); the elevation
	values are read from elevfname raster */

void
save_vis_elev_to_GRASS(Grid* visgrid, char* elevfname, char* visfname,  
					   float vp_elev) {
					   
  G_message(_("saving grid to %s"), visfname);
  assert(visgrid && elevfname && visfname);
  
  /*get the mapset name */
  char *mapset;
  mapset = G_find_cell(elevfname, "");
  if (mapset == NULL) {
	G_fatal_error(_("raster map [%s] not found"), elevfname);
	exit(EXIT_FAILURE);
  }
  /*open elevation map */
  int elevfd;
  if ((elevfd = G_open_cell_old(elevfname, mapset)) < 0) {
	G_fatal_error(_("Cannot open raster file [%s]"), elevfname);
	exit(EXIT_FAILURE);
  }
  
  /*get elevation data_type */
  RASTER_MAP_TYPE elev_data_type;
  elev_data_type = G_raster_map_type(elevfname, mapset);
  
  /* create the visibility raster of same type */
  int visfd;
  visfd = G_open_raster_new(visfname, elev_data_type);
  
  /*get the buffers to store each row */
  void *elevrast;
  elevrast = G_allocate_raster_buf(elev_data_type);
  assert(elevrast); 
  void *visrast;
  visrast = G_allocate_raster_buf(elev_data_type);
  assert(visrast); 
  
  dimensionType i, j;
  double elev=0, viewshed_value; 
  for (i = 0; i < G_window_rows(); i++) {
	/* get the row from elevation */
	if (G_get_raster_row(elevfd, elevrast, i, elev_data_type) <= 0) {
	  G_fatal_error(_("save_vis_elev_to_GRASS: could not read row %d"), i);
	}
	for (j = 0; j < G_window_cols(); j++) {
	  
	  /* read the current elevation value */
	  int isNull = 0;
	  switch (elev_data_type) {
	  case CELL_TYPE:
		isNull = G_is_c_null_value(&((CELL *) elevrast)[j]);
		elev = (double)(((CELL *) elevrast)[j]);
		break;
	  case FCELL_TYPE:
		isNull = G_is_f_null_value(&((FCELL *) elevrast)[j]);
		elev = (double)(((FCELL *) elevrast)[j]);
		break;
	  case DCELL_TYPE:
		isNull = G_is_d_null_value(&((DCELL *) elevrast)[j]);
		elev = (double)(((DCELL *) elevrast)[j]);
		break;
	  }
	  
	  if (is_visible(visgrid->grid_data[i][j])) {
		/* elevation cannot be null */
		assert(!isNull); 
		/* write elev - viewpoint_elevation*/
		viewshed_value = elev - vp_elev; 
		writeValue(visrast,j,viewshed_value, elev_data_type);
	  } else if (is_invisible_not_nodata(visgrid->grid_data[i][j])) {
		/* elevation cannot be null */
		assert(!isNull); 
		/* write INVISIBLE */
		viewshed_value = INVISIBLE; 
		writeValue(visrast,j,viewshed_value, elev_data_type);
	  } else {
		/* nodata */ 
		assert(isNull);
		/* write  NODATA */
		writeNodataValue(visrast, j, elev_data_type);
	  }

	  
	} /* for j*/
	G_put_raster_row(visfd, visrast, elev_data_type);
  } /* for i */
  
  G_close_cell(elevfd);
  G_close_cell(visfd);
  return;
}




/* helper function to deal with GRASS writing to a row buffer */ 
void writeValue(void* bufrast, int j, double x, RASTER_MAP_TYPE data_type) {
  
    switch (data_type) {
  case CELL_TYPE:
	((CELL *) bufrast)[j] = (CELL) x;
	break;
  case FCELL_TYPE:
	((FCELL *) bufrast)[j] = (FCELL) x; 
	break;
  case DCELL_TYPE:
	((DCELL *) bufrast)[j] = (DCELL) x; 
	break;
  default: 
	G_fatal_error(_("unknown type")); 
  }
}

void writeNodataValue(void* bufrast, int j,  RASTER_MAP_TYPE data_type) {
  
  switch (data_type) {
  case CELL_TYPE:
	 G_set_c_null_value(& ((CELL *) bufrast)[j], 1);
	break;
  case FCELL_TYPE:
	G_set_f_null_value(& ((FCELL *) bufrast)[j], 1);
	break;
  case DCELL_TYPE:
	G_set_d_null_value(& ((DCELL *) bufrast)[j], 1);
	break;
  default: 
	G_fatal_error(_("unknown type")); 
	exit(EXIT_FAILURE); 
  }
}


/* ************************************************************ */
/* write the visibility grid to GRASS. assume all cells that are not
   in stream are NOT visible. assume stream is sorted in (i,j) order.
   for each value x it writes to grass fun(x) */
void
save_io_visibilitygrid_to_GRASS(IOVisibilityGrid * visgrid, 
								char* fname, RASTER_MAP_TYPE type, 
								float (*fun)(float)) {
  
  G_message(_("saving grid to %s"), fname);
  assert(fname && visgrid);
  
  /* open the output raster  and set up its row buffer */
  int visfd;
  visfd = G_open_raster_new(fname, type);
  void * visrast;
  visrast = G_allocate_raster_buf(type);
  assert(visrast); 
  
  /*set up reading data from visibility stream */
  AMI_STREAM < VisCell > *vstr = visgrid->visStr;
  off_t streamLen, counter = 0;
  streamLen = vstr->stream_len();
  vstr->seek(0);

  /*read the first element */
  AMI_err ae;
  VisCell *curResult = NULL;
  if (streamLen > 0) {
	ae = vstr->read_item(&curResult);
	assert(ae == AMI_ERROR_NO_ERROR);
	counter++;
  }

  dimensionType i, j;
  for (i = 0; i < G_window_rows(); i++) {
		for (j = 0; j < G_window_cols(); j++) {
	  
	  if (curResult->row == i && curResult->col == j) {	
		/*cell is recodred in the visibility stream: it must be
		  either visible, or NODATA  */
		writeValue(visrast,j, fun(curResult->angle), type);   
		
		/*read next element of stream */
		if (counter < streamLen) {
		  ae = vstr->read_item(&curResult);
		  assert(ae == AMI_ERROR_NO_ERROR);
		  counter++;
		}
	  }
	  else {
		/*  this cell is not in stream, so it is  invisible */
		writeValue(visrast, j, fun(INVISIBLE), type);
	  } 
	} /* for j */
	
	G_put_raster_row(visfd, visrast, type);
  } /* for i */
  
  G_close_cell(visfd);
} 



   



/* ************************************************************ */
/*  using the visibility information recorded in visgrid, it creates
	an output viewshed raster with name outfname; for every point p
	that is visible in the grid, the corresponding value in the output
	raster is elevation(p) - viewpoint_elevation(p); the elevation
	values are read from elevfname raster. assume stream is sorted in
	(i,j) order. */
void
save_io_vis_and_elev_to_GRASS(IOVisibilityGrid * visgrid, char* elevfname, 
							  char* visfname, float vp_elev) {
  
  G_message(_("saving grid to %s"), visfname);
  assert(visfname && visgrid);
  
  /*get mapset name and data type */
  char *mapset;
  mapset = G_find_cell(elevfname, "");
  if (mapset == NULL) {
	G_fatal_error(_("opening %s: cannot find raster"), elevfname); 
	exit(EXIT_FAILURE);
  }

 /*open elevation map */
  int elevfd;
  if ((elevfd = G_open_cell_old(elevfname, mapset)) < 0) {
	G_fatal_error(_("Cannot open raster file [%s]"), elevfname);
	exit(EXIT_FAILURE);
  }
  
  /*get elevation data_type */
  RASTER_MAP_TYPE elev_data_type;
  elev_data_type = G_raster_map_type(elevfname, mapset);
  
  /* open visibility raster */
  int visfd;
  visfd = G_open_raster_new(visfname, elev_data_type);
  
  /*get the buffers to store each row */
  void *elevrast;
  elevrast = G_allocate_raster_buf(elev_data_type);
  assert(elevrast); 
  void *visrast;
  visrast = G_allocate_raster_buf(elev_data_type);
  assert(visrast); 

  /*set up stream reading stuff */
  off_t streamLen, counter = 0;
  AMI_err ae;
  VisCell *curResult = NULL;
  
  AMI_STREAM < VisCell > *vstr = visgrid->visStr;
  streamLen = vstr->stream_len();
  vstr->seek(0);
  
  /*read the first element */
  if (streamLen > 0) {
	ae = vstr->read_item(&curResult);
	assert(ae == AMI_ERROR_NO_ERROR);
	counter++;
  }
  
  dimensionType i, j;
  double elev=0, viewshed_value; 
  for (i = 0; i < G_window_rows(); i++) {

	if (G_get_raster_row(elevfd, elevrast, i, elev_data_type) <= 0) {
	  G_fatal_error(_("could not read row %d"), i);
	  exit(EXIT_FAILURE);
	}

	for (j = 0; j < G_window_cols(); j++) {
	
	  /* read the current elevation value */
	  int isNull = 0;
	  switch (elev_data_type) {
	  case CELL_TYPE:
		isNull = G_is_c_null_value(&((CELL *) elevrast)[j]);
		elev = (double)(((CELL *) elevrast)[j]);
		break;
	  case FCELL_TYPE:
		isNull = G_is_f_null_value(&((FCELL *) elevrast)[j]);
		elev = (double)(((FCELL *) elevrast)[j]);
		break;
	  case DCELL_TYPE:
		isNull = G_is_d_null_value(&((DCELL *) elevrast)[j]);
		elev = (double)(((DCELL *) elevrast)[j]);
		break;
	  }

 
	  if (curResult->row == i && curResult->col == j) {	
		/*cell is recodred in the visibility stream: it must be
		  either visible, or NODATA  */
		if (is_visible(curResult->angle)) 
		  writeValue(visrast,j, elev -vp_elev, elev_data_type);   
		else 
		  writeNodataValue(visrast,j, elev_data_type);
		/*read next element of stream */
		if (counter < streamLen) {
		  ae = vstr->read_item(&curResult);
		  assert(ae == AMI_ERROR_NO_ERROR);
		  counter++;
		}
	  }
	  else {
		/*  this cell is not in stream, so it is  invisible */
		writeValue(visrast, j, INVISIBLE, elev_data_type);
	  } 
	} /* for j */
	
		G_put_raster_row(visfd, visrast, elev_data_type);
  } /* for i */
  
  G_close_cell(elevfd);
  G_close_cell(visfd);
  return;
}



#endif
