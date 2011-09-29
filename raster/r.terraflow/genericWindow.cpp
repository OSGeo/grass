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

#include "types.h"
#include "genericWindow.h"

/* ********************************************************************** */
/* ********************************************************************** */

/* if center of the wind is a pit, fill it */

void 
fillPit(ElevationWindow& win) {
  /* find min of the 8 neighbors */
  elevation_type min = win.get(0);
  for (int k=1; k<9; k++) {
    if (k != 4 && win.get(k) < min) {
      min = win.get(k);
    }
  }
  if (win.get(4) < min) {
    win.set(4, min);
  }
};

