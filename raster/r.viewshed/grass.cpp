
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
 *****************************************************************************/


#include <stdlib.h>
#include <stdio.h>

extern "C"
{
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/glocale.h>
}

#include "grass.h"
#include "visibility.h"





/* ------------------------------------------------------------ */
/* If viewOptions.doCurv is on then adjust the passed height for
   curvature of the earth; otherwise return the passed height
   unchanged.
   If viewOptions.doRefr is on then adjust the curved height for
   the effect of atmospheric refraction too.
 */
surface_type adjust_for_curvature(Viewpoint vp, double row,
			   double col, surface_type h,
			   ViewOptions viewOptions, GridHeader *hd)
{

    if (!viewOptions.doCurv)
	return h;

    assert(viewOptions.ellps_a != 0);

    /* distance must be in meters because ellps_a is in meters */
    double dist = G_distance(Rast_col_to_easting(vp.col + 0.5, &(hd->window)),
                             Rast_row_to_northing(vp.row + 0.5, &(hd->window)),
			     Rast_col_to_easting(col + 0.5, &(hd->window)),
			     Rast_row_to_northing(row + 0.5, &(hd->window)));

    double adjustment = (dist * dist) / (2.0 * viewOptions.ellps_a);

    if (!viewOptions.doRefr)
	return h - adjustment;

    return h - (adjustment * (1.0 - viewOptions.refr_coef));
}



/* ************************************************************ */
/*return a GridHeader that has all the relevant data filled in */
GridHeader *read_header(char *rastName, Cell_head * region)
{

    assert(rastName);

    /*allocate the grid header to fill */
    GridHeader *hd = (GridHeader *) G_malloc(sizeof(GridHeader));

    assert(hd);

    /*get the num rows and cols from GRASS */
    int nrows, ncols;

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    /*check for loss of prescion */
    if (nrows <= maxDimension && ncols <= maxDimension) {
	hd->nrows = (dimensionType) nrows;
	hd->ncols = (dimensionType) ncols;
    }
    else
	G_fatal_error(_("Grid dimension too big for current precision"));


    /*fill in rest of header */
    hd->xllcorner = Rast_col_to_easting(0, region);
    hd->yllcorner = Rast_row_to_northing(0, region);
    /* Cell_head stores 2 resolutions, while GridHeader only stores 1 */
       // make sure the two Cell_head resolutions are equal
    if (fabs(region->ew_res - region->ns_res) > .001) {
	G_warning(_("East-west resolution does not equal north-south resolution. "
		    "The viewshed computation assumes the cells are square, so in "
		    "this case this may result in innacuracies."));
	//    exit(EXIT_FAILURE);
    }
    hd->ew_res = region->ew_res;
    hd->ns_res = region->ns_res;
    //store the null value of the map
    Rast_set_null_value(&(hd->nodata_value), 1, G_SURFACE_TYPE);
    G_verbose_message("Nodata value set to %f", hd->nodata_value);
    
    
    
    return hd;
}

