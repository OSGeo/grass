/****************************************************************************
 * 
 *  MODULE:	r.terraflow
 *
 *  COPYRIGHT (C) 2007 Laura Toma
 *   
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *****************************************************************************/

#ifndef __FILL_DEPR_H
#define __FILL_DEPR_H

#include <grass/iostream/ami.h>
#include "types.h"
#include "water.h"


/************************************************************/
/* INPUT: edgelist of watershed adjacency graph E={(u,v,h)}, 1 \le u,v
\le W; the maximum number of watersheds

h is the smallest height on the boundary between watershed u and
watershed v;

E contains the edges between the watersheds on the boundary and the
outside face; 

the outside face is assumed to be watershed number W+1

E is sorted increasingly by (h,u,v)

OUTPUT: an array raise[1..W], raise[i] is the height to which the
watershed i must be raised in order to have a valid flow path to the
outside watershed; 
*/
/************************************************************/

elevation_type* fill_depression(AMI_STREAM<boundaryType>*boundaryStr,
				cclabel_type maxWatersheds);

elevation_type*  inmemory_fill_depression(AMI_STREAM<boundaryType>*boundaryStr,
					  cclabel_type maxWatersheds);

elevation_type*  ext_fill_depression(AMI_STREAM<boundaryType>*boundaryStr,
				     cclabel_type maxWatersheds);



/************************************************************/
/* returns the amount of mmemory allocated by
   inmemory_fill_depression() */
/************************************************************/
size_t inmemory_fill_depression_mmusage(cclabel_type maxWatersheds);


/************************************************************/
/* produce a new stream where each elevation e inside watershed i is
   replaced with max(raise[i], e) */
/************************************************************/
void commit_fill(AMI_STREAM<labelElevType>* labeledGrid, 
		 elevation_type* raise, cclabel_type maxWatersheds, 
		 AMI_STREAM<elevation_type>* filledGrid);

#endif

