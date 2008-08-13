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


#ifdef __GRASS__

#ifndef _GRASS_H
#define _GRASS_H

#include <math.h>
extern "C"
{
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/glocale.h>
}

#include "eventlist.h"
#include "grid.h"
#include "visibility.h"


/* ------------------------------------------------------------ */
/* if viewOptions.doCurv is on then adjust the passed height for
   curvature of the earth; otherwise return the passed height
   unchanged. 
*/
float  adjust_for_curvature(Viewpoint vp, dimensionType row,
							dimensionType col, float h,
							ViewOptions viewOptions);


/* helper function to deal with GRASS writing to a row buffer */ 
void writeValue(void* ptr, int j, double x, RASTER_MAP_TYPE data_type); 
void writeNodataValue(void* ptr, int j,  RASTER_MAP_TYPE data_type);
  


/*return a GridHeader with all the relevant data filled in from GRASS */
GridHeader *read_header_from_GRASS(char *rastName, Cell_head * region);

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
								MemoryVisibilityGrid* visgrid ); 



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
					  IOVisibilityGrid* visgrid);


/* ************************************************************ */
/*  saves the grid into a GRASS raster.  Loops through all elements x
	in row-column order and writes fun(x) to file.*/
void
save_grid_to_GRASS(Grid* grid, char* filename, RASTER_MAP_TYPE type, 
				   float(*fun)(float));


/* ************************************************************ */
/*  using the visibility information recorded in visgrid, it creates an
	output viewshed raster with name outfname; for every point p that
	is visible in the grid, the corresponding value in the output
	raster is elevation(p) - viewpoint_elevation(p); the elevation
	values are read from elevfname raster */

void
save_vis_elev_to_GRASS(Grid* visgrid, char* elevfname, char* visfname,  
					   float vp_elev);


/* ************************************************************ */
/* write the visibility grid to GRASS. assume all cells that are not
   in stream are NOT visible. assume stream is sorted in (i,j) order.
   for each value x it writes to grass fun(x) */
void
save_io_visibilitygrid_to_GRASS(IOVisibilityGrid * visgrid, 
								 char* outfname, RASTER_MAP_TYPE type, 
								 float (*fun)(float));



/* ************************************************************ */
/*  using the visibility information recorded in visgrid, it creates
	an output viewshed raster with name outfname; for every point p
	that is visible in the grid, the corresponding value in the output
	raster is elevation(p) - viewpoint_elevation(p); the elevation
	values are read from elevfname raster. assume stream is sorted in
	(i,j) order. */
void
save_io_vis_and_elev_to_GRASS(IOVisibilityGrid * visgrid, char* elevfname, 
							  char* visfname, float vp_elev);

#endif/*_GRASS_H*/

#endif /*__GRASS__*/