/* calculate ENTER and EXIT event elevation (bilinear interpolation) */
surface_type calculate_event_elevation(AEvent e, int nrows, int ncols,
                                       dimensionType vprow, dimensionType vpcol,
				       G_SURFACE_T **inrast, RASTER_MAP_TYPE data_type)
{
    int row1, col1;
    surface_type event_elev;
    G_SURFACE_T elev1, elev2, elev3, elev4;
    
    calculate_event_row_col(e, vprow, vpcol, &row1, &col1);
    if (row1 >= 0 && row1 < nrows && col1 >= 0 && col1 < ncols) {
	elev1 = inrast[row1 - e.row + 1][col1];
	elev2 = inrast[row1 - e.row + 1][e.col];
	elev3 = inrast[1][col1];
	elev4 = inrast[1][e.col];
	if (Rast_is_null_value(&elev1, data_type) ||
	    Rast_is_null_value(&elev2, data_type) ||
	    Rast_is_null_value(&elev3, data_type) ||
	    Rast_is_null_value(&elev4, data_type))
	    event_elev = inrast[1][e.col];
	else {
	    event_elev = (elev1 + elev2 + elev3 + elev4) / 4.;
	}
    }
    else
	event_elev = inrast[1][e.col];

    return event_elev;
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
init_event_list_in_memory(AEvent * eventList, char *rastName,
				Viewpoint * vp, GridHeader * hd,
				ViewOptions viewOptions, surface_type ***data,
				MemoryVisibilityGrid * visgrid)
{

    G_message(_("Computing events..."));
    assert(eventList && vp && visgrid);
    //GRASS should be defined 

    /*alloc data ; data is used to store all the cells on the same row
       as the viewpoint. */
    *data = (surface_type **)G_malloc(3 * sizeof(surface_type *));
    assert(*data);
    (*data)[0] = (surface_type *)G_malloc(3 * Rast_window_cols() * sizeof(surface_type));
    assert((*data)[0]);
    (*data)[1] = (*data)[0] + Rast_window_cols();
    (*data)[2] = (*data)[1] + Rast_window_cols();

    /*get the mapset name */
    const char *mapset;

    mapset = G_find_raster(rastName, "");
    if (mapset == NULL)
	G_fatal_error(_("Raster map [%s] not found"), rastName);

    /*open map */
    int infd;

    if ((infd = Rast_open_old(rastName, mapset)) < 0)
	G_fatal_error(_("Cannot open raster file [%s]"), rastName);

    /*get the data_type */
    RASTER_MAP_TYPE data_type;

    /* data_type = G_raster_map_type(rastName, mapset); */
    data_type = G_SURFACE_TYPE;

    /*buffer to hold 3 rows */
    G_SURFACE_T **inrast;
    int nrows = Rast_window_rows();
    int ncols = Rast_window_cols();

    inrast = (G_SURFACE_T **)G_malloc(3 * sizeof(G_SURFACE_T *));
    assert(inrast);
    inrast[0] = (G_SURFACE_T *)Rast_allocate_buf(data_type);
    assert(inrast[0]);
    inrast[1] = (G_SURFACE_T *)Rast_allocate_buf(data_type);
    assert(inrast[1]);
    inrast[2] = (G_SURFACE_T *)Rast_allocate_buf(data_type);
    assert(inrast[2]);
    
    Rast_set_null_value(inrast[0], ncols, data_type);
    Rast_set_null_value(inrast[1], ncols, data_type);
    Rast_set_null_value(inrast[2], ncols, data_type);

    int isnull = 0;

    /*keep track of the number of events added, to be returned later */
    size_t nevents = 0;

    /*scan through the raster data */
    dimensionType i, j;
    double ax, ay;
    AEvent e;
    
    /* read first row */
    Rast_get_row(infd, inrast[2], 0, data_type);

    e.angle = -1;
    for (i = 0; i < nrows; i++) {
	/*read in the raster row */
	
	G_SURFACE_T *tmprast = inrast[0];
	inrast[0] = inrast[1];
	inrast[1] = inrast[2];
	inrast[2] = tmprast;

	if (i < nrows - 1)
	    Rast_get_row(infd, inrast[2], i + 1, data_type);
	else
	    Rast_set_null_value(inrast[2], ncols, data_type);

	G_percent(i, nrows, 2);

	/*fill event list with events from this row */
	for (j = 0; j < Rast_window_cols(); j++) {
	    e.row = i;
	    e.col = j;

	    /*read the elevation value into the event */
	    isnull = Rast_is_null_value(&(inrast[1][j]), data_type);
	    e.elev[1] = inrast[1][j];

	    /* adjust for curvature */
	    e.elev[1] = adjust_for_curvature(*vp, i, j, e.elev[1], viewOptions, hd);

	    /*write it into the row of data going through the viewpoint */
	    if (i == vp->row) {
		(*data)[0][j] = e.elev[1];
		(*data)[1][j] = e.elev[1];
		(*data)[2][j] = e.elev[1];
	    }

	    /* set the viewpoint, and don't insert it into eventlist */
	    if (i == vp->row && j == vp->col) {
		set_viewpoint_elev(vp, e.elev[1] + viewOptions.obsElev);
		if (viewOptions.tgtElev > 0)
		    vp->target_offset = viewOptions.tgtElev;
		else
		    vp->target_offset = 0.;
		if (isnull) {
		    /*what to do when viewpoint is NODATA ? */
		    G_warning(_("Viewpoint is NODATA."));
		    G_message(_("Will assume its elevation is = %f"),
			      vp->elev);
		}

		add_result_to_inmem_visibilitygrid(visgrid, i, j,
						   180);
		continue;
	    }

	    /*don't insert in eventlist nodata cell events */
	    if (isnull) {
		/* record this cell as being NODATA; this is necessary so
		   that we can distingush invisible events, from nodata
		   events in the output */
		add_result_to_inmem_visibilitygrid(visgrid, i, j,
						   hd->nodata_value);
		continue;
	    }

	    /* if point is outside maxDist, do NOT include it as an
	       event */
	    if (is_point_outside_max_dist
		(*vp, *hd, i, j, viewOptions.maxDist))
		continue;

	    /* if it got here it is not the viewpoint, not NODATA, and
	       within max distance from viewpoint; generate its 3 events
	       and insert them */

	    /* get ENTER elevation */
	    e.eventType = ENTERING_EVENT;
	    e.elev[0] = calculate_event_elevation(e, nrows, ncols,
                                       vp->row, vp->col, inrast, data_type);
	    /* adjust for curvature */
	    if (viewOptions.doCurv) {
		calculate_event_position(e, vp->row, vp->col, &ay, &ax);
		e.elev[0] = adjust_for_curvature(*vp, ay, ax, e.elev[0], viewOptions, hd);
	    }

	    /* get EXIT elevation */
	    e.eventType = EXITING_EVENT;
	    e.elev[2] = calculate_event_elevation(e, nrows, ncols,
                                       vp->row, vp->col, inrast, data_type);
	    /* adjust for curvature */
	    if (viewOptions.doCurv) {
		calculate_event_position(e, vp->row, vp->col, &ay, &ax);
		e.elev[2] = adjust_for_curvature(*vp, ay, ax, e.elev[2], viewOptions, hd);
	    }

	    /*write adjusted elevation into the row of data going through the viewpoint */
	    if (i == vp->row) {
		(*data)[0][j] = e.elev[0];
		(*data)[1][j] = e.elev[1];
		(*data)[2][j] = e.elev[2];
	    }

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
    G_percent(nrows, nrows, 2);

    Rast_close(infd);

    G_free(inrast[0]);
    G_free(inrast[1]);
    G_free(inrast[2]);
    G_free(inrast);

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
AMI_STREAM < AEvent > *init_event_list(char *rastName, Viewpoint * vp,
					     GridHeader * hd,
					     ViewOptions viewOptions,
					     surface_type ***data,
					     IOVisibilityGrid * visgrid)
{

    G_message(_("Computing events..."));
    assert(rastName && vp && hd && visgrid);

    if (data != NULL) {
	/*data is used to store all the cells on the same row as the
	   //viewpoint. */
	*data = (surface_type **)G_malloc(3 * sizeof(surface_type *));
	assert(*data);
	(*data)[0] = (surface_type *)G_malloc(3 * Rast_window_cols() * sizeof(surface_type));
	assert((*data)[0]);
	(*data)[1] = (*data)[0] + Rast_window_cols();
	(*data)[2] = (*data)[1] + Rast_window_cols();
    }

    /*create the event stream that will hold the events */
    AMI_STREAM < AEvent > *eventList = new AMI_STREAM < AEvent > ();
    assert(eventList);

    /*determine which mapset we are in */
    const char *mapset;

    mapset = G_find_raster(rastName, "");
    if (mapset == NULL)
	G_fatal_error(_("Raster map [%s] not found"), rastName);

    /*open map */
    int infd;

    if ((infd = Rast_open_old(rastName, mapset)) < 0)
	G_fatal_error(_("Cannot open raster file [%s]"), rastName);

    RASTER_MAP_TYPE data_type;

    /* data_type = G_raster_map_type(rastName, mapset); */
    data_type = G_SURFACE_TYPE;
    G_SURFACE_T **inrast;
    int nrows = Rast_window_rows();
    int ncols = Rast_window_cols();

    inrast = (G_SURFACE_T **)G_malloc(3 * sizeof(G_SURFACE_T *));
    assert(inrast);
    inrast[0] = (G_SURFACE_T *)Rast_allocate_buf(data_type);
    assert(inrast[0]);
    inrast[1] = (G_SURFACE_T *)Rast_allocate_buf(data_type);
    assert(inrast[1]);
    inrast[2] = (G_SURFACE_T *)Rast_allocate_buf(data_type);
    assert(inrast[2]);
    
    Rast_set_null_value(inrast[0], ncols, data_type);
    Rast_set_null_value(inrast[1], ncols, data_type);
    Rast_set_null_value(inrast[2], ncols, data_type);

    /*scan through the raster data */
    int isnull = 0;
    dimensionType i, j;
    double ax, ay;
    AEvent e;

    /* read first row */
    Rast_get_row(infd, inrast[2], 0, data_type);

    e.angle = -1;

    /*start scanning through the grid */
    for (i = 0; i < nrows; i++) {
	
	G_percent(i, nrows, 2);

	/*read in the raster row */
	
	G_SURFACE_T *tmprast = inrast[0];
	inrast[0] = inrast[1];
	inrast[1] = inrast[2];
	inrast[2] = tmprast;

	if (i < nrows - 1)
	    Rast_get_row(infd, inrast[2], i + 1, data_type);
	else
	    Rast_set_null_value(inrast[2], ncols, data_type);

	/*fill event list with events from this row */
	for (j = 0; j < ncols; j++) {

	    e.row = i;
	    e.col = j;

	    /*read the elevation value into the event */
	    isnull = Rast_is_null_value(&(inrast[1][j]), data_type);
	    e.elev[1] = inrast[1][j];

	    /* adjust for curvature */
	    e.elev[1] = adjust_for_curvature(*vp, i, j, e.elev[1], viewOptions, hd);

	    if (data != NULL) {

		/*write the row of data going through the viewpoint */
		if (i == vp->row) {
		    (*data)[0][j] = e.elev[1];
		    (*data)[1][j] = e.elev[1];
		    (*data)[2][j] = e.elev[1];
		}
	    }

	    /* set the viewpoint */
	    if (i == vp->row && j == vp->col) {
		set_viewpoint_elev(vp, e.elev[1] + viewOptions.obsElev);
		/*what to do when viewpoint is NODATA */
		if (is_nodata(hd, e.elev[1])) {
		    G_warning("Viewpoint is NODATA.");
		    G_message("Will assume its elevation is %.f", e.elev[1]);
		};
		if (viewOptions.tgtElev > 0)
		    vp->target_offset = viewOptions.tgtElev;
		else
		    vp->target_offset = 0.;

		/* add viewpoint to visibility grid */
		VisCell visCell = { i, j, 180 };
		add_result_to_io_visibilitygrid(visgrid, &visCell);

		/*don't insert viewpoint into eventlist */
		continue;
	    }

	    /*don't insert the nodata cell events */
	    if (is_nodata(hd, e.elev[1])) {
		/* record this cell as being NODATA. ; this is necessary so
		   that we can distingush invisible events, from nodata
		   events in the output */
		VisCell visCell = { i, j, hd->nodata_value };
		add_result_to_io_visibilitygrid(visgrid, &visCell);
		continue;
	    }

	    /* if point is outside maxDist, do NOT include it as an
	       event */
	    if (is_point_outside_max_dist
		(*vp, *hd, i, j, viewOptions.maxDist))
		continue;

	    /* get ENTER elevation */
	    e.eventType = ENTERING_EVENT;
	    e.elev[0] = calculate_event_elevation(e, nrows, ncols,
                                       vp->row, vp->col, inrast, data_type);
	    /* adjust for curvature */
	    if (viewOptions.doCurv) {
		calculate_event_position(e, vp->row, vp->col, &ay, &ax);
		e.elev[0] = adjust_for_curvature(*vp, ay, ax, e.elev[0], viewOptions, hd);
	    }

	    /* get EXIT elevation */
	    e.eventType = EXITING_EVENT;
	    e.elev[2] = calculate_event_elevation(e, nrows, ncols,
                                       vp->row, vp->col, inrast, data_type);
	    /* adjust for curvature */
	    if (viewOptions.doCurv) {
		calculate_event_position(e, vp->row, vp->col, &ay, &ax);
		e.elev[2] = adjust_for_curvature(*vp, ay, ax, e.elev[2], viewOptions, hd);
	    }

	    if (data != NULL) {

		/*write the row of data going through the viewpoint */
		if (i == vp->row) {
		    (*data)[0][j] = e.elev[0];
		    (*data)[1][j] = e.elev[1];
		    (*data)[2][j] = e.elev[2];
		}
	    }

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
	}			/* for j */

    }				/* for i */
    G_percent(nrows, nrows, 2);

    Rast_close(infd);

    G_free(inrast[0]);
    G_free(inrast[1]);
    G_free(inrast[2]);
    G_free(inrast);

    G_debug(1, "nbEvents = %lu", (unsigned long)eventList->stream_len());
    G_debug(1, "Event stream length: %lu x %dB (%lu MB)",
	      (unsigned long)eventList->stream_len(), (int)sizeof(AEvent),
	      (unsigned long)(((long long)(eventList->stream_len() *
					   sizeof(AEvent))) >> 20));

    return eventList;
}






/* ************************************************************ */
/*  saves the grid into a GRASS raster.  Loops through all elements x
   in row-column order and writes fun(x) to file. */
void
save_grid_to_GRASS(Grid * grid, char *filename, RASTER_MAP_TYPE type,
		   float (*fun) (float))
{

    G_important_message(_("Writing output raster map..."));
    assert(grid && filename);

    /*open the new raster  */
    int outfd;

    outfd = Rast_open_new(filename, type);

    /*get the buffer to store values to read and write to each row */
    void *outrast;

    outrast = Rast_allocate_buf(type);
    assert(outrast);

    dimensionType i, j;

    for (i = 0; i < Rast_window_rows(); i++) {
        G_percent(i, Rast_window_rows(), 5);
	for (j = 0; j < Rast_window_cols(); j++) {

	    if (is_visible(grid->grid_data[i][j])) {
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
	    }
	    else
		writeNodataValue(outrast, j, type);
	}			/* for j */
	Rast_put_row(outfd, outrast, type);
    }				/* for i */
    G_percent(1, 1, 1);
    
    Rast_close(outfd);
    return;
}





/* ************************************************************ */
/*  using the visibility information recorded in visgrid, it creates an
   output viewshed raster with name outfname; for every point p that
   is visible in the grid, the corresponding value in the output
   raster is elevation(p) - viewpoint_elevation(p); the elevation
   values are read from elevfname raster */

void
save_vis_elev_to_GRASS(Grid * visgrid, char *elevfname, char *visfname,
		       float vp_elev)
{

    G_message(_("Saving grid to <%s>"), visfname);
    assert(visgrid && elevfname && visfname);

    /*get the mapset name */
    const char *mapset;

    mapset = G_find_raster(elevfname, "");
    if (mapset == NULL)
	G_fatal_error(_("Raster map [%s] not found"), elevfname);

    /*open elevation map */
    int elevfd;

    if ((elevfd = Rast_open_old(elevfname, mapset)) < 0)
	G_fatal_error(_("Cannot open raster file [%s]"), elevfname);

    /*get elevation data_type */
    RASTER_MAP_TYPE elev_data_type;

    elev_data_type = Rast_map_type(elevfname, mapset);

    /* create the visibility raster of same type */
    int visfd;

    visfd = Rast_open_new(visfname, elev_data_type);

    /*get the buffers to store each row */
    void *elevrast;

    elevrast = Rast_allocate_buf(elev_data_type);
    assert(elevrast);
    void *visrast;

    visrast = Rast_allocate_buf(elev_data_type);
    assert(visrast);

    dimensionType i, j;
    double elev = 0, viewshed_value;

    for (i = 0; i < Rast_window_rows(); i++) {
	/* get the row from elevation */
	Rast_get_row(elevfd, elevrast, i, elev_data_type);

	for (j = 0; j < Rast_window_cols(); j++) {

	    /* read the current elevation value */
	    int isNull = 0;

	    switch (elev_data_type) {
	    case CELL_TYPE:
		isNull = Rast_is_c_null_value(&((CELL *) elevrast)[j]);
		elev = (double)(((CELL *) elevrast)[j]);
		break;
	    case FCELL_TYPE:
		isNull = Rast_is_f_null_value(&((FCELL *) elevrast)[j]);
		elev = (double)(((FCELL *) elevrast)[j]);
		break;
	    case DCELL_TYPE:
		isNull = Rast_is_d_null_value(&((DCELL *) elevrast)[j]);
		elev = (double)(((DCELL *) elevrast)[j]);
		break;
	    }

	    if (is_visible(visgrid->grid_data[i][j])) {
		/* elevation cannot be null */
		assert(!isNull);
		/* write elev - viewpoint_elevation */
		viewshed_value = elev - vp_elev;
		writeValue(visrast, j, viewshed_value, elev_data_type);
	    }
	    else if (is_invisible_not_nodata(visgrid->grid_data[i][j])) {
		/* elevation cannot be null */
		assert(!isNull);
		/* write INVISIBLE */
		/*
		viewshed_value = INVISIBLE;
		writeValue(visrast, j, viewshed_value, elev_data_type);
		*/
		/* write  NODATA */
		writeNodataValue(visrast, j, elev_data_type);
	    }
	    else {
		/* nodata */
		assert(isNull);
		/* write  NODATA */
		writeNodataValue(visrast, j, elev_data_type);
	    }


	}			/* for j */
	Rast_put_row(visfd, visrast, elev_data_type);
    }				/* for i */

    Rast_close(elevfd);
    Rast_close(visfd);
    return;
}




/* helper function to deal with GRASS writing to a row buffer */
void writeValue(void *bufrast, int j, double x, RASTER_MAP_TYPE data_type)
{

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
	G_fatal_error(_("Unknown data type"));
    }
}

void writeNodataValue(void *bufrast, int j, RASTER_MAP_TYPE data_type)
{

    switch (data_type) {
    case CELL_TYPE:
	Rast_set_c_null_value(&((CELL *) bufrast)[j], 1);
	break;
    case FCELL_TYPE:
	Rast_set_f_null_value(&((FCELL *) bufrast)[j], 1);
	break;
    case DCELL_TYPE:
	Rast_set_d_null_value(&((DCELL *) bufrast)[j], 1);
	break;
    default:
	G_fatal_error(_("Unknown data type"));
    }
}


/* ************************************************************ */
/* write the visibility grid to GRASS. assume all cells that are not
   in stream are NOT visible. assume stream is sorted in (i,j) order.
   for each value x it writes to grass fun(x) */
void
save_io_visibilitygrid_to_GRASS(IOVisibilityGrid * visgrid,
				char *fname, RASTER_MAP_TYPE type,
				float (*fun) (float))
{

    G_message(_("Saving grid to <%s>"), fname);
    assert(fname && visgrid);

    /* open the output raster  and set up its row buffer */
    int visfd;

    visfd = Rast_open_new(fname, type);
    void *visrast;

    visrast = Rast_allocate_buf(type);
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

    for (i = 0; i < Rast_window_rows(); i++) {
	for (j = 0; j < Rast_window_cols(); j++) {

	    if (curResult->row == i && curResult->col == j) {
		/*cell is recodred in the visibility stream: it must be
		   either visible, or NODATA  */
		if (is_visible(curResult->angle))
		    writeValue(visrast, j, fun(curResult->angle), type);
		else
		    writeNodataValue(visrast, j, type);

		/*read next element of stream */
		if (counter < streamLen) {
		    ae = vstr->read_item(&curResult);
		    assert(ae == AMI_ERROR_NO_ERROR);
		    counter++;
		}
	    }
	    else {
		/*  this cell is not in stream, so it is invisible */
		writeNodataValue(visrast, j, type);
	    }
	}			/* for j */

	Rast_put_row(visfd, visrast, type);
    }				/* for i */

    Rast_close(visfd);
}







/* ************************************************************ */
/*  using the visibility information recorded in visgrid, it creates
   an output viewshed raster with name outfname; for every point p
   that is visible in the grid, the corresponding value in the output
   raster is elevation(p) - viewpoint_elevation(p); the elevation
   values are read from elevfname raster. assume stream is sorted in
   (i,j) order. */
void
save_io_vis_and_elev_to_GRASS(IOVisibilityGrid * visgrid, char *elevfname,
			      char *visfname, float vp_elev)
{

    G_message(_("Saving grid to <%s>"), visfname);
    assert(visfname && visgrid);

    /*get mapset name and data type */
    const char *mapset;

    mapset = G_find_raster(elevfname, "");
    if (mapset == NULL)
	G_fatal_error(_("Opening <%s>: cannot find raster"), elevfname);

    /*open elevation map */
    int elevfd;

    if ((elevfd = Rast_open_old(elevfname, mapset)) < 0)
	G_fatal_error(_("Cannot open raster file [%s]"), elevfname);

    /*get elevation data_type */
    RASTER_MAP_TYPE elev_data_type;

    elev_data_type = Rast_map_type(elevfname, mapset);

    /* open visibility raster */
    int visfd;

    visfd = Rast_open_new(visfname, elev_data_type);

    /*get the buffers to store each row */
    void *elevrast;

    elevrast = Rast_allocate_buf(elev_data_type);
    assert(elevrast);
    void *visrast;

    visrast = Rast_allocate_buf(elev_data_type);
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
    double elev = 0, viewshed_value;

    for (i = 0; i < Rast_window_rows(); i++) {

	Rast_get_row(elevfd, elevrast, i, elev_data_type);

	for (j = 0; j < Rast_window_cols(); j++) {

	    /* read the current elevation value */
	    int isNull = 0;

	    switch (elev_data_type) {
	    case CELL_TYPE:
		isNull = Rast_is_c_null_value(&((CELL *) elevrast)[j]);
		elev = (double)(((CELL *) elevrast)[j]);
		break;
	    case FCELL_TYPE:
		isNull = Rast_is_f_null_value(&((FCELL *) elevrast)[j]);
		elev = (double)(((FCELL *) elevrast)[j]);
		break;
	    case DCELL_TYPE:
		isNull = Rast_is_d_null_value(&((DCELL *) elevrast)[j]);
		elev = (double)(((DCELL *) elevrast)[j]);
		break;
	    }


	    if (curResult->row == i && curResult->col == j) {
		/*cell is recodred in the visibility stream: it must be
		   either visible, or NODATA  */
		if (is_visible(curResult->angle))
		    writeValue(visrast, j, elev - vp_elev, elev_data_type);
		else
		    writeNodataValue(visrast, j, elev_data_type);
		/*read next element of stream */
		if (counter < streamLen) {
		    ae = vstr->read_item(&curResult);
		    assert(ae == AMI_ERROR_NO_ERROR);
		    counter++;
		}
	    }
	    else {
		/*  this cell is not in stream, so it is  invisible */
		    writeNodataValue(visrast, j, elev_data_type);
	    }
	}			/* for j */

	Rast_put_row(visfd, visrast, elev_data_type);
    }				/* for i */

    Rast_close(elevfd);
    Rast_close(visfd);
    return;
}
