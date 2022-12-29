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

#ifndef _direction_H
#define _direction_H


#include <stdio.h>

#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#include <ostream>
#else
#include <ostream.h>
#endif


#include <grass/iostream/ami.h>

#include "types.h"
#include "genericWindow.h"
#include "nodata.h"



/***************************************************************/
/***************************************************************/
class directionWindow: public genericWindow<bool> {

public:
  int numdir;

  /***************************************************************/
  /* Modifies the window by setting to 1 only the neighbors to which
     direction point. This is the inverse function of encodeDirection().
     
     direction:   
     32 64 128  
     16 *   1   
     8  4   2 */
  directionWindow(direction_type dir)
    : genericWindow<bool>() {

    /* first set everything to 0 */
    numdir = 0;
    int i;
    for (i=0; i<9; i++) {
      set(i, false);
    }

    if (dir == 0 || dir == DIRECTION_UNDEF) {
      return;
    }
    assert(dir > 0 && dir < 256);
    if (dir & 1) {
      set(5, true); numdir++;
    }
    if (dir & 2) {
      set(8, true); numdir++;
    }
    if (dir & 4) {
      set(7, true); numdir++;
    }
    if (dir & 8) {
      set(6, true); numdir++;
    }
    if (dir & 16) {
      set(3, true); numdir++;
    }
    if (dir & 32) {
      set(0, true); numdir++;
    }
    if (dir & 64) {
      set(1, true); numdir++;
    }
    if (dir & 128) {
      set(2, true); numdir++;
    }
  }

  /***************************************************************/
  /* Check direction consistency. */
  void checkDirection(short di, short dj, int skipit,
		      elevation_type el, elevation_type elneighb) const {
#ifndef NDEBUG 
    if (skipit == 1) {
      assert(get(di,dj) ==  false);
    } else {
      if ((el > elneighb) && !is_nodata(elneighb) && !is_nodata(el)) {
	assert(get(di,dj) ==  true);
      }
    }
#endif   
  }
  
  
  /***************************************************************/
  /* Correct direction (di,dj). If direction points to invalid
     neighbor which must be skipped, set it to 0; if direction does
     not point to valid downslope neighbor, set it to 1. */
  void correctDirection(short di, short dj, int skipit, 
			dimension_type i, dimension_type j,
			elevation_type elev_crt, direction_type dir,
			elevation_type elev_neighb) {
    
    if (skipit && get(di,dj) == true) {
      cout << "WARNING:  at (" 
	   << i << "," << j << " , h=" << elev_crt << ", dir=" << dir << ")"
	   << "direction points to non-valid neighbor ("
	   <<  i + di << ","
	   <<  j + dj << ", h="
	   << elev_crt - elev_neighb
	   << ")\n";
      set(di,dj, false); /* correct it */
    }
    if (!skipit && elev_crt > elev_neighb &&  !is_nodata(elev_neighb)
	&& get(di,dj) == 0) {
      set(di,dj, true); /* correct it */
    }
  }
};


direction_type encodeDirection(const genericWindow<elevation_type>& elevwin,
			       const dimension_type nrows, 
			       const dimension_type ncols,
			       dimension_type row, dimension_type col);

direction_type encodeDirectionMFD(const genericWindow<elevation_type>& elevwin,
			       const dimension_type nrows, 
			       const dimension_type ncols,
			       dimension_type row, dimension_type col);

direction_type encodeDirectionSFD(const genericWindow<elevation_type>& elevwin,
			       const dimension_type nrows, 
			       const dimension_type ncols,
			       dimension_type row, dimension_type col);

direction_type findDominant(direction_type dir);
char directionSymbol(direction_type dir);


#endif

