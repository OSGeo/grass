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

#ifndef _fill_h
#define _fill_h

#include <grass/iostream/ami.h>
#include "common.h"
#include "water.h"

/* fill the terrain if necessary and compute flow direction stream;
   elstr is deleted and replaced with the classified elstr, which has
   boundary nodata distinguished from inner nodata */
AMI_STREAM<waterWindowBaseType> * 
computeFlowDirections(AMI_STREAM<elevation_type>*& elstr,
		      AMI_STREAM<elevation_type>*& filledstr,
		      AMI_STREAM<direction_type>*& dirstr,
		      AMI_STREAM<labelElevType> *& labeledWater);

#endif



