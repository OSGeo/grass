
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


#ifndef _KREVELD_H
#define _KREVELD_H

#include "visibility.h"
#include "grid.h"
#include "eventlist.h"

#include "grass.h"
#include <grass/iostream/ami.h>


/* ------------------------------------------------------------ */
/*return the memory usage in bytes of the algorithm when running in
   memory */
long long get_viewshed_memory_usage(GridHeader * hd);




/* ------------------------------------------------------------ */
/* run Kreveld's algorithm on the grid stored in the given file, and
   with the given viewpoint.  Create a visibility grid and return
   it. It runs in memory, i.e. the input grid and output grid are
   stored in 2D arrays in memory.  The computation runs in memory,
   which means the input grid, the status structure and the output
   grid are stored in arrays in memory.

   The output: A cell x in the visibility grid is recorded as follows:

   if it is NODATA, then x is set to NODATA if it is invisible, then x
   is set to INVISIBLE if it is visible, then x is set to the vertical
   angle wrt to viewpoint

 */
MemoryVisibilityGrid *viewshed_in_memory(char *inputfname,
					 GridHeader * hd,
					 Viewpoint * vp,
					 ViewOptions viewOptions);




/* ------------------------------------------------------------ */
/* compute viewshed on the grid stored in the given file, and with the
   given viewpoint.  Create a visibility grid and return it. The
   program runs in external memory.

   The output: A cell x in the visibility grid is recorded as follows:

   if it is NODATA, then x is set to NODATA if it is invisible, then x
   is set to INVISIBLE if it is visible, then x is set to the vertical
   angle wrt to viewpoint. 

 */
IOVisibilityGrid *viewshed_external(char *inputfname,
				    GridHeader * hd,
				    Viewpoint * vp, ViewOptions viewOptions);



void print_viewshed_timings(Rtimer initEventTime, Rtimer sortEventTime,
			    Rtimer sweepTime);



#endif
